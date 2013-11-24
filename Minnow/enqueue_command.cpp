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

extern bool is_checkpoint_last;

static void send_enqueue_error(uint8_t error_type, uint8_t block_index, uint8_t reply_error_code = 0xFF);
static uint8_t generate_enqueue_insufficient_bytes_error(uint8_t expected_num_bytes, uint8_t rcvd_num_bytes);

FORCE_INLINE uint8_t enqueue_linear_move_command(const uint8_t *queue_command, uint8_t queue_command_length);

FORCE_INLINE static uint8_t enqueue_delay_command(const uint8_t *queue_command, uint8_t queue_command_length);
FORCE_INLINE static uint8_t enqueue_set_output_switch_state_command(const uint8_t *queue_command, uint8_t queue_command_length);
FORCE_INLINE static uint8_t enqueue_set_pwm_output_state_command(const uint8_t *queue_command, uint8_t queue_command_length);
FORCE_INLINE static uint8_t enqueue_set_output_tone_command(const uint8_t *queue_command, uint8_t queue_command_length);
FORCE_INLINE static uint8_t enqueue_set_heater_target_temperature_command(const uint8_t *queue_command, uint8_t queue_command_length);
FORCE_INLINE static uint8_t enqueue_set_stepper_enable_state_command(const uint8_t *queue_command, uint8_t queue_command_length);
FORCE_INLINE static uint8_t enqueue_set_endstop_enable_state_command(const uint8_t *queue_command, uint8_t queue_command_length);
FORCE_INLINE static uint8_t enqueue_move_checkpoint_command(const uint8_t *queue_command, uint8_t queue_command_length);
uint8_t validate_linear_move(const uint8_t *queue_command, uint8_t queue_command_length);

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
    const uint8_t cmd = ptr[0];
    
    if (length < 1 || ptr + length < parameter_value + parameter_length
      || (cmd == QUEUE_COMMAND_ORDER_WRAPPER && length < 2))
    {
      generate_response_msg_addPGM(PMSG(ERR_MSG_INSUFFICENT_BYTES));
      generate_response_msg_add_ascii_number(length);
      send_enqueue_error(QUEUE_COMMAND_ERROR_TYPE_MALFORMED_BLOCK, index);
      return;
    }
    
    uint8_t retval;
    switch (cmd)
    {
    case QUEUE_COMMAND_LINEAR_MOVE:
      retval = enqueue_linear_move_command(ptr+1, length-1);
      break;
    
    case QUEUE_COMMAND_MOVEMENT_CHECKPOINT:
      retval = enqueue_move_checkpoint_command(ptr+1, length-1);
      break;
    
    case QUEUE_COMMAND_DELAY:
      retval = enqueue_delay_command(ptr+1, length-1);
      break;
    
    case QUEUE_COMMAND_ORDER_WRAPPER:
      switch (ptr[1])
      {
      case ORDER_SET_OUTPUT_SWITCH_STATE:
        retval = enqueue_set_output_switch_state_command(ptr+2, length-2);
        break;
      case ORDER_SET_PWM_OUTPUT_STATE:
        retval = enqueue_set_pwm_output_state_command(ptr+2, length-2);
        break;
      case ORDER_SET_OUTPUT_TONE:
        retval = enqueue_set_output_tone_command(ptr+2, length-2);
        break;
      case ORDER_SET_HEATER_TARGET_TEMP:
        retval = enqueue_set_heater_target_temperature_command(ptr+2, length-2);
        break;
      case ORDER_ENABLE_DISABLE_STEPPERS:
        retval = enqueue_set_stepper_enable_state_command(ptr+2, length-2);
        break;
      case ORDER_ENABLE_DISABLE_ENDSTOPS:
        retval = enqueue_set_endstop_enable_state_command(ptr+2, length-2);
        break;
      default:
        generate_response_msg_addPGM(PMSG(ERR_MSG_QUEUE_ORDER_NOT_PERMITTED));
        generate_response_msg_add_ascii_number(ptr[1]);
        send_enqueue_error(QUEUE_COMMAND_ERROR_TYPE_ERROR_IN_COMMAND_BLOCK, index, PARAM_APP_ERROR_TYPE_UNKNOWN_ORDER);
        return;
      }
      break;
      
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
  generate_response_msg_add_ascii_number(rcvd_num_bytes);
  generate_response_msg_addPGM(PSTR(", "));
  generate_response_msg_addPGM(PMSG(MSG_EXPECTING));
  generate_response_msg_add_ascii_number(expected_num_bytes);
  return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_FORMAT; 
}
 
