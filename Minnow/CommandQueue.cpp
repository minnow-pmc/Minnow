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

uint8_t *CommandQueue::queue_buffer;
uint16_t CommandQueue::queue_buffer_length;
  
uint8_t *CommandQueue::queue_head;
uint8_t *CommandQueue::queue_tail;
uint8_t CommandQueue::in_progress_length;
  
void 
CommandQueue::Init(uint8_t *buffer, uint16_t buffer_length)
{
  queue_buffer = buffer;
  queue_buffer_length = buffer_length;
  
  queue_head = &buffer[0];
  queue_tail = &buffer[0];
  in_progress_length = 0;
}
  
