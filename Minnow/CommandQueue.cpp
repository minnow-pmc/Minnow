/*
 Minnow Pacemaker client firmware.
    
 Copyright (C) 2013 Robert Fairlie-Cuninghame

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CommandQueue.h"

#include "QueueCommandStructs.h"
#include "Minnow.h"
#include "movement_ISR.h"

// queue statics placed in the movement.cpp compilation unit to allow better ISR optimization
  
void 
CommandQueue::Init(uint8_t *buffer, uint16_t buffer_length)
{
  CRITICAL_SECTION_START
  queue_buffer = buffer;
  queue_buffer_length = buffer_length;
  
  queue_head = queue_buffer;
  queue_tail = queue_buffer;
  in_progress_length = 0;
  
  current_queue_command_count = 0;
  total_attempted_queue_command_count = 0;
  CRITICAL_SECTION_END
}
  
uint8_t *
CommandQueue::GetCommandInsertionPoint(uint8_t length_required)
{
  const uint8_t *cached_queue_head; 
  CRITICAL_SECTION_START
  cached_queue_head = (const uint8_t *)queue_head;
  if (cached_queue_head == queue_tail)
  {
    queue_head = queue_buffer;
    cached_queue_head = queue_buffer;
    queue_tail = queue_buffer;
  }
  CRITICAL_SECTION_END
  
  if (cached_queue_head <= queue_tail)
  {
    // does command fit before end of buffer?
    if ((queue_buffer + queue_buffer_length) - queue_tail > length_required) 
      return (uint8_t *)queue_tail+1;
    // otherwise does command fit after start of buffer?
    if (cached_queue_head - queue_buffer <= length_required + 1)
      return 0;
    // yes it fits but we need to skip remaining space at end of buffer
    if (queue_tail < queue_buffer + queue_buffer_length)
    {
      *queue_tail = 0;
      CRITICAL_SECTION_START
      queue_tail = queue_buffer + queue_buffer_length; 
      CRITICAL_SECTION_END
    }
    return queue_buffer+1;
  }
  else
  {
    // will it fit at all?
    if (cached_queue_head - queue_tail <= length_required + 1)
      return 0;
    return (uint8_t *)queue_tail+1;
  }
}

bool 
CommandQueue::EnqueueCommand(uint8_t command_length)
{
  bool success;
  CRITICAL_SECTION_START
  if (queue_head <= queue_tail)
  {
    if (queue_tail < queue_buffer + queue_buffer_length)
      success = queue_tail + command_length < queue_buffer + queue_buffer_length;
    else
      success = queue_buffer + command_length + 1 < queue_head;
  }
  else
  {
    success = queue_tail + command_length + 1 < queue_head;
  }
    
  if (success)
  {
    if (queue_tail >= queue_buffer + queue_buffer_length)
    {
      queue_buffer[0] = command_length;
      queue_tail = queue_buffer + command_length + 1;
    }
    else
    {
      queue_tail[0] = command_length;
      queue_tail += command_length + 1; 
    }
    current_queue_command_count += 1;

#if QUEUE_TEST
    extern uint16_t queue_test_enqueue_count;
    queue_test_enqueue_count += 1;
#endif    

    if (in_progress_length == 0)
      movement_ISR_wake_up();
  }  
  CRITICAL_SECTION_END
  if (!success)
  {
    DEBUGLNPGM("Bad length for EnqueueCommand"); // shouldn't happen if GetCommandInsertionPoint was called correctly
  }    
  return success;
}

void 
CommandQueue::FlushQueuedCommands()
{
  CRITICAL_SECTION_START
  if (in_progress_length != 0)
  {
    queue_tail = queue_head + in_progress_length + 1;
    current_queue_command_count = 1;
  }
  else
  {
    queue_head = 0;
    current_queue_command_count = 0;
  }
  CRITICAL_SECTION_END
}
  
void CommandQueue::GetQueueInfo(uint16_t &remaining_slots, 
                                uint16_t &current_command_count, 
                                uint16_t &total_executed_queue_command_count)
{
  CRITICAL_SECTION_START
  current_command_count = current_queue_command_count;
  total_executed_queue_command_count = total_attempted_queue_command_count;
  const uint8_t * const cached_queue_head = (const uint8_t *)queue_head; 
  CRITICAL_SECTION_END
  if (cached_queue_head == queue_tail)
  {
    remaining_slots = queue_buffer_length / QUEUE_SLOT_SIZE;
  }
  else if (cached_queue_head < queue_tail)
  {
    remaining_slots = ((cached_queue_head - queue_buffer) 
                            + ((queue_buffer + queue_buffer_length) - queue_tail))
                        / QUEUE_SLOT_SIZE;
  }
  else
  {
    remaining_slots = (cached_queue_head - queue_tail) / QUEUE_SLOT_SIZE;
  }
}
  