FORCE_INLINE uint8_t enqueue_delay_command(const uint8_t *queue_command, uint8_t queue_command_length)
{
  if (queue_command_length != sizeof(uint16_t))
    return generate_enqueue_insufficient_bytes_error(sizeof(uint16_t), queue_command_length);

  uint8_t *insertion_point = CommandQueue::GetCommandInsertionPoint(sizeof(DelayQueueCommand));
  
  if (insertion_point == 0)
    return ENQUEUE_ERROR_QUEUE_FULL;
    
  DelayQueueCommand *cmd = (DelayQueueCommand *)insertion_point;
    
  cmd->command_type = QUEUE_COMMAND_STRUCTS_TYPE_DELAY;
  
  uint16_t delay = (queue_command[0] << 8) | queue_command[1];
  cmd->delay = delay * 10UL; // use us for delay 
  
  CommandQueue::EnqueueCommand(sizeof(DelayQueueCommand));
  return ENQUEUE_SUCCESS;
}

FORCE_INLINE uint8_t enqueue_set_output_switch_state_command(const uint8_t *queue_command, uint8_t queue_command_length)
{
  const uint8_t number_of_switches = queue_command_length/3;

  if ((number_of_switches * 3) != queue_command_length)
    return generate_enqueue_insufficient_bytes_error(3*(number_of_switches+1), queue_command_length);
  
  const uint8_t length = sizeof(SetOutputSwitchStateQueueCommand) + 
                            ((number_of_switches - 1) * sizeof(DeviceBitState));
  uint8_t *insertion_point = CommandQueue::GetCommandInsertionPoint(length);
  
  if (insertion_point == 0)
    return ENQUEUE_ERROR_QUEUE_FULL;
  
  SetOutputSwitchStateQueueCommand *cmd = (SetOutputSwitchStateQueueCommand *)insertion_point;
    
  cmd->command_type = QUEUE_COMMAND_STRUCTS_TYPE_SET_OUTPUT_SWITCH_STATE;
  cmd->num_bits = number_of_switches;
  
  uint8_t device_number, device_state;

  for (uint8_t i = 0; i < queue_command_length; i+=3)
  {
    device_number = queue_command[i+1];
    device_state = queue_command[i+2];
    
    switch(queue_command[i])
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
      break;
    }    
    default:
      return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_TYPE;
    }
  }  
  
  CommandQueue::EnqueueCommand(length);
  return ENQUEUE_SUCCESS;
}

FORCE_INLINE uint8_t enqueue_set_pwm_output_state_command(const uint8_t *queue_command, uint8_t queue_command_length)
{
  if (queue_command_length < 4)
    return generate_enqueue_insufficient_bytes_error(4, queue_command_length);
  
  uint8_t *insertion_point = CommandQueue::GetCommandInsertionPoint(sizeof(SetPwmOutputStateQueueCommand));
  
  if (insertion_point == 0)
    return ENQUEUE_ERROR_QUEUE_FULL;
     
  const uint8_t device_number = queue_command[1];
  
  switch(queue_command[0])
  {
  case PM_DEVICE_TYPE_PWM_OUTPUT:
  {
    if (!Device_PwmOutput::IsInUse(device_number))
      return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;

    SetPwmOutputStateQueueCommand *cmd = (SetPwmOutputStateQueueCommand *)insertion_point;
    cmd->command_type = QUEUE_COMMAND_STRUCTS_TYPE_SET_PWM_OUTPUT_STATE;
    cmd->device_number = device_number;
    cmd->value = queue_command[2]; // the LSB is ignored
    CommandQueue::EnqueueCommand(sizeof(SetPwmOutputStateQueueCommand));
    return ENQUEUE_SUCCESS;
  }    
  default:
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_TYPE;
  }  
}

