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

#include <avr/pgmspace.h>

#include "movement_ISR.h"
#include "CommandQueue.h"
#include "QueueCommandStructs.h"
#include "Minnow.h"
#include "Device_OutputSwitch.h"
#include "Device_PwmOutput.h"
#include "Device_Buzzer.h"
#include "Device_Heater.h"

// Some useful constants


#define ENABLE_STEPPER_DRIVER_INTERRUPT()  TIMSK1 |= (1<<OCIE1A)
#define DISABLE_STEPPER_DRIVER_INTERRUPT() TIMSK1 &= ~(1<<OCIE1A)

// queue statics placed in this compilation unit to allow better optimization
uint8_t *CommandQueue::queue_buffer = 0;
uint16_t CommandQueue::queue_buffer_length = 0;
  
volatile uint8_t *CommandQueue::queue_head = 0;
volatile uint8_t *CommandQueue::queue_tail = 0; // ISR will never change this value

volatile uint8_t CommandQueue::in_progress_length = 0;
volatile uint16_t CommandQueue::current_queue_command_count ;
volatile uint16_t CommandQueue::total_attempted_queue_command_count;

// Function declarations
FORCE_INLINE void movement_ISR(); // needs to be non-static due to friend usage elsewhere
FORCE_INLINE bool handleQueueCommand(const uint8_t* command, uint8_t command_length, bool continuing); // needs to be non-static due to friend usage elsewhere
FORCE_INLINE bool handleLinearMove(const LinearMoveCommand *command, bool continuing);    

// Temporary variables used to store command state between ISR invocations

uint32_t microseconds_move_remaining;
bool is_checkpoint_last;


static uint32_t tmp_uint32;

#if QUEUE_TEST
#define QUEUE_TEST_SHORT 0xF0
#define QUEUE_TEST_MEDIUM 0xF1
#define QUEUE_TEST_LONG 0xF2
uint8_t queue_test_sequence_number = 0;
uint16_t queue_test_enqueue_count = 0;
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

  OCR1A = 0x4000;
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
  movement_ISR(); // hand-off out of extern C function allows C++ friend to work correctly (don't worry - its inlined)
}

void movement_ISR()
{
  bool continuing;
  uint8_t length;
  
  #ifndef AT90USB
  PSERIAL.checkRx(); // Check for serial chars frequently
  #endif
  
  length = CommandQueue::in_progress_length;

  while (true)
  {
    // are we in the middle of working on a command?
    if (length == 0)
    {
      // nothing is in progress so let's start on the next command in the queue
      if (CommandQueue::queue_head == CommandQueue::queue_tail)
      {
        // nothing left to do
        CommandQueue::in_progress_length = 0;
#if QUEUE_DEBUG
        if (CommandQueue::current_queue_command_count != 0)
          ERRORLNPGM("queue count wrong 1");
#endif      
        OCR1A = 2000; // == 1ms
        return;
      }
     
      // get the length of the next command
      length = *CommandQueue::queue_head;
      
      if (length == 0)
      {
        // this is a skip marker
        
        // anything left in queue after the skip marker
        if (CommandQueue::queue_tail > CommandQueue::queue_head)
        {
          // nothing left after skip marker (handled next time around the loop)
#if QUEUE_DEBUG
          if (CommandQueue::queue_tail != CommandQueue::queue_buffer + CommandQueue::queue_buffer_length)
            ERRORLNPGM("unexpected tail position");
#endif      
          CommandQueue::queue_head = CommandQueue::queue_tail; 
        }
        else
        {
          // next command is at start of buffer
          CommandQueue::queue_head = CommandQueue::queue_buffer; 
        }
        continue;
      }
      
      CommandQueue::in_progress_length = length;
      CommandQueue::total_attempted_queue_command_count += 1;
      continuing = false;
    }
    else
    {
      // continue on existing command
      continuing = true;
    }

#if 0
    DEBUGPGM("HQC: head=");
    DEBUG((int)(CommandQueue::queue_head - CommandQueue::queue_buffer));
    DEBUGPGM(" tail=");
    DEBUG((int)(CommandQueue::queue_tail - CommandQueue::queue_buffer));
    DEBUGPGM(" length=");
    DEBUG((int)length);
    DEBUGPGM(" cont=");
    DEBUGLN((int)continuing);
    DEBUGPGM("eqc=");
    DEBUG(queue_test_enqueue_count);
    DEBUGPGM(" ccc=");
    DEBUG(CommandQueue::current_queue_command_count);
    DEBUGPGM(" tcc=");
    DEBUGLN(CommandQueue::total_attempted_queue_command_count);
#endif    
    
    if (!handleQueueCommand((uint8_t *)CommandQueue::queue_head+1, length, continuing))
      return; // command still in progress

    // queue command is finished
    CommandQueue::current_queue_command_count -= 1;
    CommandQueue::queue_head += length + 1;
    length = 0;
    
    // was it the last command in the queue?
    if (CommandQueue::queue_head == CommandQueue::queue_tail) 
    {
      // handled next time around the loop.
      continue;
    }
    
    // have we reached the end of the ring buffer
    if (CommandQueue::queue_head >= CommandQueue::queue_buffer + CommandQueue::queue_buffer_length)
    {
#if QUEUE_DEBUG
      if (CommandQueue::queue_head > CommandQueue::queue_buffer + CommandQueue::queue_buffer_length)
        ERRORLNPGM("queue ran off the end");
#endif      
      CommandQueue::queue_head = CommandQueue::queue_buffer;
    }
    
    #ifndef AT90USB
    PSERIAL.checkRx(); // Check for serial chars frequently
    #endif
  }
}

