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

#include "enqueue_command.h"
#include "QueueCommandStructs.h"

#include "Minnow.h"
#include "response.h"
#include "CommandQueue.h"

#include "Device_OutputSwitch.h"
#include "Device_PwmOutput.h"
#include "Device_Buzzer.h"
#include "Device_Heater.h"
#include "Device_InputSwitch.h"
#include "AxisInfo.h"


static void send_enqueue_error(uint8_t error_type, uint8_t block_index, uint8_t reply_error_code = 0xFF);
static uint8_t generate_enqueue_insufficient_bytes_error(uint8_t expected_num_bytes, uint8_t rcvd_num_bytes);

FORCE_INLINE static uint8_t enqueue_delay_command(const uint8_t *pacemaker_command, uint8_t pacemaker_command_length);
FORCE_INLINE static uint8_t enqueue_set_output_switch_state_command(const uint8_t *pacemaker_command, uint8_t pacemaker_command_length);
FORCE_INLINE static uint8_t enqueue_set_pwm_output_state_command(const uint8_t *pacemaker_command, uint8_t pacemaker_command_length);
FORCE_INLINE static uint8_t enqueue_set_heater_target_temperature_command(const uint8_t *pacemaker_command, uint8_t pacemaker_command_length);
FORCE_INLINE static uint8_t enqueue_set_stepper_enable_state_command(const uint8_t *pacemaker_command, uint8_t pacemaker_command_length);
FORCE_INLINE static uint8_t enqueue_set_endstop_enable_state_command(const uint8_t *pacemaker_command, uint8_t pacemaker_command_length);
uint8_t validate_linear_move(const uint8_t *parameter, uint8_t parameter_length);

uint16_t last_enqueued_final_speed;

//
// Top level enqueue command handler
//
void enqueue_command()
{
  uint8_t *ptr = parameter_value;
  uint8_t index = 0;

  if (CommandQueue::GetQueueBufferLength() == 0)
  {
    if (!allocate_command_queue_memory())
    {
      send_app_error_response(PARAM_APP_ERROR_TYPE_FIRMWARE_ERROR, 
                              PMSG(MSG_ERR_INSUFFICIENT_MEMORY));
      return;
    }
  }  
  
  generate_response_start(RSP_ORDER_SPECIFIC_ERROR, QUEUE_ERROR_MSG_OFFSET);
  
  while (ptr < parameter_value + parameter_length)
  {
    const uint8_t length = *ptr++;
    // 2 is currently minimum length of defined command blocks
    if (length < 2 || ptr + length < parameter_value + parameter_length)
    {
      generate_response_msg_addPGM(PMSG(ERR_MSG_INSUFFICENT_BYTES));
      generate_response_msg_addbyte((parameter_value + parameter_length) - (ptr + length));
      generate_response_msg_add(", ");
      generate_response_msg_addPGM(PMSG(MSG_EXPECTING));
      generate_response_msg_addbyte(parameter_length + length); 
      send_enqueue_error(QUEUE_COMMAND_ERROR_TYPE_MALFORMED_BLOCK, index);
      return;
    }
    
    uint8_t retval;
    switch (*ptr++)
    {
    case QUEUE_COMMAND_DELAY:
      retval = enqueue_delay_command(ptr, length-1);
      break;
    
    case QUEUE_COMMAND_ORDER_WRAPPER:
    {
      switch (*ptr++)
      {
      case ORDER_SET_OUTPUT_SWITCH_STATE:
        retval = enqueue_set_output_switch_state_command(ptr, length-2);
        break;
      case ORDER_SET_PWM_OUTPUT_STATE:
        retval = enqueue_set_pwm_output_state_command(ptr, length-2);
        break;
      case ORDER_SET_HEATER_TARGET_TEMP:
        retval = enqueue_set_heater_target_temperature_command(ptr, length-2);
        break;
      default:
        generate_response_msg_addPGM(PMSG(ERR_MSG_QUEUE_ORDER_NOT_PERMITTED));
        char code[4];
        utoa(*(ptr-1), code, 10);
        generate_response_msg_add(code);
        send_enqueue_error(QUEUE_COMMAND_ERROR_TYPE_ERROR_IN_COMMAND_BLOCK, index, PARAM_APP_ERROR_TYPE_UNKNOWN_ORDER);
        return;
      }
    }
    default:
      send_enqueue_error(QUEUE_COMMAND_ERROR_TYPE_UNKNOWN_COMMAND_BLOCK, index);
      return;
    }
    if (retval != ENQUEUE_SUCCESS)
    {
      send_enqueue_error(QUEUE_COMMAND_ERROR_TYPE_ERROR_IN_COMMAND_BLOCK, index, retval);
      return;
    }
    ptr += length;
    index += 1;
  }
  
  uint16_t remaining_slots;
  uint16_t current_command_count;
  uint16_t total_command_count;
  CommandQueue::GetQueueInfo(remaining_slots, current_command_count, total_command_count);
  
  generate_response_start(RSP_OK);
  generate_response_data_add(remaining_slots);
  generate_response_data_add(current_command_count);
  generate_response_data_add(total_command_count);
  generate_response_send(); 
}
 