FORCE_INLINE uint8_t enqueue_set_output_tone_command(const uint8_t *queue_command, uint8_t queue_command_length)
{
  if (queue_command_length < 4)
    return generate_enqueue_insufficient_bytes_error(4, queue_command_length);
  
  uint8_t *insertion_point = CommandQueue::GetCommandInsertionPoint(sizeof(SetBuzzerStateQueueCommand));
  
  if (insertion_point == 0)
    return ENQUEUE_ERROR_QUEUE_FULL;
     
  const uint8_t device_number = queue_command[1];
  
  switch(queue_command[0])
  {
  case PM_DEVICE_TYPE_BUZZER:
  {
    if (!Device_Buzzer::IsInUse(device_number))
      return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;

    SetBuzzerStateQueueCommand *cmd = (SetBuzzerStateQueueCommand *)insertion_point;
    cmd->command_type = QUEUE_COMMAND_STRUCTS_TYPE_SET_BUZZER_STATE;
    cmd->device_number = device_number;
    cmd->value = queue_command[2]; // the LSB is ignored
    CommandQueue::EnqueueCommand(sizeof(SetBuzzerStateQueueCommand));
    return ENQUEUE_SUCCESS;
  }    
  default:
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_TYPE;
  }  
}

FORCE_INLINE uint8_t enqueue_set_heater_target_temperature_command(const uint8_t *queue_command, uint8_t queue_command_length)
{
  if (queue_command_length < 3)
    return generate_enqueue_insufficient_bytes_error(3, queue_command_length);
  
  uint8_t *insertion_point = CommandQueue::GetCommandInsertionPoint(sizeof(SetHeaterTargetTempCommand));
  
  if (insertion_point == 0)
    return ENQUEUE_ERROR_QUEUE_FULL;
  
  uint8_t heater_number = queue_command[0];
  int16_t target_temp = (queue_command[1] << 8) | queue_command[2];
  float target_ftemp = (float)target_temp / 10;
  
  uint8_t retval = Device_Heater::ValidateTargetTemperature(heater_number, target_ftemp);
  if (retval != APP_ERROR_TYPE_SUCCESS)
    return retval;

  if (is_stopped && target_ftemp != SENSOR_TEMPERATURE_INVALID)
  {
    generate_response_msg_addPGM(PMSG(MSG_ERR_CANNOT_ACTIVATE_DEVICE_WHEN_STOPPED));
    return PARAM_APP_ERROR_TYPE_CANNOT_ACTIVATE_DEVICE;
  }
    
  SetHeaterTargetTempCommand *cmd = (SetHeaterTargetTempCommand *)insertion_point;
  cmd->command_type = QUEUE_COMMAND_STRUCTS_TYPE_SET_HEATER_TARGET_TEMP;
  cmd->heater_number = queue_command[0];
  cmd->target_temp = target_ftemp;

  CommandQueue::EnqueueCommand(sizeof(SetHeaterTargetTempCommand));
  return ENQUEUE_SUCCESS;
}

FORCE_INLINE uint8_t enqueue_set_stepper_enable_state_command(const uint8_t *queue_command, uint8_t queue_command_length)
{
  if (queue_command_length != 0 && queue_command_length != 2)
    return generate_enqueue_insufficient_bytes_error(2, queue_command_length);
  
  uint8_t *insertion_point = CommandQueue::GetCommandInsertionPoint(sizeof(SetStepperEnableStateQueueCommand));
  
  if (insertion_point == 0)
    return ENQUEUE_ERROR_QUEUE_FULL;
  
  SetStepperEnableStateQueueCommand *cmd = (SetStepperEnableStateQueueCommand *)insertion_point;
    
  cmd->command_type = QUEUE_COMMAND_STRUCTS_TYPE_SET_STEPPER_ENABLE_STATE;
  if (queue_command_length == 0)
  {
    cmd->stepper_number = 0xFF;
    cmd->stepper_state = 0;
  }
  else
  {
    if (!AxisInfo::IsInUse(queue_command[0]))
    {
      return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;
    }
    if (is_stopped && queue_command[1] != 0)
    {
      generate_response_msg_addPGM(PMSG(MSG_ERR_CANNOT_ACTIVATE_DEVICE_WHEN_STOPPED));
      return PARAM_APP_ERROR_TYPE_CANNOT_ACTIVATE_DEVICE;
    }
    cmd->stepper_number = queue_command[0];
    cmd->stepper_state = queue_command[1];
  }

  CommandQueue::EnqueueCommand(sizeof(SetStepperEnableStateQueueCommand));
  return ENQUEUE_SUCCESS;
}