bool handleQueueCommand(const uint8_t* command, uint8_t command_length, bool continuing)
{
  int8_t i;
  
  // start command processing
  switch (*command)
  {
  case QUEUE_COMMAND_STRUCTS_TYPE_LINEAR_MOVE:
  {
    return handleLinearMove((const LinearMoveCommand *)command, continuing);    
  }
  case QUEUE_COMMAND_STRUCTS_TYPE_DELAY:
  {
    const DelayQueueCommand * const cmd = (const DelayQueueCommand*)command;
    int32_t delay;

    if (!continuing)
    {
      delay = cmd->delay;
      tmp_uint32 = micros() + delay;
    }
    else
    {
      delay = micros() - tmp_uint32;
    }

    if (delay <= 0)
    {
      return true;
    }
    else
    {
      if (delay > 15000)
        OCR1A = 2 * 15000; // == 30ms
      else
        OCR1A = max(delay * 2, 10L);
      return false;
    }
  }
  case QUEUE_COMMAND_STRUCTS_TYPE_SET_OUTPUT_SWITCH_STATE:
  {
    const SetOutputSwitchStateQueueCommand * const cmd = (const SetOutputSwitchStateQueueCommand*)command;
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
    return true;
  }
  case QUEUE_COMMAND_STRUCTS_TYPE_SET_PWM_OUTPUT_STATE:
  {
    const SetPwmOutputStateQueueCommand * const cmd = (const SetPwmOutputStateQueueCommand*)command;
    Device_PwmOutput::WriteState(cmd->device_number, cmd->value);
    return true;
  }
  case QUEUE_COMMAND_STRUCTS_TYPE_SET_BUZZER_STATE:
  {
    const SetBuzzerStateQueueCommand * const cmd = (const SetBuzzerStateQueueCommand*)command;
    Device_Buzzer::WriteState(cmd->device_number, cmd->value);
    return true;
  }
  case QUEUE_COMMAND_STRUCTS_TYPE_SET_HEATER_TARGET_TEMP:
  {
    const SetHeaterTargetTempCommand * const cmd = (const SetHeaterTargetTempCommand*)command;
    Device_Heater::SetTargetTemperature(cmd->heater_number, cmd->target_temp);
    return true;
  }
  case QUEUE_COMMAND_STRUCTS_TYPE_SET_ACTIVE_TOOLHEAD:
  {
    // do nothing for now.
    return true;
  }
#if QUEUE_TEST
  case QUEUE_TEST_SHORT:
    if (command_length != 1)
    {
      ERRORPGM("Incorrect length in short queue test: ");
      ERRORLN((int)command_length);
    }
    return true;
  case QUEUE_TEST_MEDIUM:
    if (command_length != 2)
    {
      ERRORPGM("Incorrect length in medium queue test: ");
      ERRORLN((int)command_length);
    }
    if (command[1] != queue_test_sequence_number++)
      ERRORLNPGM("Incorrect sequence number in medium queue test");
    return true;
  case QUEUE_TEST_LONG:
    if (command_length < 3)
    {
      ERRORPGM("Incorrect length in long queue test: ");
      ERRORLN((int)command_length);
    }
    if (command[1] != queue_test_sequence_number++)
      ERRORLNPGM("Incorrect sequence number in long queue test");
    if (command[2] != command_length - 3)
      ERRORLNPGM("Incorrect payload length in long queue test");
    return true;
#endif    
  default:
    ERRORPGM("Unknown cmd in ISR: cmd=");
    ERROR((int)*command);
    ERRORPGM(" len="); // RM
    ERROR((int)command_length);
    ERRORPGM(" tcc="); // RM
    ERRORLN(CommandQueue::total_attempted_queue_command_count);
    return true;
  }
}

