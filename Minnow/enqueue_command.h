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

#ifndef QUEUE_COMMAND_BUILDER_H 
#define QUEUE_COMMAND_BUILDER_H
 
#define ENQUEUE_SUCCESS                             0x0
#define ENQUEUE_ERROR_QUEUE_FULL                    0xFE
#define ENQUEUE_UNSPECIFIED_FAILURE                 0xFF

#define QUEUE_ERROR_MSG_OFFSET      9 // bytes before error reason starts: 1+1+2+2+2+1
 
void enqueue_command();
     
#endif