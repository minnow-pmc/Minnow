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

//
//
// This file contains the command structures which are inserted into the CommandQueue
//
//
 
#ifndef QUEUE_COMMAND_STRUCTS_H 
#define QUEUE_COMMAND_STRUCTS_H
 
#include <stdint.h>

#define QUEUE_COMMAND_STRUCTS_TYPE_SKIP 0x00
#define QUEUE_COMMAND_STRUCTS_TYPE_LINEAR_MOVE 0x01
#define QUEUE_COMMAND_STRUCTS_TYPE_DELAY 0x02
#define QUEUE_COMMAND_STRUCTS_TYPE_SET_OUTPUT_SWITCH_STATE 0x03
#define QUEUE_COMMAND_STRUCTS_TYPE_SET_PWM_OUTPUT_STATE 0x04
#define QUEUE_COMMAND_STRUCTS_TYPE_SET_BUZZER_STATE 0x05
#define QUEUE_COMMAND_STRUCTS_TYPE_SET_HEATER_TARGET_TEMP 0x06
#define QUEUE_COMMAND_STRUCTS_TYPE_SET_ACTIVE_TOOLHEAD 0x07

typedef struct _DelayQueueCommand
{
  uint8_t command_type; // == QUEUE_COMMAND_STRUCTS_TYPE_DELAY
  uint32_t delay; // in us units
} DelayQueueCommand;      

typedef struct _DeviceBitState
{
  uint8_t device_number;
  uint8_t *device_reg;
  uint8_t device_bit;
  uint8_t device_state;
} DeviceBitState;      

typedef struct _SetOutputSwitchStateQueueCommand
{
  uint8_t command_type; // QUEUE_COMMAND_STRUCTS_TYPE_SET_OUTPUT_SWITCH_STATE
  uint8_t num_bits;
  DeviceBitState output_bit_info[1]; // array length == num_bits in practice
} SetOutputSwitchStateQueueCommand;      


typedef struct _SetPwmOutputStateQueueCommand
{
  uint8_t command_type; // QUEUE_COMMAND_STRUCTS_TYPE_SET_PWM_OUTPUT_STATE
  uint8_t device_number;
  uint8_t value;
} SetPwmOutputStateQueueCommand;      
     
typedef struct _SetBuzzerStateQueueCommand
{
  uint8_t command_type; // QUEUE_COMMAND_STRUCTS_TYPE_SET_PWM_OUTPUT_STATE
  uint8_t device_number;
  uint8_t value;
} SetBuzzerStateQueueCommand;      

typedef struct _SetHeaterTargetTempCommand
{
  uint8_t command_type; // QUEUE_COMMAND_STRUCTS_TYPE_SET_HEATER_TARGET_TEMP
  uint8_t heater_number;
  int16_t target_temp;
} SetHeaterTargetTempCommand;

typedef struct _SetActiveToolheadCommand
{
  uint8_t command_type; // QUEUE_COMMAND_STRUCTS_TYPE_SET_ACTIVE_TOOLHEAD
  uint8_t toolhead_number;
} SetActiveToolheadCommand;
     
typedef struct _AxisMoveInfo
{
//  StepperInfo *stepper_info;
  int8_t direction; // == -1 or +1
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
  uint8_t initial_loops;
  uint8_t initial_rate;
  uint8_t primary_axis;
  uint8_t number_of_axes;
  AxisMoveInfo axis_info[1]; // actual length of array is number_of_axes
} LinearMoveCommand;    

     
#endif