bool handleLinearMove(const LinearMoveCommand *cmd, bool continuing)
{
#if 0
  static uint8_t move_phase;
  static uint8_t num_axis;
  static AxisMoveInfo *last_axis_info;
  static AxisMoveInfo axis_info;
  
  if (!continuing)
  {
    num_axis = cmd->num_axis;
    OCR1A = cmd->step_time1;
    move_phase = 1;
    step_events_completed = 0;
    total_step_events = cmd-?
    step_loops =
    start_axis_info = &cmd->axis_info[0];
    last_axis_info = &cmd->axis_info[num_axis-1];

    write_directions();
  }

  check_endstops();
  
  for(int8_t i=0; i < step_loops; i++) // Take multiple steps per interrupt (For high speed moves)
  {
    write_steps();
  
    if (++steps_events_completed >= max_step_events)
    {
       // update remaining time
       microseconds_move_remaining -= 
       return true;
    }
  }

  if (steps_events_completed >= step_events_phase2)
  {
  }
  else if (steps_events_completed >= step_events_phase1)
  {
  }
  return false;
  
  recalculate_speed();
  
}  
  
void write_directions()
{
    axis_info = start_axis_info;
    do
    {
      write_direction(axis_info);
    }
    while ((axis_info++ < last_axis_info);   
}     
void check_endstops()
{
    axis_info = start_axis_info;
    do
    {
      write_direction(axis_info);
    }
    while ((axis_info++ < last_axis_info);   
} 
void write_steps()
{
    axis_info = start_axis_info;
    do
    {
      write_step(axis_info);
    }
    while ((axis_info++ < last_axis_info);   
}  

typedef struct _AxisMoveInfo
{
  StepperInfo *stepper_info;
  bool direction;
  uint16_t step_count1;
  uint16_t step_count2;
  uint16_t step_count3;
} AxisMoveInfo;
     
typedef struct _LinearMoveCommand
{
  uint8_t command_type; // QUEUE_COMMAND_STRUCTS_TYPE_LINEAR_MOVE
  uint16_t starting_speed;
  uint16_t nominal_speed;
  uint16_t end_speed;
  uint16_t nominal_time;
  uint8_t primary_axis;
  uint8_t number_of_axes;
  AxisMoveInfo axis_info[1]; // actual length of array is number_of_axes
} LinearMoveCommand;    
  for 
#endif
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