void send_enqueue_error(uint8_t error_type, uint8_t block_index, uint8_t error_code)
{
  uint16_t remaining_slots;
  uint16_t current_command_count;
  uint16_t total_command_count;
  CommandQueue::GetQueueInfo(remaining_slots, current_command_count, total_command_count);
 
  generate_response_data_addbyte(error_type);
  generate_response_data_addbyte(block_index);
  generate_response_data_add(remaining_slots);
  generate_response_data_add(current_command_count);
  generate_response_data_add(total_command_count);
  generate_response_data_addbyte(error_code);
  generate_response_send();
} 

uint8_t generate_enqueue_insufficient_bytes_error(uint8_t expected_num_bytes, uint8_t rcvd_num_bytes)
{
  generate_response_msg_addPGM(PMSG(ERR_MSG_INSUFFICENT_BYTES));
  generate_response_msg_addbyte(rcvd_num_bytes);
  generate_response_msg_add(", ");
  generate_response_msg_addPGM(PMSG(MSG_EXPECTING));
  generate_response_msg_addbyte(expected_num_bytes);
  return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_FORMAT; 
}
 
FORCE_INLINE uint8_t enqueue_delay_command(const uint8_t *parameter, uint8_t parameter_length)
{
  if (parameter_length != sizeof(uint16_t))
    return generate_enqueue_insufficient_bytes_error(sizeof(uint16_t), parameter_length);

  uint8_t *insertion_point = CommandQueue::GetCommandInsertionPoint(sizeof(DelayQueueCommand));
  
  if (insertion_point == 0)
    return ENQUEUE_ERROR_QUEUE_FULL;
    
  DelayQueueCommand *cmd = (DelayQueueCommand *)insertion_point;
    
  cmd->command_type = QUEUE_COMMAND_STRUCTS_TYPE_DELAY;
  cmd->delay = ((parameter[0] << 8) | parameter[1]) * 10; // use us for delay 
  
  CommandQueue::EnqueueCommand(sizeof(DelayQueueCommand));
  return ENQUEUE_SUCCESS;
}

FORCE_INLINE uint8_t enqueue_set_output_switch_state_command(const uint8_t *parameter, uint8_t parameter_length)
{
  const uint8_t number_of_switches = parameter_length/3;

  if ((number_of_switches * 3) != parameter_length)
    return generate_enqueue_insufficient_bytes_error(3*(number_of_switches+1), parameter_length);
  
  const uint8_t length = sizeof(SetOutputSwitchStateQueueCommand) + 
                            ((number_of_switches - 1) * sizeof(DeviceBitState));
  uint8_t *insertion_point = CommandQueue::GetCommandInsertionPoint(length);
  
  if (insertion_point == 0)
    return ENQUEUE_ERROR_QUEUE_FULL;
  
  SetOutputSwitchStateQueueCommand *cmd = (SetOutputSwitchStateQueueCommand *)insertion_point;
    
  cmd->command_type = QUEUE_COMMAND_STRUCTS_TYPE_SET_OUTPUT_SWITCH_STATE;
  cmd->num_bits = number_of_switches;
  
  uint8_t device_number, device_state;

  for (uint8_t i = 0; i < parameter_length; i+=3)
  {
    device_number = parameter_value[i+1];
    device_state = parameter_value[i+2];
    
    switch(parameter_value[i])
    {
    case PM_DEVICE_TYPE_SWITCH_OUTPUT:
    {
      if (!Device_OutputSwitch::IsInUse(device_number))
        return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;

      // assume that Device_OutputSwitch::SetPin checks that pin maps to valid port
      const uint8_t pin = Device_OutputSwitch::GetPin(device_number);
      
      DeviceBitState *output_state = (DeviceBitState *)cmd->output_bit_info + i;
      
        // could further optimize this by including the actual address of the output/mode register
        // but this is sufficient for now
      output_state->device_number = device_number;
      output_state->device_reg = (uint8_t *)portOutputRegister(digitalPinToPort(pin));
      output_state->device_bit = digitalPinToBitMask(pin);
      output_state->device_state = device_state;
    }    
    default:
      return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_TYPE;
    }
  }  
  
  CommandQueue::EnqueueCommand(length);
  return ENQUEUE_SUCCESS;
}

