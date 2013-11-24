/*
  stepper.c - stepper motor driver: executes motion plans using stepper motors
  Part of Grbl

  Copyright (c) 2009-2011 Simen Svale Skogsrud

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

// Movement Design Description
//
// Like Marlin or Grbl, Minnow uses trapezoid speed curves to drive the steppers 
//
//         __________________________
//        /|                        |\     _________________         ^
//       / |                        | \   /|               |\        |
//      /  |                        |  \ / |               | \       s
//     /   |                        |   |  |               |  \      p
//    /    |                        |   |  |               |   \     e
//   +-----+------------------------+---+--+---------------+----+    e
//   |Phase1         Phase2       Phase3|                       |    d
//   |               BLOCK 1            |      BLOCK 2          |    
//                           time ----->
//
//  The trapezoid is the shape the speed curve over time. It starts at initial_rate, accelerates
//  until steps remaining < cmd->phase2_steps, keeps going at constant cmd->nominal_rate speed until
//  steps remaining < cmd->phase3_steps after which it decelerates to cmd->final_rate.
//
//  Minnow does not hardcode the number of axes or stepper pins and therefore the inner step loop
//  uses slightly more instructions to complete however this is mitigated because:
//    - it can still support 40000 steps per second (as per Marlin/Grbl). 
//    - all step inputs and output registers are precalculated to minimize execution time (so 
//       it is fairly close to hard-coded output times anyway)
//    - efficient block transmission and queuing means that high rates of small step blocks can be
//       maintained without motion-planning becoming the bottle-neck (especially a problem with 
//       non-cartesian co-ordinate systems).
//    - speed calculation is only done once per ISR invocaton as normal (i.e., regardless of number  
//       of axes) and is very similar to Marlin/Grbl.
//    - step spacing consistency is maximized because the current movement steps are output 
//       before the step interval is recalculated for the next step (and therefore the effect
//       of any differences in recalculation time are minimized).
//    - far less non-movement ISR CPU time is needed as there is no motion planning required 
//    - the underrun avoidance algorithm also avoids lengthy calculations and the extra logic is 
//       only used in abnormal operating cases when the system needs to be slowed down due 
//       to a lack of input data anyway.
//
//  Underrun Avoidance Algorithm Description
//  ----------------------------------------
//
//  The Underrun Avoidance Algorithm will slow the movement down to lesser of the nominal underrun 
//  rate for the primary axis and the requested nominal rate. 
//  The movement is changed to always accelerate up/down to this speed until the system approaches
//  the end of the movement block. The LinearMoveCommand block includes a precalculated 
//  step count required to decelerate from the expected underrun rate to the final speed (when that
//  is less than the nominal rate). 
//
//  The underrun avoidance algorithm must also ensure that system comes to a complete stop if the
//  block has a non-zero final speed but there are no more movement blocks. This is again accomplished
//  by using a pre-calculated step count to know when to start decelerating from the underrun rate
//  to a complete stop.
//
//  When the underrun condition is cleared, the system will accelerate to the normal nominal rate.
//  Once achieved, it will exit underrun mode. The trickiest part is working out when to start
//  decelerating if the nominal speed is not reached before approaching the end of the block.
//  If the system does not reach the nominal rate before phase 3, it will begin decelerating to the 
//  nominal underrun rate. From that speed the system knows when it must decelerate to reach the 
//  final rate (again using the precalculated step count). 
//
//  The corner case here is if the system hasn't reached the nominal underrun rate and the remaining 
//  steps are less than that which is required to decelerate from the underrun rate to the final rate.
//  In this (rare) case, the system will simply decelerate to the final rate. If the final rate is 
//  reached before the end of the block then the system will "hop" to the end by accelerating for half
//  the remaining steps and then decelerating again for the second half of the remaining steps - 
//  thereby reaching the end of the block at the required exit speed. The system will then exit 
//  underrun mode for the next block.

#include <avr/pgmspace.h>

#include "Minnow.h"

#include "movement_ISR.h"
#include "CommandQueue.h"
#include "QueueCommandStructs.h"
#include "Minnow.h"
#include "Device_InputSwitch.h"
#include "Device_OutputSwitch.h"
#include "Device_PwmOutput.h"
#include "Device_Buzzer.h"
#include "Device_Heater.h"

#include "speed_lookuptable.h"

// Some useful constants

#define ALLOWED_SPEED_DIFF 4

#define IDLE_INTERRUPT_RATE 2000 // == 1ms at the 2Mhz counter rate (ensures that serial Rx is still checked regularly)

// queue statics placed in this compilation unit to allow better optimization
uint8_t *CommandQueue::queue_buffer = 0;
uint16_t CommandQueue::queue_buffer_length = 0;
  
volatile uint8_t *CommandQueue::queue_head = 0;
volatile uint8_t *CommandQueue::queue_tail = 0; // ISR will never change this value

volatile uint8_t CommandQueue::in_progress_length = 0;
volatile uint16_t CommandQueue::current_queue_command_count ;
volatile uint16_t CommandQueue::total_attempted_queue_command_count;

extern uint16_t last_enqueued_final_speed;

bool come_to_stop_and_flush_queue;
bool is_checkpoint_last; // is last movement command in queue a checkpoint?


// Function declarations
FORCE_INLINE void movement_ISR(); // needs to be non-static due to friend usage elsewhere
FORCE_INLINE bool handle_queue_command(); // needs to be non-static due to friend usage elsewhere
FORCE_INLINE bool handle_linear_move();    
FORCE_INLINE bool handle_delay_command();    
FORCE_INLINE void setup_new_move();
FORCE_INLINE void update_directions_and_initial_counts();
FORCE_INLINE bool check_endstops();
FORCE_INLINE void write_steps();
FORCE_INLINE void recalculate_speed();
FORCE_INLINE bool check_underrun_condition();
FORCE_INLINE void setup_underrun_mode();
FORCE_INLINE void handle_underrun_condition();
FORCE_INLINE void accelerate_to_underrun_target_rate(uint16_t target_rate, bool is_final_rate);
FORCE_INLINE unsigned short calc_timer(unsigned short step_rate);
FORCE_INLINE uint8_t calc_step_loops(uint16_t my_step_rate);

// Pointer to current command
static uint8_t *command_in_progress;

// Linear Move State
static BITMASK(MAX_STEPPERS) axis_current_directions;
static const AxisMoveInfo *start_axis_move_info;

static uint8_t num_axes;
static uint16_t step_rate;
static uint8_t step_loops;
static uint32_t acceleration_time;

static uint16_t step_events_next_phase;
static uint16_t step_events_remaining;
static uint16_t total_step_events;

static bool in_phase_1;
static bool in_phase_2;
static bool in_phase_3;

static uint16_t nominal_rate;
static uint16_t nominal_rate_timer;
static uint16_t nominal_rate_step_loops;

static uint16_t initial_rate;
static uint16_t final_rate;

static uint16_t nominal_block_time; 

// queue counts(not including current block)
static uint32_t queued_microseconds_remaining; 
static uint32_t queued_steps_remaining;

// Underrun avoidance state
static bool underrun_condition; // whether condition is active
static bool underrun_active; // true if normal movement has not been regained after underrun_condition was true
static uint8_t current_underrun_accel_sign; // -1, 0, +1 (indicates when movement should accel, coast or decel).
static uint16_t current_underrun_accel_start_rate; // initial rate at which accel/decel started
static uint32_t current_underrun_accel_time; // time since accel/decel started
static uint16_t steps_for_underrun_hop_to_end; // steps required to "hop" to end of block at final speed
static uint16_t steps_to_final_speed_from_underrun_rate; // steps required to decel from min(underrun_max_rate,nominal_rate) to final speed
static uint16_t steps_to_stop_from_underrun_rate; // steps required to decel from min(underrun_max_rate,nominal_rate) to full stop
static uint16_t underrun_max_rate; // underrun nominal rate for current primary axis
static uint32_t underrun_acceleration_rate; // underrun acceleration rate for current primary axis

// Endstop State
static BITMASK(MAX_ENDSTOPS) endstop_hit;
static BITMASK(MAX_ENDSTOPS) stopped_axes;

// General state
static bool continuing;
static uint32_t tmp_uint32;

#if QUEUE_TEST
#define QUEUE_TEST_SHORT 0xF0
#define QUEUE_TEST_MEDIUM 0xF1
#define QUEUE_TEST_LONG 0xF2
uint8_t queue_test_sequence_number = 0;
uint16_t queue_test_enqueue_count = 0;
#endif

#ifdef MOVEMENT_DEBUG
uint32_t isr_start_time; // used to track starting time for ISR invocations
uint32_t isr_handling_time; // accumulative time spent in ISR when movement is being processed
uint32_t isr_elasped_time; // accumulative elapsed time when movement is being processed
#endif

//
// Routines
//

void movement_ISR_init() 
{
  // waveform generation = 0100 = CTC
  TCCR1B &= ~(1<<WGM13);
  TCCR1B |=  (1<<WGM12);
  TCCR1A &= ~(1<<WGM11);
  TCCR1A &= ~(1<<WGM10);

  // output mode = 00 (disconnected)
  TCCR1A &= ~(3<<COM1A0);
  TCCR1A &= ~(3<<COM1B0);

  // Set the timer pre-scaler
  // Generally we use a divider of 8, resulting in a 2MHz timer
  // frequency on a 16MHz MCU. If you are going to change this, be
  // sure to regenerate speed_lookuptable.h with
  // create_speed_lookuptable.py
  TCCR1B = (TCCR1B & ~(0x07<<CS10)) | (2<<CS10);

  OCR1A = IDLE_INTERRUPT_RATE;
  TCNT1 = 0;
  ENABLE_STEPPER_DRIVER_INTERRUPT();

  sei();
}

void movement_ISR_wake_up() 
{
  //  TCNT1 = 0;
  ENABLE_STEPPER_DRIVER_INTERRUPT();
}


// "The Movement Interrupt" - This timer interrupt is the workhorse.
// It pops blocks from the command queue and executes them.
ISR(TIMER1_COMPA_vect)
{
#ifdef MOVEMENT_DEBUG
  // we use this for determining the % of CP time that the movement_ISR is using
  if (isr_start_time != 0)
    isr_elasped_time += micros() - isr_start_time;
#endif 

  movement_ISR(); // hand-off out of extern C function allows C++ friend to work correctly (don't worry - its inlined)
  
#ifdef MOVEMENT_DEBUG
  if (continuing)
    isr_handling_time += micros() - isr_start_time;
  else
    isr_start_time = 0;
#endif  
}

FORCE_INLINE void dump_movement_queue()
{
  // dump queue
  CommandQueue::queue_head = CommandQueue::queue_tail;
  if (CommandQueue::queue_tail >= CommandQueue::queue_buffer
      && CommandQueue::queue_tail < CommandQueue::queue_buffer + CommandQueue::queue_buffer_length)
  {
    *CommandQueue::queue_tail = '\0'; // this causes a partially completed enqueue to fail
  }
  final_rate = 0;
  last_enqueued_final_speed = 0;
  come_to_stop_and_flush_queue = false;
}

FORCE_INLINE void movement_ISR()
{
  uint8_t cmd_start_cnt = 0;

#ifdef MOVEMENT_DEBUG
  isr_start_time = micros();
#endif  

  // are we in the middle of a command?
  if (continuing)
  {
    if (!is_stopped)
    {
do_handle_queue_command:    
      if (*command_in_progress == QUEUE_COMMAND_STRUCTS_TYPE_LINEAR_MOVE)
      {
        continuing = handle_linear_move();
      }
      else
      {
        continuing = handle_queue_command();
      }
      if (continuing)
      {
        // still not finished yet
        #ifndef AT90USB
        PSERIAL.checkRx(); // Check for serial chars frequently
        #endif
        return;
      }
      CommandQueue::current_queue_command_count -= 1;
      CommandQueue::queue_head += CommandQueue::in_progress_length + 1;
      //  in_progress_length will be updated later
    }
  }
  
  #ifndef AT90USB
  PSERIAL.checkRx(); // Check for serial chars frequently
  #endif
  
  // nothing is in progress so let's start on the next command in the queue
  uint8_t *queue_head = (uint8_t*)CommandQueue::queue_head;
  uint8_t *queue_tail = (uint8_t*)CommandQueue::queue_tail;
  uint8_t length;
  
  while (true)
  {
    // queue empty or do we need to stop?
    if (queue_head == queue_tail || is_stopped 
        || (come_to_stop_and_flush_queue && step_rate <= ALLOWED_SPEED_DIFF))
    {
      // nothing else to do or we need to stop
      CommandQueue::in_progress_length = 0;
      continuing = false;
      if (is_stopped || come_to_stop_and_flush_queue)
      {
        dump_movement_queue();
      }
      else
      {
  #if QUEUE_DEBUG
        if (CommandQueue::current_queue_command_count != 0)
          ERRORLNPGM("queue count wrong 1");
  #endif      
      }
      CommandQueue::current_queue_command_count = 0;
      OCR1A = IDLE_INTERRUPT_RATE; 
      return;
    }

    // allow other ISRs to execute if there is a sequence of non-continuing commands
    if (cmd_start_cnt >= 4)
    {
      CommandQueue::in_progress_length = 0;
      continuing = false;
      OCR1A = 100; // == 50us
      return;
    }
    
    // have we reached the end of the ring buffer
    if (queue_head >= CommandQueue::queue_buffer + CommandQueue::queue_buffer_length) 
    {
  #if QUEUE_DEBUG
      if (CommandQueue::queue_head > CommandQueue::queue_buffer + CommandQueue::queue_buffer_length)
        ERRORLNPGM("queue ran off the end");
  #endif      
      queue_head = CommandQueue::queue_buffer;
      CommandQueue::queue_head = queue_head;
    }

    // get the length of the next command
    length = *queue_head;
    CommandQueue::in_progress_length = length;
      
    if (length == 0)
    {
      // this is a skip marker
      
      // anything left in queue after the skip marker?
      if (queue_tail > queue_head)
      {
        // nothing left after skip marker (handled next time around the loop)
  #if QUEUE_DEBUG
        if (queue_tail != CommandQueue::queue_buffer + CommandQueue::queue_buffer_length)
          ERRORLNPGM("unexpected tail position");
  #endif      
        queue_head = queue_tail;
        CommandQueue::queue_head = queue_head; 
      }
      else
      {
        // next command is at start of buffer
        queue_head = CommandQueue::queue_buffer;
        CommandQueue::queue_head = queue_head; 
      }
      continue;
    }
    
    // we have the next command to execute.
    CommandQueue::total_attempted_queue_command_count += 1;
    cmd_start_cnt += 1;
    command_in_progress = queue_head+1;
    continuing = false;

  #if QUEUE_TEST
    DEBUGPGM("HQC: head=");
    DEBUG((int)(queue_head - CommandQueue::queue_buffer));
    DEBUGPGM(" tail=");
    DEBUG((int)(queue_tail - CommandQueue::queue_buffer));
    DEBUGPGM(" length=");
    DEBUG((int)length);
    DEBUGPGM("eqc=");
    DEBUG(queue_test_enqueue_count);
    DEBUGPGM(" ccc=");
    DEBUG(CommandQueue::current_queue_command_count);
    DEBUGPGM(" tcc=");
    DEBUGLN(CommandQueue::total_attempted_queue_command_count);
  #endif    
     
    // this is a bit ugly but saves on a massive duplication of inline code
    goto do_handle_queue_command; 
  }
}

FORCE_INLINE bool handle_queue_command()
{
  int8_t i;
  
  // start command processing
  switch (*command_in_progress)
  {
  case QUEUE_COMMAND_STRUCTS_TYPE_DELAY:
  {
    if (!continuing)
    {  
      const DelayQueueCommand *cmd = (const DelayQueueCommand*)command_in_progress;

      tmp_uint32 = micros() + cmd->delay; 
    }
    return handle_delay_command();
  }
  case QUEUE_COMMAND_STRUCTS_TYPE_SET_OUTPUT_SWITCH_STATE:
  {
    const SetOutputSwitchStateQueueCommand *cmd = (const SetOutputSwitchStateQueueCommand*)command_in_progress;
    const DeviceBitState *device_info = cmd->output_bit_info;
    for (i=0; i<cmd->num_bits; i++)
    {
      if (device_info->device_state < OUTPUT_SWITCH_STATE_DISABLED)
      {
        if (Device_OutputSwitch::output_switch_disabled[device_info->device_number])
        {
          // expect enabling/disabling a pin is not a common operation - so this not currently optimized
          pinMode(Device_OutputSwitch::output_switch_pins[device_info->device_number],OUTPUT); 
          Device_OutputSwitch::output_switch_disabled[device_info->device_number] = false;
        }
        volatile uint8_t *output_reg = device_info->device_reg;
        if (device_info->device_state == OUTPUT_SWITCH_STATE_LOW)
          *output_reg &= ~device_info->device_bit; 
        else
          *output_reg |= device_info->device_bit;
      }
      else
      {
        // expect enabling/disabling a pin is not a common operation - so this not currently optimized
        pinMode(Device_OutputSwitch::output_switch_pins[device_info->device_number],INPUT); 
        Device_OutputSwitch::output_switch_disabled[device_info->device_number] = true;
      }
      device_info += 1;
    }
    return false;
  }
  case QUEUE_COMMAND_STRUCTS_TYPE_SET_STEPPER_ENABLE_STATE:
  {
    SetStepperEnableStateQueueCommand *cmd = (SetStepperEnableStateQueueCommand*)command_in_progress;
    if (cmd->stepper_number == 0xFF)
    {
      // disable all steppers
      for (uint8_t i=0; i<AxisInfo::GetNumAxes(); i++)
        AxisInfo::WriteStepperEnableState(i, false);
    }
    else
    {
      AxisInfo::WriteStepperEnableState(cmd->stepper_number, cmd->stepper_state);
    }
    return false;
  }
  case QUEUE_COMMAND_STRUCTS_TYPE_SET_ENDSTOP_ENABLE_STATE:
  {
    SetEndstopEnableStateQueueCommand *cmd = (SetEndstopEnableStateQueueCommand*)command_in_progress;
    uint8_t bit = 0;
    do
    {
      if (((uint8_t)cmd->endstops_to_change & 1) != 0)
        AxisInfo::WriteEndstopEnableState(bit, cmd->endstop_enable_state & (1 << bit));
      cmd->endstops_to_change >>= 1;
      bit++;
    }
    while (cmd->endstops_to_change != 0);
    return false;
  }
  case QUEUE_COMMAND_STRUCTS_TYPE_SET_PWM_OUTPUT_STATE:
  {
    const SetPwmOutputStateQueueCommand *cmd = (const SetPwmOutputStateQueueCommand*)command_in_progress;
    Device_PwmOutput::WriteState(cmd->device_number, cmd->value);
    return false;
  }
  case QUEUE_COMMAND_STRUCTS_TYPE_SET_BUZZER_STATE:
  {
    const SetBuzzerStateQueueCommand *cmd = (const SetBuzzerStateQueueCommand*)command_in_progress;
    Device_Buzzer::WriteState(cmd->device_number, cmd->value);
    return false;
  }
  case QUEUE_COMMAND_STRUCTS_TYPE_SET_HEATER_TARGET_TEMP:
  {
    const SetHeaterTargetTempCommand *cmd = (const SetHeaterTargetTempCommand*)command_in_progress;
    Device_Heater::SetTargetTemperature(cmd->heater_number, cmd->target_temp);
    return false;
  }
  case QUEUE_COMMAND_STRUCTS_TYPE_SET_ACTIVE_TOOLHEAD:
  {
    // do nothing for now.
    return false;
  }
#if QUEUE_TEST
  case QUEUE_TEST_SHORT:
    if (CommandQueue::in_progress_length != 1)
    {
      ERRORPGM("Incorrect length in short queue test: ");
      ERRORLN((int)CommandQueue::in_progress_length);
    }
    return false;
  case QUEUE_TEST_MEDIUM:
    if (CommandQueue::in_progress_length != 2)
    {
      ERRORPGM("Incorrect length in medium queue test: ");
      ERRORLN((int)CommandQueue::in_progress_length);
    }
    if (command_in_progress[1] != queue_test_sequence_number++)
      ERRORLNPGM("Incorrect sequence number in medium queue test");
    return false;
  case QUEUE_TEST_LONG:
    if (CommandQueue::in_progress_length < 3)
    {
      ERRORPGM("Incorrect length in long queue test: ");
      ERRORLN((int)CommandQueue::in_progress_length);
    }
    if (command_in_progress[1] != queue_test_sequence_number++)
      ERRORLNPGM("Incorrect sequence number in long queue test");
    if (command_in_progress[2] != CommandQueue::in_progress_length - 3)
      ERRORLNPGM("Incorrect payload length in long queue test");
    return false;
#endif    
  default:
    ERRORPGM("Unknown cmd in ISR: cmd=");
    ERRORLN((int)*command_in_progress);
    return false;
  }
}

FORCE_INLINE bool handle_delay_command()
{
  int32_t delay = tmp_uint32 - micros();
  if (delay <= 0)
  {
    return false;
  }
  else
  {
    if (delay > IDLE_INTERRUPT_RATE / 2) // assumes 2Mhz counter frequency
      OCR1A = IDLE_INTERRUPT_RATE; 
    else
      OCR1A = max(delay * 2, 10L);
    return true;
  }
}
  
FORCE_INLINE bool handle_linear_move()
{
  if (!continuing)
  {
    setup_new_move();
    update_directions_and_initial_counts();
  }
  else 
  {
    if (step_events_remaining == 0)
      return false; // finished
  }
  
  if (!check_endstops())
    return false; // finished
    
  for(int8_t i=0; i < step_loops; i++) // Take multiple steps per interrupt (For high speed moves)
  {
    #ifndef AT90USB
    PSERIAL.checkRx(); // Check for serial chars frequently
    #endif

    write_steps();
  
    if (--step_events_remaining == 0)
      break;
  }
  
  // recalculation is done after outputting the current steps to achieve greatest step consistency
  // (the counter is already running, what we are doing here is calculating the 
  // point at which it next triggers an interrupt and resets)
  recalculate_speed();
  return true;
}  
 
FORCE_INLINE void setup_new_move()
{
  const LinearMoveCommand *cmd = (LinearMoveCommand *)command_in_progress;
  
  num_axes = cmd->num_axes;
  total_step_events = cmd->total_steps;
  step_events_remaining = total_step_events;
  step_events_next_phase = cmd->steps_phase_2;
  in_phase_1 = true;
  in_phase_2 = false;
  in_phase_3 = false;
  initial_rate = final_rate; 
  final_rate = cmd->final_rate;
  nominal_rate = cmd->nominal_rate;
  nominal_block_time = cmd->nominal_block_time;
  acceleration_time = 0;
  start_axis_move_info = cmd->axis_move_info;
  stopped_axes = 0;

  queued_microseconds_remaining -= nominal_block_time;
  queued_steps_remaining -= total_step_events;
  
  // currently only check for underrun condition at the start of the block
  // (although underrun_condition might be cleared during block if more 
  // movement blocks are enqueued).
  underrun_condition = check_underrun_condition();
  if (!underrun_active && !underrun_condition)
  {
    step_rate = initial_rate;
    step_loops = calc_step_loops(initial_rate);
  }
  else
  {
    if (!underrun_condition &&  
        (int16_t)(step_rate - initial_rate) <= ALLOWED_SPEED_DIFF && 
        (int16_t)(step_rate - initial_rate) >= -ALLOWED_SPEED_DIFF) 
    {
      // can exit underrun mode immediately.
      underrun_active = false;
      step_rate = initial_rate;
      step_loops = calc_step_loops(initial_rate);
    }
    else
    {
      // leave step_rate and step_loops unchanged but recaculate underrun rates and reset 
      // necessary state for block
      setup_underrun_mode();
    }
  }
  
#if TRACE_MOVEMENT
  DEBUGPGM("New ISR move: axes:");
  DEBUG(cmd->num_axes);
  DEBUGPGM(", homing:");
  DEBUG(cmd->homing_bit);
  DEBUGPGM(", tot steps:");
  DEBUG(cmd->total_steps);
  DEBUGPGM(" (P2:");
  DEBUG(cmd->steps_phase_2);
  DEBUGPGM("/P3:");
  DEBUG(cmd->steps_phase_3);
  DEBUGPGM("), rates(i/n/f):");
  DEBUG(initial_rate);
  DEBUGPGM(",");
  DEBUG(cmd->nominal_rate);
  DEBUGPGM(",");
  DEBUG(cmd->final_rate);
  DEBUGPGM(", accels(i/f):");
  DEBUG(cmd->acceleration_rate);
  DEBUGPGM(",");
  DEBUG(cmd->deceleration_rate);
  DEBUGPGM(", estops(i/e/h):");
  DEBUG_F(cmd->endstops_of_interest,HEX);
  DEBUGPGM("/");
  DEBUG_F(AxisInfo::endstop_enable_state,HEX);
  DEBUGPGM("/");
  DEBUG_F(endstop_hit,HEX);
  DEBUGPGM(", dirs:");
  DEBUG_F(cmd->directions,HEX);
  DEBUGPGM(", loops:");
  DEBUG(step_loops);
  DEBUGPGM(", blk time:");
  DEBUG(cmd->nominal_block_time);
  DEBUGPGM(", queued(t/s):");
  DEBUG(queued_microseconds_remaining);
  DEBUGPGM(",");
  DEBUG(queued_steps_remaining);
  DEBUGPGM(", urun:");
  DEBUG(underrun_active);
  DEBUGPGM(", last?:");
  DEBUG(is_checkpoint_last);
  DEBUGPGM(", stop?:");
  DEBUG(come_to_stop_and_flush_queue);
  DEBUG_EOL();
#endif  
    
}

FORCE_INLINE void update_directions_and_initial_counts()
{
  const AxisMoveInfo *axis_move_info = start_axis_move_info;
  uint8_t cnt = num_axes;
  uint16_t directions = ((LinearMoveCommand *)command_in_progress)->directions;
  while (true)
  {
    AxisInfoInternal *axis_info = axis_move_info->axis_info;
    BITMASK(MAX_STEPPERS) axis_bit = (1 << axis_info->stepper_number);

    if ((AxisInfo::stepper_enable_state & axis_bit) == 0)
    {
      if (!axis_info->stepper_enable_invert)
        *((volatile uint8_t *)axis_info->stepper_enable_reg) |= axis_info->stepper_enable_bit;
      else
        *((volatile uint8_t *)axis_info->stepper_enable_reg) &= ~(axis_info->stepper_enable_bit);
      AxisInfo::stepper_enable_state |= axis_bit;
    }
    
    // update count
    axis_info->step_event_counter = -(total_step_events >> 1);

    // update directions
    if (((uint8_t)directions & 1) != 0)
    {
      // +ve direction
      if ((axis_current_directions & axis_bit) == 0)
      {
        if (!axis_info->stepper_direction_invert)
          *((volatile uint8_t *)axis_info->stepper_direction_reg) |= axis_info->stepper_direction_bit;
        else
          *((volatile uint8_t *)axis_info->stepper_direction_reg) &= ~(axis_info->stepper_direction_bit);
        axis_current_directions |= axis_bit;
      }
    }
    else
    {
      // -ve direction
      if ((axis_current_directions & axis_bit) != 0)
      {
        if (!axis_info->stepper_direction_invert)
          *((volatile uint8_t *)axis_info->stepper_direction_reg) &= ~(axis_info->stepper_direction_bit);
        else
          *((volatile uint8_t *)axis_info->stepper_direction_reg) |= axis_info->stepper_direction_bit;
        axis_current_directions &= ~axis_bit;
      }
    }
    if (--cnt == 0)
      break;
    axis_move_info++;
    directions >>= 1;
  }
}   
  
FORCE_INLINE bool check_endstops()
{
  const LinearMoveCommand *cmd = (LinearMoveCommand *)command_in_progress;
  BITMASK(MAX_ENDSTOPS) endstops_to_check = cmd->endstops_of_interest & AxisInfo::endstop_enable_state;
  uint8_t index = 0;
  uint16_t directions = cmd->directions;
  while (endstops_to_check != 0)
  {
    while (((uint8_t)endstops_to_check & 1) == 0)
    {
      endstops_to_check >>= 1;
      index++;
    }
    
    BITMASK(MAX_ENDSTOPS) endstop_bit = 1 << index;
    bool new_endstop_hit = Device_InputSwitch::ReadState(index) == ((AxisInfo::endstop_trigger_level & endstop_bit) != 0);
    if (new_endstop_hit && (endstop_hit & endstop_bit))
    {
      if (cmd->homing_bit)
      {
        // find all axes using this endstop
        const AxisMoveInfo *axis_move_info = start_axis_move_info;
        uint8_t cnt = num_axes;
        while (true)
        {
          AxisInfoInternal *axis_info = axis_move_info->axis_info;
          if ((directions & (1<<axis_info->stepper_number)) == 0)
          {
            if ((axis_info->min_endstops_configured & endstop_bit) != 0)
            {
              // stop this axis - keep others going
              if (axis_move_info->step_count != 0)
                stopped_axes += 1;
              ((AxisMoveInfo *)axis_move_info)->step_count = 0;
              axis_info->step_event_counter = 0;
              if (stopped_axes == num_axes)
                return false;
            }
          }
          else
          {
            if ((axis_info->max_endstops_configured & endstop_bit) != 0)
            {
              // stop this axis - keep others going
              if (axis_move_info->step_count != 0)
                stopped_axes += 1;
              ((AxisMoveInfo *)axis_move_info)->step_count = 0;
              axis_info->step_event_counter = 0;
              if (stopped_axes == num_axes)
                return false;
            }
          }
          if (--cnt == 0)
            break;
          axis_move_info++;
        }
      }
      else
      {
        // stop system
        DISABLE_STEPPER_DRIVER_INTERRUPT();
        is_stopped = true;
        final_rate = 0;
        // TODO handle
      }
    }
    if (new_endstop_hit)
      endstop_hit |= endstop_bit;
    else
      endstop_hit &= ~endstop_bit;
    
    endstops_to_check >>= 1;
    index++;
  }
  return true;
}

FORCE_INLINE void write_steps()
{
  const AxisMoveInfo *axis_move_info = start_axis_move_info;
  uint8_t cnt = num_axes;
  while (true)
  {
    AxisInfoInternal *axis_info = axis_move_info->axis_info;
    volatile uint8_t *step_reg = axis_info->stepper_step_reg;
    if ((axis_info->step_event_counter += axis_move_info->step_count) > 0) 
    {
      if (!axis_info->stepper_step_invert)
      {
        *step_reg |= axis_info->stepper_step_bit;
        axis_info->step_event_counter -= total_step_events;
        *step_reg &= ~(axis_info->stepper_step_bit);
      }
      else
      {
        *step_reg &= ~(axis_info->stepper_step_bit);
        axis_info->step_event_counter -= total_step_events;
        *step_reg |= axis_info->stepper_step_bit;
      }        
    }
    if (--cnt == 0)
      break;
    axis_move_info++;
  }
}  

FORCE_INLINE void recalculate_speed()
{ 
  const LinearMoveCommand *cmd = (const LinearMoveCommand *)command_in_progress;
  uint16_t timer;
  
  if (underrun_active || come_to_stop_and_flush_queue)
  {
    // do underrun avoidance handling
    handle_underrun_condition();
    return;
  }
  
  // check if we need to move to the next movement phase
  if (in_phase_1 && step_events_remaining <= step_events_next_phase
      && step_events_remaining > cmd->steps_phase_3)
  {
    in_phase_1 = false;
    in_phase_2 = true;
    step_events_next_phase = cmd->steps_phase_3;
    nominal_rate_timer = calc_timer(nominal_rate);
    nominal_rate_step_loops = step_loops;
  }
  else if (!in_phase_3 && step_events_remaining <= step_events_next_phase)
  { // this also handles the case where there is no phase 2.
    in_phase_1 = false; 
    in_phase_2 = false;
    in_phase_3 = true;
    acceleration_time = 0;
  }
  
  // normal system operatoin
  if (in_phase_1)
  {
    // phase 1: accelerate from initial_rate to nominal_rate
    MultiU24X24toH16(step_rate, acceleration_time, cmd->acceleration_rate);
    step_rate += initial_rate;

    // upper limit
    if(step_rate > nominal_rate)
      step_rate = nominal_rate;

    // step_rate to timer interval
    timer = calc_timer(step_rate);
    OCR1A = timer;
    acceleration_time += timer;
  }
  else if (in_phase_3)
  {
    // phase 3: decelerate from nominal_rate to final_rate
    MultiU24X24toH16(step_rate, acceleration_time, cmd->deceleration_rate);

    if(step_rate > nominal_rate) 
    { // Check step_rate stays positive
      step_rate = final_rate;
    }
    else 
    {
      step_rate = nominal_rate - step_rate; // Decelerate from aceleration end point.
    }

    // lower limit
    if(step_rate < final_rate)
      step_rate = final_rate;

    // step_rate to timer interval
    timer = calc_timer(step_rate);
    OCR1A = timer;
    acceleration_time += timer;
  }
  else 
  {
    // phase 2: maintain nominal_rate
    OCR1A = nominal_rate_timer;
    step_loops = nominal_rate_step_loops;
  }
}

FORCE_INLINE bool check_underrun_condition()
{
  uint16_t queue_count = CommandQueue::current_queue_command_count;
  uint32_t queued_time = queued_microseconds_remaining + nominal_block_time;
  if (come_to_stop_and_flush_queue)
    return true;
  if (is_checkpoint_last)
    return false;
  if ((queue_count < AxisInfo::underrun_queue_low_level && queued_time < AxisInfo::underrun_queue_high_time)
      || queued_time < AxisInfo::underrun_queue_low_time)
    return true;
  else
    return false;    
}

FORCE_INLINE void setup_underrun_mode()
{
  const LinearMoveCommand *cmd = (LinearMoveCommand *)command_in_progress;
  
  const AxisInfoInternal *primary_axis_info = start_axis_move_info[cmd->primary_axis].axis_info;
  underrun_acceleration_rate = max(max(cmd->acceleration_rate, cmd->deceleration_rate),
                                    primary_axis_info->underrun_accel_rate);
  underrun_max_rate = primary_axis_info->underrun_max_rate;
  steps_to_final_speed_from_underrun_rate = cmd->steps_to_final_speed_from_underrun_rate;
  steps_for_underrun_hop_to_end = 0; 
  current_underrun_accel_sign = 0;
  underrun_active = true;
}
 

// underrun avoidance algorithm handling
//
// there are a few different cases to cover here but none of them involve 
// complex calculations
//
// remember also that the system will be operating at lower speeds normally
// and that this is considered an abnormal operating mode.
FORCE_INLINE void handle_underrun_condition()
{
  uint16_t target_rate;
  bool is_final_rate = false;
  
  // is the underrun condition persisting?
  if (underrun_condition)
  {
    if (!underrun_active)
    {
      // not previously in underrun condition
      setup_underrun_mode();
    }

    // to minimise checking in high speed cases we always decel to underrun rate first
    if (step_rate > underrun_max_rate) 
    {
      target_rate = underrun_max_rate;
    }
    // check whether we are about to run out of steps and need to come to a stop?
    else if (queued_steps_remaining < steps_to_stop_from_underrun_rate
            && step_events_remaining + (uint16_t)queued_steps_remaining <= steps_to_stop_from_underrun_rate)
    {
      target_rate = 0;
      is_final_rate = true;
    }
    else if (come_to_stop_and_flush_queue
            && step_events_remaining <= steps_to_stop_from_underrun_rate)
    {
      target_rate = 0;
      is_final_rate = true;
    }
    else if (step_events_remaining <= steps_to_final_speed_from_underrun_rate)
    {
      target_rate = min(final_rate,underrun_max_rate);
      is_final_rate = true;
    }
    else
    {
      target_rate = underrun_max_rate; 
    }
  }
  else 
  {
    // underrun condition has cleared - time to attempt to resume normal movement
    if (step_events_remaining <= steps_to_final_speed_from_underrun_rate)
    {
      target_rate = final_rate;
      is_final_rate = true;
    }
    else if (step_events_remaining <= ((const LinearMoveCommand *)command_in_progress)->steps_phase_3)
    {
      target_rate = max(final_rate, underrun_max_rate);
    }
    else 
    {
      // we aren't near the end of the block - so speed up to nominal again.
      target_rate = nominal_rate;
      if (step_rate == nominal_rate)
      {
        // re-attained nominal speed - so we can leave underrun mode now.
        underrun_active = false;
      }
    }
  }

  accelerate_to_underrun_target_rate(target_rate, is_final_rate);
}

FORCE_INLINE void accelerate_to_underrun_target_rate(uint16_t target_rate, bool is_final_rate)
{
  if (is_final_rate && steps_for_underrun_hop_to_end != 0)
  {
    if (step_events_remaining > steps_for_underrun_hop_to_end)
    {
      // accelerate for first half of hop
      if (current_underrun_accel_sign != 1)
      {
        // change in acceleration
        current_underrun_accel_time = 0;
        current_underrun_accel_start_rate = step_rate;
        current_underrun_accel_sign = 1;
      }
    }
    else
    {
      // decelerate for second half of hop
      if (step_events_remaining <= step_loops)
      {
        // last loop through - set final speed to match.
        current_underrun_accel_sign = 0;
        step_rate = final_rate;
      }
      else if (current_underrun_accel_sign != -1)
      {
        // change in acceleration
        current_underrun_accel_time = 0;
        current_underrun_accel_start_rate = step_rate;
        current_underrun_accel_sign = -1;
      }
    }
  }
  else if (step_rate > target_rate)
  { 
    if (current_underrun_accel_sign != 1)
    {
      // we've reach the target plateau speed from below
      current_underrun_accel_sign = 0;
      step_rate = target_rate;
      is_final_rate = false; // avoid later check
    }
    else if (current_underrun_accel_sign != -1)
    {
      // change in acceleration
      current_underrun_accel_time = 0;
      current_underrun_accel_start_rate = step_rate;
      current_underrun_accel_sign = -1;
    }
    else
    {
      // keep going
    }
  }
  else if (step_rate < target_rate)
  {
    if (current_underrun_accel_sign < 0)
    {
      // we've reach the target plateau speed from above
      current_underrun_accel_sign = 0;
      step_rate = target_rate;
    }
    else if (current_underrun_accel_sign != 1)
    {
      // change in acceleration
      current_underrun_accel_time = 0;
      current_underrun_accel_start_rate = step_rate;
      current_underrun_accel_sign = 1; // accelerate
    }
    else
    {
      // keep going
    }
  }
  else
  {
    current_underrun_accel_sign = 0;
  }
  
  if (is_final_rate 
      && current_underrun_accel_sign == 0 // i.e., step_rate == target_rate
      && step_rate < underrun_max_rate // (if its higher than this rate, we'll just wait until we reach the end)
      && step_events_remaining > (uint8_t)(step_loops << 1)) 
  {
    // this is a corner case. we've reached the target speed too early and it
    // is a very low speed - potentially even 0.
    // Reaccelerate and decelerate for an equal distance to reach end of block (hop).
    current_underrun_accel_sign = 1;
    steps_for_underrun_hop_to_end = step_events_remaining >> 1;
  }

  if (current_underrun_accel_sign > 0)
  {
    MultiU24X24toH16(step_rate, current_underrun_accel_time, underrun_acceleration_rate);
    step_rate += current_underrun_accel_start_rate;
  }
  else if (current_underrun_accel_sign < 0)
  {
    MultiU24X24toH16(step_rate, current_underrun_accel_time, underrun_acceleration_rate);
    step_rate = current_underrun_accel_start_rate - step_rate;
  }

  // step_rate to timer interval
  uint16_t timer = calc_timer(step_rate);
  OCR1A = timer;
  current_underrun_accel_time += timer;
  return;
}

// Copied from Marlin
FORCE_INLINE unsigned short calc_timer(unsigned short step_rate) {
  unsigned short timer;
  if(step_rate > MAX_STEP_FREQUENCY) step_rate = MAX_STEP_FREQUENCY;

  if(step_rate > 20000) { // If steprate > 20kHz >> step 4 times
    step_rate = (step_rate >> 2)&0x3fff;
    step_loops = 4;
  }
  else if(step_rate > 10000) { // If steprate > 10kHz >> step 2 times
    step_rate = (step_rate >> 1)&0x7fff;
    step_loops = 2;
  }
  else {
    step_loops = 1;
  }

  if(step_rate < (F_CPU/500000)) step_rate = (F_CPU/500000);
  step_rate -= (F_CPU/500000); // Correct for minimal speed
  if(step_rate >= (8*256)){ // higher step rate
    unsigned short table_address = (unsigned short)&speed_lookuptable_fast[(unsigned char)(step_rate>>8)][0];
    unsigned char tmp_step_rate = (step_rate & 0x00ff);
    unsigned short gain = (unsigned short)pgm_read_word_near(table_address+2);
    MultiU16X8toH16(timer, tmp_step_rate, gain);
    timer = (unsigned short)pgm_read_word_near(table_address) - timer;
  }
  else { // lower step rates
    unsigned short table_address = (unsigned short)&speed_lookuptable_slow[0][0];
    table_address += ((step_rate)>>1) & 0xfffc;
    timer = (unsigned short)pgm_read_word_near(table_address);
    timer -= (((unsigned short)pgm_read_word_near(table_address+2) * (unsigned char)(step_rate & 0x0007))>>3);
  }
  if(timer < 100) 
  {
    timer = 100; 
#if MOVEMENT_DEBUG        //(20kHz this should never happen)
    ERROR("Stepper too high:"); ERRORLN(step_rate); 
#endif
  }
  return timer;
}

// this is a subset which justs calcs the step_loops value
uint8_t calc_step_loops(uint16_t my_step_rate) 
{
  if(my_step_rate > MAX_STEP_FREQUENCY) my_step_rate = MAX_STEP_FREQUENCY;

  if(my_step_rate > 20000) { // If steprate > 20kHz >> step 4 times
    return 4;
  }
  else if(my_step_rate > 10000) { // If steprate > 10kHz >> step 2 times
    return 2;
  }
  else {
    return 1;
  }
}

#if QUEUE_TEST
void run_queue_test()
{
  uint8_t buffer[50];
  CommandQueue::Init(buffer, sizeof(buffer));
  
  uint16_t remaining_slots;
  uint16_t current_command_count;
  uint16_t total_command_count;
  uint8_t *ptr;
  int i;

  DEBUGLNPGM("Begin test");  
  
  for (i = 0; i < 1000; i++)
  {
    while ((ptr = CommandQueue::GetCommandInsertionPoint(1)) == 0)
      ;
    *ptr = QUEUE_TEST_SHORT;
    if (!CommandQueue::EnqueueCommand(1))
      ERRORLNPGM("Failed to short enqueue");
    
    if (i % 59 == 0)
    {
      while ((ptr = CommandQueue::GetCommandInsertionPoint(5)) == 0)
        ;
      memset(ptr, 0, 5);
      ptr[0] = QUEUE_COMMAND_STRUCTS_TYPE_DELAY;
      ptr[4] = 1 + (i % 13) ;
      if (!CommandQueue::EnqueueCommand(5))
        ERRORLNPGM("Failed to short enqueue delay");
    }
  }

  CommandQueue::GetQueueInfo(remaining_slots, current_command_count, total_command_count);
  DEBUGPGM("Info1a: slots=");
  DEBUG(remaining_slots);
  DEBUGPGM(" ccc=");
  DEBUG(current_command_count);
  DEBUGPGM(" tcc=");
  DEBUGLN(total_command_count);

  delay(1000);

  CommandQueue::GetQueueInfo(remaining_slots, current_command_count, total_command_count);
  DEBUGPGM("Info1b: slots=");
  DEBUG(remaining_slots);
  DEBUGPGM(" ccc=");
  DEBUG(current_command_count);
  DEBUGPGM(" tcc=");
  DEBUGLN(total_command_count);

  uint8_t sequence_num = 0;
  for (i = 0; i < 1000; i++)
  {
    while ((ptr = CommandQueue::GetCommandInsertionPoint(2)) == 0)
      ;
    *ptr++ = QUEUE_TEST_MEDIUM;
    *ptr = sequence_num++;
    if (!CommandQueue::EnqueueCommand(2))
      ERRORLNPGM("Failed to medium enqueue");

    if (i % 59 == 0)
    {
      while ((ptr = CommandQueue::GetCommandInsertionPoint(5)) == 0)
        ;
      memset(ptr, 0, 5);
      ptr[0] = QUEUE_COMMAND_STRUCTS_TYPE_DELAY;
      ptr[4] = 1 + (i % 13) ;
      if (!CommandQueue::EnqueueCommand(5))
        ERRORLNPGM("Failed to short enqueue delay");
    }
  }
  
  CommandQueue::GetQueueInfo(remaining_slots, current_command_count, total_command_count);
  DEBUGPGM("Info2a: slots=");
  DEBUG(remaining_slots);
  DEBUGPGM(" ccc=");
  DEBUG(current_command_count);
  DEBUGPGM(" tcc=");
  DEBUGLN(total_command_count);

  delay(1000);

  CommandQueue::GetQueueInfo(remaining_slots, current_command_count, total_command_count);
  DEBUGPGM("Info2b: slots=");
  DEBUG(remaining_slots);
  DEBUGPGM(" ccc=");
  DEBUG(current_command_count);
  DEBUGPGM(" tcc=");
  DEBUGLN(total_command_count);

#if 1
  for (int i = 0; i < 1005; i++)
  {
    while ((ptr = CommandQueue::GetCommandInsertionPoint(3+(i%30))) == 0)
      ;
    *ptr++ = QUEUE_TEST_LONG;
    *ptr++ = sequence_num++;
    *ptr++ = i%30;
    if (!CommandQueue::EnqueueCommand(3+(i%30)))
      ERRORLNPGM("Failed to long enqueue");

    if (i % 59 == 0)
    {
      while ((ptr = CommandQueue::GetCommandInsertionPoint(5)) == 0)
        ;
      memset(ptr, 0, 5);
      ptr[0] = QUEUE_COMMAND_STRUCTS_TYPE_DELAY;
      ptr[4] = 1 + (i % 13) ;
      if (!CommandQueue::EnqueueCommand(5))
        ERRORLNPGM("Failed to short enqueue delay");
    }
  }
    
  CommandQueue::GetQueueInfo(remaining_slots, current_command_count, total_command_count);
  DEBUGPGM("Info3a: slots=");
  DEBUG(remaining_slots);
  DEBUGPGM(" ccc=");
  DEBUG(current_command_count);
  DEBUGPGM(" tcc=");
  DEBUGLN(total_command_count);

  delay(1000);

  CommandQueue::GetQueueInfo(remaining_slots, current_command_count, total_command_count);
  DEBUGPGM("Info3b: slots=");
  DEBUG(remaining_slots);
  DEBUGPGM(" ccc=");
  DEBUG((int)current_command_count);
  DEBUGPGM(" tcc=");
  DEBUGLN(total_command_count);
#endif  
}
#endif //if QUEUE_TEST