FORCE_INLINE static uint8_t enqueue_set_endstop_enable_state_command(const uint8_t *queue_command, uint8_t queue_command_length)
{
  if ((queue_command_length & 1) != 0)
    return generate_enqueue_insufficient_bytes_error(queue_command_length+1, queue_command_length);
  
  uint8_t *insertion_point = CommandQueue::GetCommandInsertionPoint(sizeof(SetEndstopEnableStateQueueCommand));
  
  SetEndstopEnableStateQueueCommand *cmd = (SetEndstopEnableStateQueueCommand *)insertion_point;
    
  cmd->command_type = QUEUE_COMMAND_STRUCTS_TYPE_SET_ENDSTOP_ENABLE_STATE;
  cmd->endstops_to_change = 0;
  cmd->endstop_enable_state = 0;
  
  uint8_t device_number, device_state;

  for (uint8_t i=0; i < queue_command_length; i+=2)
  {
    device_number = queue_command[i];
    device_state = queue_command[i+1];
    
    if (!Device_InputSwitch::IsInUse(device_number) || device_number >= MAX_ENDSTOPS)
      return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;

    cmd->endstops_to_change |= (1 << device_number);
    if (device_state)
      cmd->endstop_enable_state |= (1 << device_number);
  }
  
  CommandQueue::EnqueueCommand(sizeof(SetEndstopEnableStateQueueCommand));
  return ENQUEUE_SUCCESS;
}

