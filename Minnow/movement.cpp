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

#include "movement.h"
#include "CommandQueue.h"
#include "QueueCommandStructs.h"
#include "Minnow.h"
#include "Device_OutputSwitch.h"
#include "Device_Heater.h"

// Some useful constants

#define ENABLE_STEPPER_DRIVER_INTERRUPT()  TIMSK1 |= (1<<OCIE1A)
#define DISABLE_STEPPER_DRIVER_INTERRUPT() TIMSK1 &= ~(1<<OCIE1A)

// queue statics placed in this compilation unit to allow better optimization
uint8_t *CommandQueue::queue_buffer;
uint16_t CommandQueue::queue_buffer_length;
  
volatile uint8_t *CommandQueue::queue_head;
volatile uint8_t *CommandQueue::queue_tail; // ISR will never change this value

volatile uint8_t CommandQueue::in_progress_length;
volatile uint16_t CommandQueue::current_queue_command_count;
volatile uint16_t CommandQueue::total_attempted_queue_command_count;

// Function declarations
FORCE_INLINE void movement_ISR(); // needs to be non-static due to friend usage elsewhere
FORCE_INLINE bool handleQueueCommand(const uint8_t* command, uint8_t command_length, bool continuing); // needs to be non-static due to friend usage elsewhere

// Temporary variables used to store command state between ISR invocations

static uint32_t tmp_uint32;

#ifdef QUEUE_DEBUG
#define QUEUE_DEBUG_SHORT 0xF0
#define QUEUE_DEBUG_MEDIUM 0xF1
#define QUEUE_DEBUG_LONG 0xF2
uint8_t queue_debug_sequence_number = 0;
uint16_t queue_debug_enqueue_count = 0;
#endif

//
// Routines
//

void mv_init() 
{
  
}

void mv_wake_up() 
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
#ifdef QUEUE_DEBUG
        if (CommandQueue::current_queue_command_count != 0)
          ERRORPGMLN("queue count wrong 1");
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
#ifdef QUEUE_DEBUG
          if (CommandQueue::queue_tail != CommandQueue::queue_buffer + CommandQueue::queue_buffer_length)
          ERRORPGMLN("unexpected tail position");
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
    DEBUG(queue_debug_enqueue_count);
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
#ifdef QUEUE_DEBUG
      if (CommandQueue::queue_head > CommandQueue::queue_buffer + CommandQueue::queue_buffer_length)
        ERRORPGMLN("queue ran off the end");
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
          *output_reg = *output_reg & !device_info->device_bit; 
        else
          *output_reg = *output_reg | device_info->device_bit;
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
    analogWrite(cmd->pin, cmd->value);
    return true;
  }
  case QUEUE_COMMAND_STRUCTS_TYPE_SET_HEATER_TARGET_TEMP:
  {
    const SetHeaterTargetTempCommand * const cmd = (const SetHeaterTargetTempCommand*)command;
    Device_Heater::SetTargetTemperature(cmd->heater_number, cmd->target_temp);
    return true;
  }
#ifdef QUEUE_DEBUG
  case QUEUE_DEBUG_SHORT:
    if (command_length != 1)
    {
      ERRORPGM("Incorrect length in short queue test: ");
      ERRORLN((int)command_length);
    }
    return true;
  case QUEUE_DEBUG_MEDIUM:
    if (command_length != 2)
    {
      ERRORPGM("Incorrect length in medium queue test: ");
      ERRORLN((int)command_length);
    }
    if (command[1] != queue_debug_sequence_number++)
      ERRORPGMLN("Incorrect sequence number in medium queue test");
    return true;
  case QUEUE_DEBUG_LONG:
    if (command_length < 3)
    {
      ERRORPGM("Incorrect length in long queue test: ");
      ERRORLN((int)command_length);
    }
    if (command[1] != queue_debug_sequence_number++)
      ERRORPGMLN("Incorrect sequence number in long queue test");
    if (command[2] != command_length - 3)
      ERRORPGMLN("Incorrect payload length in long queue test");
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

