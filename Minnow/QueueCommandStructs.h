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
 
#include "AxisInfo.h"

#include <stdint.h>

#define QUEUE_COMMAND_STRUCTS_TYPE_LINEAR_MOVE 0x01
#define QUEUE_COMMAND_STRUCTS_TYPE_DELAY 0x02
#define QUEUE_COMMAND_STRUCTS_TYPE_SET_OUTPUT_SWITCH_STATE 0x03
#define QUEUE_COMMAND_STRUCTS_TYPE_SET_STEPPER_ENABLE_STATE 0x04
#define QUEUE_COMMAND_STRUCTS_TYPE_SET_ENDSTOP_ENABLE_STATE 0x05
#define QUEUE_COMMAND_STRUCTS_TYPE_SET_PWM_OUTPUT_STATE 0x06
#define QUEUE_COMMAND_STRUCTS_TYPE_SET_BUZZER_STATE 0x07
#define QUEUE_COMMAND_STRUCTS_TYPE_SET_HEATER_TARGET_TEMP 0x08
#define QUEUE_COMMAND_STRUCTS_TYPE_SET_ACTIVE_TOOLHEAD 0x09

struct DelayQueueCommand
{
  uint8_t command_type; // == QUEUE_COMMAND_STRUCTS_TYPE_DELAY
  uint32_t delay; // in us units
};      

struct DeviceBitState
{
  uint8_t device_number;
  uint8_t *device_reg;
  uint8_t device_bit;
  uint8_t device_state;
};      

struct SetOutputSwitchStateQueueCommand
{
  uint8_t command_type; // QUEUE_COMMAND_STRUCTS_TYPE_SET_OUTPUT_SWITCH_STATE
  uint8_t num_bits;
  DeviceBitState output_bit_info[1]; // array length == num_bits in practice
};      

struct SetStepperEnableStateQueueCommand
{
  uint8_t command_type; // QUEUE_COMMAND_STRUCTS_TYPE_SET_STEPPER_ENABLE_STATE
  uint8_t stepper_number; // or 0xFF to disable all steppers
  uint8_t stepper_state; // 1 = enable, 2 = disable
};      

struct SetEndstopEnableStateQueueCommand
{
  uint8_t command_type; // QUEUE_COMMAND_STRUCTS_TYPE_SET_ENDSTOP_ENABLE_STATE
  BITMASK(MAX_ENDSTOPS) endstops_to_change;
  BITMASK(MAX_ENDSTOPS) endstop_enable_state;
};      

struct SetPwmOutputStateQueueCommand
{
  uint8_t command_type; // QUEUE_COMMAND_STRUCTS_TYPE_SET_PWM_OUTPUT_STATE
  uint8_t device_number;
  uint8_t value;
};      
     
struct SetBuzzerStateQueueCommand
{
  uint8_t command_type; // QUEUE_COMMAND_STRUCTS_TYPE_SET_PWM_OUTPUT_STATE
  uint8_t device_number;
  uint8_t value;
};      

struct SetHeaterTargetTempCommand
{
  uint8_t command_type; // QUEUE_COMMAND_STRUCTS_TYPE_SET_HEATER_TARGET_TEMP
  uint8_t heater_number;
  float target_temp;
};

struct SetActiveToolheadCommand
{
  uint8_t command_type; // QUEUE_COMMAND_STRUCTS_TYPE_SET_ACTIVE_TOOLHEAD
  uint8_t toolhead_number;
};
     
struct AxisMoveInfo
{
  AxisInfoInternal *axis_info;
  uint16_t step_count;
};
     
struct LinearMoveCommand
{
  uint8_t command_type; // QUEUE_COMMAND_STRUCTS_TYPE_LINEAR_MOVE
  uint8_t num_axes;
  uint8_t primary_axis; // as axis_info index
  uint16_t total_steps;
  uint16_t steps_phase_2; // steps remaining to enter phase 2
  uint16_t steps_phase_3; // steps remaining to enter phase 2
  uint16_t nominal_rate; // in steps per second
  uint16_t final_rate;
  uint32_t acceleration_rate; // in steps per second per second
  uint32_t deceleration_rate; // as positive value
  uint16_t nominal_block_time; // in 10ths of ms (don't worry about overflow) - its for underrun protection
  uint16_t steps_to_final_speed_from_underrun_rate;
  bool homing_bit;
  BITMASK(MAX_ENDSTOPS) endstops_of_interest;
  BITMASK(MAX_STEPPERS) directions; // bitmask by axis_info index
  AxisMoveInfo axis_move_info[1]; // actual length of array is number_of_axes
};    

     
#endif