FORCE_INLINE uint8_t enqueue_set_pwm_output_state_command(const uint8_t *parameter, uint8_t parameter_length)
{
  if (parameter_length < 4)
    return generate_enqueue_insufficient_bytes_error(4, parameter_length);
  
  uint8_t *insertion_point = CommandQueue::GetCommandInsertionPoint(sizeof(SetPwmOutputStateQueueCommand));
  
  if (insertion_point == 0)
    return ENQUEUE_ERROR_QUEUE_FULL;
     
  const uint8_t device_number = parameter_value[1];
  
  switch(parameter_value[0])
  {
  case PM_DEVICE_TYPE_PWM_OUTPUT:
  {
    if (!Device_PwmOutput::IsInUse(device_number))
      return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;

    SetPwmOutputStateQueueCommand *cmd = (SetPwmOutputStateQueueCommand *)insertion_point;
    cmd->command_type = QUEUE_COMMAND_STRUCTS_TYPE_SET_PWM_OUTPUT_STATE;
    cmd->device_number = device_number;
    cmd->value = parameter_value[2]; // the LSB is ignored
    CommandQueue::EnqueueCommand(sizeof(SetPwmOutputStateQueueCommand));
    return ENQUEUE_SUCCESS;
  }    
  case PM_DEVICE_TYPE_BUZZER:
  {
    if (!Device_Buzzer::IsInUse(device_number))
      return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;

    SetBuzzerStateQueueCommand *cmd = (SetBuzzerStateQueueCommand *)insertion_point;
    cmd->command_type = QUEUE_COMMAND_STRUCTS_TYPE_SET_BUZZER_STATE;
    cmd->device_number = device_number;
    cmd->value = parameter_value[2]; // the LSB is ignored
    CommandQueue::EnqueueCommand(sizeof(SetPwmOutputStateQueueCommand));
    return ENQUEUE_SUCCESS;
  }    
  default:
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_TYPE;
  }  
}

FORCE_INLINE uint8_t enqueue_set_heater_target_temperature_command(const uint8_t *parameter, uint8_t parameter_length)
{
  if (parameter_length < 3)
    return generate_enqueue_insufficient_bytes_error(3, parameter_length);
  
  uint8_t *insertion_point = CommandQueue::GetCommandInsertionPoint(sizeof(SetHeaterTargetTempCommand));
  
  if (insertion_point == 0)
    return ENQUEUE_ERROR_QUEUE_FULL;
  
  uint8_t heater_number = parameter_value[0];
  int16_t target_temp = (parameter_value[1] << 8) | parameter_value[2];
  float target_ftemp = (float)target_temp / 10;
  
  uint8_t retval = Device_Heater::ValidateTargetTemperature(heater_number, target_ftemp);
  if (retval != APP_ERROR_TYPE_SUCCESS)
    return retval;

  SetHeaterTargetTempCommand *cmd = (SetHeaterTargetTempCommand *)insertion_point;
  cmd->command_type = QUEUE_COMMAND_STRUCTS_TYPE_SET_HEATER_TARGET_TEMP;
  cmd->heater_number = parameter_value[0];
  cmd->target_temp = target_ftemp;

  CommandQueue::EnqueueCommand(sizeof(SetHeaterTargetTempCommand));
  return ENQUEUE_SUCCESS;
}