#ifdef QUEUE_DEBUG
void run_queue_test()
{
  uint8_t buffer[50];
  CommandQueue::Init(buffer, sizeof(buffer));
  
  uint16_t remaining_slots;
  bool is_in_progress;
  uint16_t current_command_count;
  uint16_t total_command_count;
  uint8_t *ptr;
  int i;

  DEBUGPGMLN("Begin test");  
  
  for (i = 0; i < 1000; i++)
  {
    while ((ptr = CommandQueue::GetCommandInsertionPoint(1)) == 0)
      ;
    *ptr = QUEUE_DEBUG_SHORT;
    if (!CommandQueue::EnqueueCommand(1))
      ERRORPGMLN("Failed to short enqueue");
    
    if (i % 59 == 0)
    {
      while ((ptr = CommandQueue::GetCommandInsertionPoint(5)) == 0)
        ;
      memset(ptr, 0, 5);
      ptr[0] = QUEUE_COMMAND_STRUCTS_TYPE_DELAY;
      ptr[4] = 1 + (i % 13) ;
      if (!CommandQueue::EnqueueCommand(5))
        ERRORPGMLN("Failed to short enqueue delay");
    }
  }

  CommandQueue::GetQueueInfo(remaining_slots, is_in_progress, current_command_count, total_command_count);
  DEBUGPGM("Info1a: slots=");
  DEBUG(remaining_slots);
  DEBUGPGM(" ip=");
  DEBUG((int)is_in_progress);
  DEBUGPGM(" ccc=");
  DEBUG(current_command_count);
  DEBUGPGM(" tcc=");
  DEBUGLN(total_command_count);

  delay(1000);

  CommandQueue::GetQueueInfo(remaining_slots, is_in_progress, current_command_count, total_command_count);
  DEBUGPGM("Info1b: slots=");
  DEBUG(remaining_slots);
  DEBUGPGM(" ip=");
  DEBUG((int)is_in_progress);
  DEBUGPGM(" ccc=");
  DEBUG(current_command_count);
  DEBUGPGM(" tcc=");
  DEBUGLN(total_command_count);

  uint8_t sequence_num = 0;
  for (i = 0; i < 1000; i++)
  {
    while ((ptr = CommandQueue::GetCommandInsertionPoint(2)) == 0)
      ;
    *ptr++ = QUEUE_DEBUG_MEDIUM;
    *ptr = sequence_num++;
    if (!CommandQueue::EnqueueCommand(2))
      ERRORPGMLN("Failed to medium enqueue");

    if (i % 59 == 0)
    {
      while ((ptr = CommandQueue::GetCommandInsertionPoint(5)) == 0)
        ;
      memset(ptr, 0, 5);
      ptr[0] = QUEUE_COMMAND_STRUCTS_TYPE_DELAY;
      ptr[4] = 1 + (i % 13) ;
      if (!CommandQueue::EnqueueCommand(5))
        ERRORPGMLN("Failed to short enqueue delay");
    }
  }
  
  CommandQueue::GetQueueInfo(remaining_slots, is_in_progress, current_command_count, total_command_count);
  DEBUGPGM("Info2a: slots=");
  DEBUG(remaining_slots);
  DEBUGPGM(" ip=");
  DEBUG((int)is_in_progress);
  DEBUGPGM(" ccc=");
  DEBUG(current_command_count);
  DEBUGPGM(" tcc=");
  DEBUGLN(total_command_count);

  delay(1000);

  CommandQueue::GetQueueInfo(remaining_slots, is_in_progress, current_command_count, total_command_count);
  DEBUGPGM("Info2b: slots=");
  DEBUG(remaining_slots);
  DEBUGPGM(" ip=");
  DEBUG((int)is_in_progress);
  DEBUGPGM(" ccc=");
  DEBUG(current_command_count);
  DEBUGPGM(" tcc=");
  DEBUGLN(total_command_count);

#if 1
  for (int i = 0; i < 1005; i++)
  {
    while ((ptr = CommandQueue::GetCommandInsertionPoint(3+(i%30))) == 0)
      ;
    *ptr++ = QUEUE_DEBUG_LONG;
    *ptr++ = sequence_num++;
    *ptr++ = i%30;
    if (!CommandQueue::EnqueueCommand(3+(i%30)))
      ERRORPGMLN("Failed to long enqueue");

    if (i % 59 == 0)
    {
      while ((ptr = CommandQueue::GetCommandInsertionPoint(5)) == 0)
        ;
      memset(ptr, 0, 5);
      ptr[0] = QUEUE_COMMAND_STRUCTS_TYPE_DELAY;
      ptr[4] = 1 + (i % 13) ;
      if (!CommandQueue::EnqueueCommand(5))
        ERRORPGMLN("Failed to short enqueue delay");
    }
  }
    
  CommandQueue::GetQueueInfo(remaining_slots, is_in_progress, current_command_count, total_command_count);
  DEBUGPGM("Info3a: slots=");
  DEBUG(remaining_slots);
  DEBUGPGM(" ip=");
  DEBUG((int)is_in_progress);
  DEBUGPGM(" ccc=");
  DEBUG(current_command_count);
  DEBUGPGM(" tcc=");
  DEBUGLN(total_command_count);

  delay(1000);

  CommandQueue::GetQueueInfo(remaining_slots, is_in_progress, current_command_count, total_command_count);
  DEBUGPGM("Info3b: slots=");
  DEBUG(remaining_slots);
  DEBUGPGM(" ip=");
  DEBUG((int)is_in_progress);
  DEBUGPGM(" ccc=");
  DEBUG((int)current_command_count);
  DEBUGPGM(" tcc=");
  DEBUGLN(total_command_count);
#endif  
}
#endif