uint8_t validate_linear_move(const uint8_t *queue_command, uint8_t queue_command_length)
{
  uint8_t expected_length = 7; // minimum short header size
    
  uint16_t axes_selected = 0;
  uint16_t directions = 0;
  bool use_long_counts = false;
  bool use_long_axis_mask = queue_command[0] & 0x80;
  
  if (use_long_axis_mask)
  {
    expected_length += 2;
    axes_selected = ((queue_command[0] & ~0x80) << 8) | queue_command[1];
    directions = ((queue_command[2] & ~0x80) << 8) | queue_command[3];
    use_long_counts = queue_command[2] & 0x80;
    queue_command += 4;
  }
  else
  {
    axes_selected = queue_command[0];
    directions = queue_command[1] & ~0x80;
    use_long_counts = queue_command[1] & 0x80;
    queue_command += 2;
  }
  if (use_long_counts)
    expected_length += 2;

  if (queue_command_length < expected_length)
    return generate_enqueue_insufficient_bytes_error(expected_length, queue_command_length);
  
  uint8_t primary_axis = *queue_command++ & 0x0F; // (don't need homing bit for validation)
  uint8_t nominal_speed_fraction = *queue_command++;
  uint8_t final_speed_fraction = *queue_command++;
  
  uint16_t accel_count;
  uint16_t decel_count;
  if (!use_long_counts)
  {
    accel_count = *queue_command++;
    decel_count = *queue_command++;
  }
  else
  {
    accel_count = (queue_command[0] << 8) | queue_command[1];
    decel_count = (queue_command[2] << 8) | queue_command[3];
    queue_command += 4;
  }
  
  uint8_t num_axes = 0;
  uint8_t tmp_axes = axes_selected;
  uint8_t axis_number = 0;
  uint8_t index = 0;
  int8_t primary_axis_index = -1;
  while (tmp_axes != 0)
  {
    if ((tmp_axes & 1) != 0)
    {
      if (!AxisInfo::IsInUse(axis_number))
      {
        generate_response_msg_addPGM(PSTR("Invalid axis included ")); // TODO: Language-ify
        generate_response_msg_add(axis_number);
        return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;
      }
      num_axes += 1;

      if (index == primary_axis)
      {
        if (AxisInfo::GetAxisMaxRate(axis_number) == 0)
        {
          generate_response_msg_addPGM(PSTR("Maximum movement rate not configured for primary axis")); // TODO: Language-ify
          return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;
        }
        if (AxisInfo::GetUnderrunRate(axis_number) == 0)
        {
          generate_response_msg_addPGM(PSTR("Underrun avoidance parameters not configured for primary axis")); // TODO: Language-ify
          return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;
        }
        primary_axis_index = index;
      }
        
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
  
  if (primary_axis_index < 0)
  {
    generate_response_msg_addPGM(PSTR("Invalid primary axis specified")); // TODO: Language-ify
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE;
  }
  
  if (nominal_speed_fraction < final_speed_fraction)
  {
    generate_response_msg_addPGM(PSTR("Nominal < Final Speed")); // TODO: Language-ify
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE;
  }

  uint16_t nominal_speed = (uint32_t)AxisInfo::GetAxisMaxRate(primary_axis) * nominal_speed_fraction / 255;  
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

  expected_length += (use_long_counts) ? 2*num_axes : num_axes;
  if (queue_command_length < expected_length)
  {
    return generate_enqueue_insufficient_bytes_error(expected_length, queue_command_length);
  }

  uint16_t primary_axis_count;
  if (!use_long_counts)
    primary_axis_count = queue_command[primary_axis_index];
  else
    primary_axis_count = (queue_command[2*primary_axis_index] << 8) | queue_command[(2*primary_axis_index) + 1];

  if (accel_count > primary_axis_count)
  {
    generate_response_msg_addPGM(PSTR("Invalid acceleration count")); // TODO: Language-ify
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE;
  }

  if (decel_count > primary_axis_count)
  {
    generate_response_msg_addPGM(PSTR("Invalid deceleration count")); // TODO: Language-ify
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE;
  }
  
  if (accel_count + decel_count > primary_axis_count)
  {
    generate_response_msg_addPGM(PSTR("Overlapped acceleration counts")); // TODO: Language-ify
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE;
  }
  
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t enqueue_linear_move_command(const uint8_t *queue_command, uint8_t queue_command_length)
{
  // validation code is separated so that it makes it easier to split 
  // the expansion to a secondary queue.
  uint8_t retval = validate_linear_move(queue_command, queue_command_length);
  if (retval != APP_ERROR_TYPE_SUCCESS)
    return retval;
    
  uint16_t axes_selected;
  uint16_t directions;
  bool use_long_counts;
  bool use_long_axis_mask = queue_command[0] & 0x80;
  
  if (use_long_axis_mask)
  {
    axes_selected = ((queue_command[0] & ~0x80) << 8) | queue_command[1];
    directions = ((queue_command[2] & ~0x80) << 8) | queue_command[3];
    use_long_counts = queue_command[2] & 0x80;
    queue_command += 4;
  }
  else
  {
    axes_selected = queue_command[0];
    directions = queue_command[1] & ~0x80;
    use_long_counts = queue_command[1] & 0x80;
    queue_command += 2;
  }
  
  uint8_t primary_axis = *queue_command & 0x0F;
  bool homing_bit = *queue_command++ & 0x10;
  uint8_t nominal_speed_fraction = *queue_command++;
  uint8_t final_speed_fraction = *queue_command++;
  
  uint16_t accel_count;
  uint16_t decel_count;
  if (!use_long_counts)
  {
    accel_count = *queue_command++;
    decel_count = *queue_command++;
  }
  else
  {
    accel_count = (queue_command[0] << 8) | queue_command[1];
    decel_count = (queue_command[2] << 8) | queue_command[3];
    queue_command += 4;
  }
  
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
  uint16_t primary_axis_steps = 0;

  tmp_axes = axes_selected;

  cmd->directions = 0;
  cmd->endstops_of_interest = 0;
  while (tmp_axes != 0)
  {
    if ((tmp_axes & 1) != 0)
    {
      AxisMoveInfo *axis_move_info = &cmd->axis_move_info[index];
      if (use_long_counts)
        axis_move_info->step_count = (queue_command[2*index] << 8) | queue_command[(2*index)+1];
      else
        axis_move_info->step_count = queue_command[index];

      if (max_steps < axis_move_info->step_count)
        max_steps = axis_move_info->step_count;

      if (axis_number == primary_axis)
      {
        cmd->primary_axis = index;
        primary_axis_steps = axis_move_info->step_count;
      }
        
      axis_move_info->axis_info = &AxisInfo::axis_info_array[axis_number];
        
      if (directions & (1 << axis_number))
      {
        cmd->directions |= (1 << index);
        cmd->endstops_of_interest |= AxisInfo::GetAxisMaxEndstops(axis_number);
      }
      else
      {
        cmd->endstops_of_interest |= AxisInfo::GetAxisMinEndstops(axis_number);
      }
        
      index += 1;
    }
    tmp_axes >>= 1;
    axis_number += 1;
  }

  cmd->total_steps = max_steps;
  cmd->nominal_rate = (uint32_t)AxisInfo::GetAxisMaxRate(primary_axis) * nominal_speed_fraction / 255;
  cmd->final_rate = (uint32_t)AxisInfo::GetAxisMaxRate(primary_axis) * final_speed_fraction / 255;

  if (max_steps == primary_axis_steps)
  {
    cmd->steps_phase_2 = primary_axis_steps - accel_count;
    cmd->steps_phase_3 = decel_count;
  }
  else
  {
    cmd->steps_phase_2 = max_steps - ((uint32_t)max_steps * accel_count / primary_axis_steps);
    cmd->steps_phase_3 = (uint32_t)max_steps * decel_count / primary_axis_steps;
  }
  
  uint16_t initial_speed = last_enqueued_final_speed;
  
  // a = (v^2 - u^2) / (2*distance)
  if (cmd->steps_phase_2 != max_steps)
    cmd->acceleration_rate = ((((uint32_t)cmd->nominal_rate * cmd->nominal_rate) - ((uint32_t)initial_speed * initial_speed)) 
                                / (max_steps - cmd->steps_phase_2)) / 2;
  else
    cmd->acceleration_rate = 0;
  
  if (cmd->steps_phase_3 != 0)
    cmd->deceleration_rate = ((((uint32_t)cmd->nominal_rate * cmd->nominal_rate) - ((uint32_t)cmd->final_rate * cmd->final_rate)) 
                                / cmd->steps_phase_3) / 2;
  else
    cmd->deceleration_rate = 0;
  
  // t = (v - u) / a  and  t = d / v
  // TODO - work through again
  uint32_t nominal_block_time = 0;
  if (cmd->acceleration_rate != 0)
    nominal_block_time += (cmd->nominal_rate - initial_speed) * 10000UL / cmd->acceleration_rate;
  if (cmd->steps_phase_2 != cmd->steps_phase_3)
    nominal_block_time += (cmd->steps_phase_2 - cmd->steps_phase_3) * 10000UL / cmd->nominal_rate;
  if (cmd->deceleration_rate != 0)
    nominal_block_time += (cmd->nominal_rate - cmd->final_rate) * 10000UL / cmd->deceleration_rate;
  if (nominal_block_time < 0xFFFF)
    cmd->nominal_block_time = nominal_block_time;
  else
    cmd->nominal_block_time = 0xFFFF;
  
  cmd->homing_bit = homing_bit;
  
  uint16_t underrun_rate = min(cmd->nominal_rate,AxisInfo::GetUnderrunRate(primary_axis));
  
  // d = (v^2 - u^2) / (2*a)
  if (underrun_rate > cmd->final_rate)
    cmd->steps_to_final_speed_from_underrun_rate = ((uint32_t)underrun_rate*underrun_rate) - ((uint32_t)cmd->final_rate*cmd->final_rate) 
                                                      / (2 * AxisInfo::GetUnderrunAccelRate(primary_axis));
  else
    cmd->steps_to_final_speed_from_underrun_rate = 0;
 
  is_checkpoint_last = false;
 
  // TODO safely increment   
  // queued_microseconds_remaining -= nominal_block_time;
  // queued_steps_remaining -= total_step_events;

  CommandQueue::EnqueueCommand(expected_output_length);
  return ENQUEUE_SUCCESS;
}

uint8_t enqueue_move_checkpoint_command(const uint8_t *queue_command, uint8_t queue_command_length)
{
  is_checkpoint_last = true;
}