FORCE_INLINE uint8_t enqueue_set_stepper_enable_state_command(const uint8_t *pacemaker_command, uint8_t pacemaker_command_length)
{
  if (parameter_length != 0 && parameter_length != 2)
    return generate_enqueue_insufficient_bytes_error(2, parameter_length);
  
  uint8_t *insertion_point = CommandQueue::GetCommandInsertionPoint(sizeof(SetStepperEnableStateQueueCommand));
  
  if (insertion_point == 0)
    return ENQUEUE_ERROR_QUEUE_FULL;
  
  SetStepperEnableStateQueueCommand *cmd = (SetStepperEnableStateQueueCommand *)insertion_point;
    
  cmd->command_type = QUEUE_COMMAND_STRUCTS_TYPE_SET_STEPPER_ENABLE_STATE;
  if (parameter_length == 0)
  {
    cmd->stepper_number = 0xFF;
    cmd->stepper_state = 0;
  }
  else
  {
    if (!AxisInfo::IsInUse(pacemaker_command[0]))
      return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;
    cmd->stepper_number = pacemaker_command[0];
    cmd->stepper_state = 0;
  }

  CommandQueue::EnqueueCommand(sizeof(SetStepperEnableStateQueueCommand));
  return ENQUEUE_SUCCESS;
}

FORCE_INLINE static uint8_t enqueue_set_endstop_enable_state_command(const uint8_t *pacemaker_command, uint8_t pacemaker_command_length)
{
  if ((parameter_length & 1) != 0)
    return generate_enqueue_insufficient_bytes_error(parameter_length+1, parameter_length);
  
  uint8_t *insertion_point = CommandQueue::GetCommandInsertionPoint(sizeof(SetEndstopEnableStateQueueCommand));
  
  SetEndstopEnableStateQueueCommand *cmd = (SetEndstopEnableStateQueueCommand *)insertion_point;
    
  cmd->command_type = QUEUE_COMMAND_STRUCTS_TYPE_SET_OUTPUT_SWITCH_STATE;
  cmd->endstops_to_change = 0;
  cmd->endstop_enable_state = 0;
  
  uint8_t device_number, device_state;

  for (uint8_t i=0; i < parameter_length; i+=2)
  {
    device_number = parameter_value[i];
    device_state = parameter_value[i+1];
    
    if (!Device_InputSwitch::IsInUse(device_number) || device_number >= MAX_ENDSTOPS)
      return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;

    cmd->endstops_to_change |= (1 << i);
    if (device_state)
      cmd->endstop_enable_state |= (1 << i);
  }
  
  CommandQueue::EnqueueCommand(sizeof(SetEndstopEnableStateQueueCommand));
  return ENQUEUE_SUCCESS;
}

