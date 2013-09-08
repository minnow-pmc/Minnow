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

FORCE_INLINE static void send_enqueue_error(uint8_t error_type, uint8_t block_index, uint8_t reply_error_code = 0xFF);

FORCE_INLINE static uint8_t enqueue_delay_command(const uint8_t *pacemaker_command, uint8_t pacemaker_command_length);
FORCE_INLINE static uint8_t enqueue_set_output_switch_state_command(const uint8_t *pacemaker_command, uint8_t pacemaker_command_length);
FORCE_INLINE static uint8_t enqueue_set_pwm_output_state_command(const uint8_t *pacemaker_command, uint8_t pacemaker_command_length);
FORCE_INLINE static uint8_t enqueue_set_heater_target_temperature_command(const uint8_t *pacemaker_command, uint8_t pacemaker_command_length);

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
      generate_response_start(RSP_APPLICATION_ERROR, 1);
      generate_response_data_addbyte(PARAM_APP_ERROR_TYPE_FIRMWARE_ERROR);
      generate_response_msg_addPGM(PMSG(MSG_ERR_INSUFFICIENT_MEMORY));
      generate_response_send(); 
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
 
uint8_t enqueue_delay_command(const uint8_t *parameter, uint8_t parameter_length)
{
  if (parameter_length != sizeof(uint16_t))
  {
    generate_response_msg_addPGM(PMSG(ERR_MSG_INSUFFICENT_BYTES));
    generate_response_msg_addbyte(parameter_length);
    generate_response_msg_add(", ");
    generate_response_msg_addPGM(PMSG(MSG_EXPECTING));
    generate_response_msg_addbyte(2);
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_FORMAT; 
  }

  uint8_t *insertion_point = CommandQueue::GetCommandInsertionPoint(sizeof(DelayQueueCommand));
  
  if (insertion_point == 0)
    return ENQUEUE_ERROR_QUEUE_FULL;
    
  DelayQueueCommand *cmd = (DelayQueueCommand *)insertion_point;
    
  cmd->command_type = QUEUE_COMMAND_STRUCTS_TYPE_DELAY;
  cmd->delay = ((parameter[0] << 8) | parameter[1]) * 10; // use us for delay 
  
  CommandQueue::EnqueueCommand(sizeof(DelayQueueCommand));
  return ENQUEUE_SUCCESS;
}

uint8_t enqueue_set_output_switch_state_command(const uint8_t *parameter, uint8_t parameter_length)
{
  const uint8_t number_of_switches = parameter_length/3;

  if ((number_of_switches * 3) != parameter_length)
  {
    generate_response_msg_addPGM(PMSG(ERR_MSG_INSUFFICENT_BYTES));
    generate_response_msg_addbyte(parameter_length);
    generate_response_msg_add(", ");
    generate_response_msg_addPGM(PMSG(MSG_EXPECTING));
    generate_response_msg_addbyte(3*(number_of_switches+1));
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_FORMAT; 
  }
  
  const uint8_t length = sizeof(SetOutputSwitchStateQueueCommand) + 
                            ((number_of_switches - 1) * sizeof(DeviceBitState));
  uint8_t *insertion_point = CommandQueue::GetCommandInsertionPoint(length);
  
  if (insertion_point == 0)
    return ENQUEUE_ERROR_QUEUE_FULL;
  
  SetOutputSwitchStateQueueCommand *cmd = (SetOutputSwitchStateQueueCommand *)insertion_point;
    
  cmd->command_type = QUEUE_COMMAND_STRUCTS_TYPE_SET_OUTPUT_SWITCH_STATE;
  cmd->num_bits = number_of_switches;
  
  uint8_t device_type, device_number, device_state;

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

uint8_t enqueue_set_pwm_output_state_command(const uint8_t *parameter, uint8_t parameter_length)
{
  if (parameter_length < 4)
  {
    generate_response_msg_addPGM(PMSG(ERR_MSG_INSUFFICENT_BYTES));
    generate_response_msg_addbyte(parameter_length);
    generate_response_msg_add(", ");
    generate_response_msg_addPGM(PMSG(MSG_EXPECTING));
    generate_response_msg_addbyte(4);
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_FORMAT; 
  }
  
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

uint8_t enqueue_set_heater_target_temperature_command(const uint8_t *parameter, uint8_t parameter_length)
{
#if 0 // TODO
  if (parameter_length < 3)
  {
    generate_response_msg_addPGM(PMSG(ERR_MSG_INSUFFICENT_BYTES)));
    generate_response_msg_addbyte(parameter_length);
    generate_response_msg_add(", ");
    generate_response_msg_addPGM(PSTR(MSG_EXPECTING));
    generate_response_msg_addbyte(3);
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_FORMAT; 
  }
  
  uint8_t *insertion_point = CommandQueue::GetCommandInsertionPoint(sizeof(SetHeaterTargetTempCommand));
  
  if (insertion_point == 0)
    return ENQUEUE_ERROR_QUEUE_FULL;
  
  const int16_t target_temp = (parameter_value[1] << 8) | parameter_value[2];
  
  if (!Device_Heater::IsValidConfig(parameter_value[0])
  xxx
      return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;
  if (!Device_Heater::ValidateTargetTemperatureRange
  
  
  (target_temp)
  {
  xxx
      return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;
  }

  SetHeaterTargetTempCommand *cmd = (SetHeaterTargetTempCommand *)insertion_point;
    
  cmd->command_type = QUEUE_COMMAND_STRUCTS_TYPE_SET_HEATER_TARGET_TEMP;
  cmd->heater_number = parameter_value[0];
  cmd->target_temp = target_temp;

  CommandQueue::EnqueueCommand(sizeof(SetHeaterTargetTempCommand));
#endif  
  return ENQUEUE_SUCCESS;
}