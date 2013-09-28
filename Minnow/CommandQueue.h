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

#ifndef COMMAND_QUEUE_H 
#define COMMAND_QUEUE_H
 
#include "config.h"
#include <stdint.h>

//
// Command Queue class
//
// Implements queuing of commands. The stepper ISR is responsible for removing 
// commands. 
//
// Maybe TODO: A possible improvement in the future is have two types of queue. The first (larger) 
// queue holds the compact PaceMaker commands (straight off the wire) - which allows a much 
// larger number of commands to be held; the second is a reduced version of the current
// queue which is in the expanded ISR ready format. The main loop would ensure that commands
// transferred from the first queue to the second before the ISR is idle. 
// This has no affect on the throughput but does mean more commands could be queueud for the
// same amount of memory.
// 
class CommandQueue
{
public:

  #define QUEUE_SLOT_SIZE 20 // TODO put in real value here

  static void Init(uint8_t *queue_buffer, uint16_t queue_buffer_len);
  
  static uint8_t *GetCommandInsertionPoint(uint8_t length_required);
  static bool EnqueueCommand(uint8_t command_length);
  static void FlushQueuedCommands();
  
  static bool IsCommandExecuting() { return in_progress_length != 0; } // don't need critical section for single byte read
  static void GetQueueInfo(uint16_t &remaining_slots, uint16_t &current_command_count, uint16_t &total_executed_queue_command_count);
  
  static uint16_t GetQueueBufferLength() { return queue_buffer_length; }

private: 
  
  friend void movement_ISR();
  friend bool handle_queue_command();
  friend bool check_underrun_condition();
  friend void dump_movement_queue();

  static uint8_t *queue_buffer;
  static uint16_t queue_buffer_length;
  
  static volatile uint8_t *queue_head;
  static volatile uint8_t *queue_tail;
  
  static volatile uint8_t in_progress_length;
  static volatile uint16_t current_queue_command_count;
  static volatile uint16_t total_attempted_queue_command_count;
};

#endif