// Not inlined to reduce stack usage
uint8_t validate_linear_move(const uint8_t *parameter, uint8_t parameter_length)
{
  if (parameter_length < 8)
    return generate_enqueue_insufficient_bytes_error(8, parameter_length);
    
  uint16_t axes_selected;
  uint16_t directions;
  bool use_long_counts;
  bool use_long_axis_mask = parameter[0] & 0x80;
  
  if (use_long_axis_mask)
  {
    if (parameter_length < 10)
      return generate_enqueue_insufficient_bytes_error(10, parameter_length);
    axes_selected = ((parameter[0] & ~0x80) << 8) | parameter[1];
    directions = ((parameter[2] & ~0x80) << 8) | parameter[3];
    use_long_counts = parameter[2] & 0x80;
    parameter += 4;
  }
  else
  {
    axes_selected = parameter[0];
    directions = parameter[1] & ~0x80;
    use_long_counts = parameter[1] & 0x80;
    parameter += 2;
  }
  
  uint8_t primary_axis = *parameter & 0x0F;
  uint8_t nominal_speed_fraction = *parameter++;
  uint8_t final_speed_fraction = *parameter++;
  uint8_t accel_count_fraction = *parameter++;
  uint8_t decel_count_fraction = *parameter++;
  
  uint8_t num_axes = 0;
  uint8_t tmp_axes = axes_selected;
  uint8_t axis_number = 0;
  uint8_t index = 0;
  bool primary_axis_used = false;
  while (tmp_axes != 0)
  {
    if ((tmp_axes & 1) != 0)
    {
      if (!AxisInfo::IsInUse(axis_number))
      {
        generate_response_msg_addPGM(PSTR("Invalid axis included")); // TODO: Language-ify
        return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;
      }
      num_axes += 1;

      if (index == primary_axis)
        primary_axis_used = true;
        
      index += 1;
    }
    tmp_axes >>= 1;
    axis_number += 1;
  }

  if (num_axes == 0)
  {
    generate_response_msg_addPGM(PSTR("No axis selected")); // TODO: Language-ify
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE;
  }
  
  if (!primary_axis_used)
  {
    generate_response_msg_addPGM(PSTR("Invalid primary axis specified")); // TODO: Language-ify
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE;
  }
  
  if (nominal_speed_fraction < final_speed_fraction)
  {
    generate_response_msg_addPGM(PSTR("Nominal < Final Speed")); // TODO: Language-ify
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE;
  }

  uint16_t nominal_speed = (uint32_t)(AxisInfo::GetAxisMaxRate(primary_axis) * nominal_speed_fraction) / 255;  
  uint16_t initial_speed= last_enqueued_final_speed;

  if (nominal_speed < initial_speed)
  {
    generate_response_msg_addPGM(PSTR("Nominal < Initial Speed")); // TODO: Language-ify
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE;
  }

  if (nominal_speed_fraction == 0)
  {
    generate_response_msg_addPGM(PSTR("Nominal Speed is zero")); // TODO: Language-ify
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE;
  }

  if (accel_count_fraction > decel_count_fraction)
  {
    generate_response_msg_addPGM(PSTR("Overlapping accel/decel portions")); // TODO: Language-ify
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE;
  }
  
  uint8_t expected_input_length = 8 + ((use_long_axis_mask) ? 2 : 0) + ((use_long_counts) ? 2*num_axes : num_axes);
  if (parameter_length < expected_input_length)
  {
    return generate_enqueue_insufficient_bytes_error(expected_input_length, parameter_length);
  }

  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t enqueue_linear_move_command(const uint8_t *parameter, uint8_t parameter_length)
{
  // validation code is separated so that it makes it easier to split 
  // the expansion to a secondary queue.
  uint8_t retval = validate_linear_move(parameter, parameter_length);
  if (retval != APP_ERROR_TYPE_SUCCESS)
    return retval;
    
  uint16_t axes_selected;
  uint16_t directions;
  bool use_long_counts;
  bool use_long_axis_mask = parameter[0] & 0x80;
  
  if (use_long_axis_mask)
  {
    axes_selected = ((parameter[0] & ~0x80) << 8) | parameter[1];
    directions = ((parameter[2] & ~0x80) << 8) | parameter[3];
    use_long_counts = parameter[2] & 0x80;
    parameter += 4;
  }
  else
  {
    axes_selected = parameter[0];
    directions = parameter[1] & ~0x80;
    use_long_counts = parameter[1] & 0x80;
    parameter += 2;
  }
  
  uint8_t primary_axis = *parameter & 0x0F;
  bool homing_bit = *parameter++ & 0x10;
  uint8_t nominal_speed_fraction = *parameter++;
  uint8_t final_speed_fraction = *parameter++;
  uint8_t accel_count_fraction = *parameter++;
  uint8_t decel_count_fraction = *parameter++;
  
  uint8_t num_axes = 0;
  uint8_t tmp_axes = axes_selected;
  while (tmp_axes != 0)
  {
    if ((tmp_axes & 1) != 0)
      num_axes += 1;
    tmp_axes >>= 1;
  }

  uint8_t expected_output_length = sizeof(LinearMoveCommand) + ((num_axes-1)*sizeof(AxisMoveInfo));
  
  uint8_t *insertion_point = CommandQueue::GetCommandInsertionPoint(expected_output_length);
  if (insertion_point == 0)
    return ENQUEUE_ERROR_QUEUE_FULL;

  LinearMoveCommand *cmd = (LinearMoveCommand *)insertion_point;
  
  cmd->command_type = QUEUE_COMMAND_STRUCTS_TYPE_LINEAR_MOVE;
  cmd->num_axes = num_axes;

  uint16_t max_steps = 0;
  uint8_t axis_number = 0;
  uint8_t index = 0;
  tmp_axes = axes_selected;

  cmd->directions = 0;
  cmd->endstops_of_interest = 0;
  while (tmp_axes != 0)
  {
    if ((tmp_axes & 1) != 0)
    {
      AxisMoveInfo *axis_move_info = &cmd->axis_move_info[index];
      if (use_long_counts)
        axis_move_info->step_count = (parameter[2*index] << 8) | parameter[(2*index)+1];
      else
        axis_move_info->step_count = parameter[index];

      if (max_steps < axis_move_info->step_count)
        max_steps = axis_move_info->step_count;

      if (axis_number == primary_axis)
        cmd->primary_axis = index;
        
      axis_move_info->axis_info = &AxisInfo::axis_info_array[axis_number];
        
      if (directions & (1 << axis_number))
      {
        cmd->directions |= (1 << index);
        cmd->endstops_of_interest |= AxisInfo::GetAxisMinEndstops(axis_number);
      }
      else
      {
        cmd->endstops_of_interest |= AxisInfo::GetAxisMaxEndstops(axis_number);
      }
        
      index += 1;
    }
    tmp_axes >>= 1;
    axis_number += 1;
  }

  cmd->total_steps = max_steps;
  cmd->steps_phase_2 = (uint32_t)(max_steps * (255 - accel_count_fraction)) / 255;
  cmd->steps_phase_3 = (uint32_t)(max_steps * (255 - decel_count_fraction)) / 255;
  cmd->nominal_rate = (uint32_t)(AxisInfo::GetAxisMaxRate(primary_axis) * nominal_speed_fraction) / 255;
  cmd->final_rate = (uint32_t)(AxisInfo::GetAxisMaxRate(primary_axis) * final_speed_fraction) / 255;

  uint16_t initial_speed = last_enqueued_final_speed;
  
  // a = (v^2 - u^2) / (2*distance)
  if (cmd->steps_phase_2 != max_steps)
    cmd->acceleration_rate = ((cmd->nominal_rate * cmd->nominal_rate) - (initial_speed * initial_speed)) / (2 * (max_steps - cmd->steps_phase_2));
  else
    cmd->acceleration_rate = 0;
  
  if (cmd->steps_phase_3 != 0)
    cmd->deceleration_rate = ((cmd->nominal_rate * cmd->nominal_rate) - (cmd->final_rate * cmd->final_rate)) / (2 * cmd->steps_phase_3);
  else
    cmd->deceleration_rate = 0;
  
  // t = (v - u) / a  and  t = d / v
  uint32_t nominal_block_time = 0;
  if (cmd->acceleration_rate != 0)
    nominal_block_time += (cmd->nominal_rate - initial_speed) * 10000UL / cmd->acceleration_rate;
  if (cmd->steps_phase_2 != cmd->steps_phase_3)
    nominal_block_time += (cmd->steps_phase_2 - cmd->steps_phase_3) * 10000UL / cmd->nominal_rate;
  if (cmd->deceleration_rate != 0)
    nominal_block_time += (cmd->nominal_rate - cmd->final_rate) * 10000UL / cmd->deceleration_rate;
  cmd->nominal_block_time = max(nominal_block_time,0xFFFF);
  
  cmd->homing_bit = homing_bit;
  
  uint16_t underrun_rate = min(cmd->nominal_rate,AxisInfo::GetUnderrunRate(primary_axis));
  
  // d = (v^2 - u^2) / (2*a)
  if (underrun_rate > cmd->final_rate)
    cmd->steps_to_final_speed_from_underrun_rate = ((uint32_t)(underrun_rate*underrun_rate) - (uint32_t)(cmd->final_rate*cmd->final_rate)) / (2 * AxisInfo::GetUnderrunAccelRate(primary_axis));
  else
    cmd->steps_to_final_speed_from_underrun_rate = 0;
 
  CommandQueue::EnqueueCommand(expected_output_length);
  return ENQUEUE_SUCCESS;
}

