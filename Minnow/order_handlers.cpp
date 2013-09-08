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
// This file handles each of the Pacemaker Orders
// 
 
#include "order_handlers.h" 
#include "Minnow.h" 
#include "protocol.h"
#include "response.h"
#include "firmware_configuration.h"


#include "Device_InputSwitch.h"
#include "Device_OutputSwitch.h"
#include "Device_PwmOutput.h" 
#include "Device_Heater.h" 
#include "Device_Buzzer.h" 

#include "NVConfigStore.h" 
#include "enqueue_command.h"
#include "CommandQueue.h"

//===========================================================================
//=============================imported variables============================
//===========================================================================

extern uint16_t total_executed_queued_command_count;

//===========================================================================
//=============================private variables=============================
//===========================================================================


//===========================================================================
//=============================public variables=============================
//===========================================================================

//===========================================================================
//=============================ROUTINES=============================
//===========================================================================

// Basic Pacemaker Orders
FORCE_INLINE static void handle_resume_order();
FORCE_INLINE static void handle_request_information_order();
FORCE_INLINE static void handle_device_name_order();
FORCE_INLINE static void handle_request_temperature_reading_order();
FORCE_INLINE static void handle_get_heater_configuration_order();
FORCE_INLINE static void handle_configure_heater_order();
FORCE_INLINE static void handle_set_heater_target_temperature_order();
FORCE_INLINE static void handle_get_input_switch_state_order();
FORCE_INLINE static void handle_set_output_switch_state_order();
FORCE_INLINE static void handle_set_pwm_output_state_order();
FORCE_INLINE static void handle_write_firmware_configuration_value_order();
FORCE_INLINE static void handle_clear_command_queue_order();

//
// Top level order handler
//
void process_command()
{
  if (is_stopped && !stopped_is_acknowledged && order_code != ORDER_RESUME)
  {
    send_stopped_response();
    return;
  }
 
  switch (order_code)
  {
  case ORDER_RESET:
    emergency_stop();
    die();
    break;
  case ORDER_RESUME:
    handle_resume_order();
    break;
  case ORDER_REQUEST_INFORMATION:
    handle_request_information_order();
    break;
  case ORDER_DEVICE_NAME:
    handle_device_name_order();
    break;
  case ORDER_REQUEST_TEMPERATURE_READING:
    handle_request_temperature_reading_order();
    break;
  case ORDER_GET_HEATER_CONFIGURATION:
    handle_get_heater_configuration_order();
    break;
  case ORDER_CONFIGURE_HEATER:
    handle_configure_heater_order();
    break;
  case ORDER_SET_HEATER_TARGET_TEMP:
    handle_set_heater_target_temperature_order();
    break;
  case ORDER_GET_INPUT_SWITCH_STATE:
    handle_get_input_switch_state_order();
    break;
  case ORDER_SET_OUTPUT_SWITCH_STATE:
    handle_set_output_switch_state_order();
    break;
  case ORDER_SET_PWM_OUTPUT_STATE:
    handle_set_pwm_output_state_order();
    break;
  case ORDER_WRITE_FIRMWARE_CONFIG_VALUE:
    handle_write_firmware_configuration_value_order();
    break;
  case ORDER_READ_FIRMWARE_CONFIG_VALUE:
    // note: get_command() already makes the end of the command (ie. name) null-terminated
    handle_firmware_configuration_request((const char *)&parameter_value[0], 0);
    break;
  case ORDER_TRAVERSE_FIRMWARE_CONFIG:
    // note: get_command() already makes the end of the command (ie. name) null-terminated
    handle_firmware_configuration_traversal((const char *)&parameter_value[0]);
    break;
  case ORDER_EMERGENCY_STOP:
    emergency_stop();
    send_OK_response();
    break;
  case ORDER_QUEUE_COMMAND_BLOCKS:
    enqueue_command();
    break;
  case ORDER_CLEAR_COMMAND_QUEUE:
    handle_clear_command_queue_order();
    break;
  default:
    generate_response_start(RSP_APPLICATION_ERROR);
    generate_response_data_addbyte(PARAM_APP_ERROR_TYPE_UNKNOWN_ORDER);
    generate_response_send();
    break;
  }
}
 
void handle_resume_order()
{
  if (parameter_length < 1)
  {
    send_insufficient_bytes_error_response(1);
    return; 
  }
  
  const uint8_t resume_type = parameter_value[0];
  
  if (resume_type == PARAM_RESUME_TYPE_QUERY)
  {
    send_stopped_response();
  }
  else if (resume_type == PARAM_RESUME_TYPE_ACKNOWLEDGE)
  {
    stopped_is_acknowledged = true;
    send_stopped_response();
  }
  else if (resume_type == PARAM_RESUME_TYPE_CLEAR)
  {
    stopped_is_acknowledged = true;

    if (stopped_type == PARAM_STOPPED_TYPE_ONE_TIME_OR_CLEARED)
      is_stopped = false;
    
    send_stopped_response();    
  }
  else
  {
    send_app_error_response(PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE,0);
  }
}
 
void handle_request_information_order()
{
  if (parameter_length < 1)
  {
    send_insufficient_bytes_error_response(1);
    return; 
  }

  const uint8_t request_type = parameter_value[0];
  
  generate_response_start(RSP_OK);
  char *response_data_buf = (char *)generate_response_data_ptr();
  uint8_t response_data_buf_len = generate_response_data_len();
  uint8_t retval;
  
  switch(request_type)
  {
  case PARAM_REQUEST_INFO_FIRMWARE_NAME:
    generate_response_data_addPGM(PSTR(MINNOW_FIRMWARE_NAME));
    break;
  
  case PARAM_REQUEST_INFO_BOARD_SERIAL_NUMBER:
    if ((retval = NVConfigStore.GetBoardSerialNumber(response_data_buf, response_data_buf_len)) > 0)
      generate_response_data_addlen(retval);
    break;
  
  case PARAM_REQUEST_INFO_BOARD_NAME:
    if ((retval = NVConfigStore.GetHardwareName(response_data_buf, response_data_buf_len)) > 0)
      generate_response_data_addlen(retval);
    break;
    
  case PARAM_REQUEST_INFO_GIVEN_NAME:
    if ((retval = NVConfigStore.GetBoardIdentity(response_data_buf, response_data_buf_len)) > 0)
      generate_response_data_addlen(retval);
    break;
    
  case PARAM_REQUEST_INFO_PROTO_VERSION_MAJOR:
    generate_response_data_addbyte(PM_PROTCOL_VERSION_MAJOR);
    break;
    
  case PARAM_REQUEST_INFO_PROTO_VERSION_MINOR:
    generate_response_data_addbyte(PM_PROTCOL_VERSION_MAJOR);
    break;
    
  case PARAM_REQUEST_INFO_SUPPORTED_EXTENSIONS:
    // TODO add extension
    break;
    
  case PARAM_REQUEST_INFO_FIRMWARE_TYPE:
    generate_response_data_addbyte(PM_FIRMWARE_TYPE_MINNOW);
    break;
    
  case PARAM_REQUEST_INFO_FIRMWARE_VERSION_MAJOR:
    generate_response_data_addbyte(MINNOW_FIRMWARE_VERSION_MAJOR);
    break;
    
  case PARAM_REQUEST_INFO_FIRMWARE_VERSION_MINOR:
    generate_response_data_addbyte(MINNOW_FIRMWARE_VERSION_MINOR);
    break;
    
  case PARAM_REQUEST_INFO_HARDWARE_TYPE:
    generate_response_data_addbyte(NVConfigStore.GetHardwareType());
    break;
    
  case PARAM_REQUEST_INFO_HARDWARE_REVISION:
    if ((retval = NVConfigStore.GetHardwareRevision()) != 0xFF)
      generate_response_data_addbyte(retval);
    break;
    
  case PARAM_REQUEST_INFO_NUM_STEPPERS:
    generate_response_data_addbyte(0); //TODO
    break;
    
  case PARAM_REQUEST_INFO_NUM_HEATERS:
    generate_response_data_addbyte(Device_Heater::GetNumDevices());
    break;
    
  case PARAM_REQUEST_INFO_NUM_PWM_OUTPUTS:
    generate_response_data_addbyte(Device_PwmOutput::GetNumDevices());
    break;
    
  case PARAM_REQUEST_INFO_NUM_TEMP_SENSORS:
    generate_response_data_addbyte(Device_TemperatureSensor::GetNumDevices());
    break;
    
  case PARAM_REQUEST_INFO_NUM_SWITCH_INPUTS:
    generate_response_data_addbyte(Device_InputSwitch::GetNumDevices());
    break;
    
  case PARAM_REQUEST_INFO_NUM_SWITCH_OUTPUTS:
    generate_response_data_addbyte(Device_OutputSwitch::GetNumDevices());
    break;
    
  case PARAM_REQUEST_INFO_NUM_BUZZERS:
    generate_response_data_addbyte(Device_Buzzer::GetNumDevices());
    break;

  default:
    send_app_error_response(PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE,0);
    return;
  }
  generate_response_send();  
}
 
void handle_device_name_order()
{
  if (parameter_length < 2)
  {
    send_insufficient_bytes_error_response(2);
    return; 
  }

  const uint8_t device_type = parameter_value[0];
  const uint8_t device_number = parameter_value[1];
  
  char *response_data_buf = (char *)generate_response_data_ptr();
  uint8_t response_data_buf_len = generate_response_data_len();
  uint8_t retval;
  
  switch(device_type)
  {
  case PM_DEVICE_TYPE_SWITCH_INPUT:
  case PM_DEVICE_TYPE_SWITCH_OUTPUT:
  case PM_DEVICE_TYPE_PWM_OUTPUT:
  case PM_DEVICE_TYPE_STEPPER:
  case PM_DEVICE_TYPE_HEATER:
  case PM_DEVICE_TYPE_TEMP_SENSOR:
  case PM_DEVICE_TYPE_BUZZER:
    // TODO check device number
    if ((retval = NVConfigStore.GetDeviceName(device_type, device_number, response_data_buf, response_data_buf_len)) > 0)
      generate_response_data_addlen(retval);
    break;
  default:
    send_app_error_response(PARAM_APP_ERROR_TYPE_INVALID_DEVICE_TYPE,0);
  }
  
}
  
void handle_request_temperature_reading_order()
{
  if (parameter_length < 2)
  {
    send_insufficient_bytes_error_response(2);
    return; 
  }
  if ((parameter_length & 1) == 1)
  {
    send_app_error_response(PARAM_APP_ERROR_TYPE_BAD_PARAMETER_FORMAT,parameter_length);
    return; 
  }

  generate_response_start(RSP_OK,parameter_length);
  
  for (int i = 0; i < parameter_length; i+=2)
  {
    const uint8_t device_type = parameter_value[i];
    const uint8_t device_number = parameter_value[i+1];
    
    switch(device_type)
    {
    case PM_DEVICE_TYPE_HEATER:
    {
      if (!Device_Heater::IsInUse(device_number))
      {
        send_app_error_response(PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER, i+1);
        return;
      }
      int16_t temp = Device_Heater::ReadCurrentTemperature(device_number);
      generate_response_data_addbyte(highByte(temp));
      generate_response_data_addbyte(lowByte(temp));
      break;
    }
    case PM_DEVICE_TYPE_TEMP_SENSOR: 
    {
      if (!Device_TemperatureSensor::IsInUse(device_number))
      {
        send_app_error_response(PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER, i+1);
        return;
      }
      int16_t temp = Device_TemperatureSensor::ReadCurrentTemperature(device_number);
      generate_response_data_addbyte(highByte(temp));
      generate_response_data_addbyte(lowByte(temp));
      break;
    }
    default:
      send_app_error_response(PARAM_APP_ERROR_TYPE_INVALID_DEVICE_TYPE, i);
      return;
    }
  }  
  
  generate_response_send();
}
  
void handle_get_heater_configuration_order()
{
  if (parameter_length < 1)
  {
    send_insufficient_bytes_error_response(1);
    return; 
  }
  
  const uint8_t heater_number = parameter_value[0];

  if (!Device_Heater::IsInUse(heater_number))
  {
    send_app_error_response(PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER,0);
    return;
  }

  // TODO(future): support dynmically configured heaters
  generate_response_start(RSP_OK);
  generate_response_data_addbyte(PARAM_HEATER_CONFIG_INTERNAL_SENSOR_CONFIG);
  generate_response_data_addbyte(0xFF);
  generate_response_send(); 
}
  
void handle_configure_heater_order()
{
  if (parameter_length < 2)
  {
    send_insufficient_bytes_error_response(2);
    return; 
  }
  
  // TODO(future): support dynmically configured heater thermistors
  send_app_error_response(PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER,0);
}
  
void handle_set_heater_target_temperature_order()
{
  const uint8_t heater_number = parameter_value[0];
  const int16_t temp = (parameter_value[1] << 8) & parameter_value[2];

  if (parameter_length < 3)
  {
    send_insufficient_bytes_error_response(3);
    return; 
  }

  if (!Device_Heater::IsInUse(heater_number))
  {
    send_app_error_response(PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER,0);
    return;
  }

  // TODO - generate error message
  if (!Device_Heater::ValidateTargetTemperature(heater_number, temp))
  {
    send_app_error_response(PARAM_APP_ERROR_TYPE_FAILED,0);
    return;
  }
  
  Device_Heater::SetTargetTemperature(heater_number, temp);
  
  send_OK_response();
}  
 
void handle_get_input_switch_state_order()
{
  uint8_t device_type, device_number;

  if (parameter_length < 2)
  {
    send_insufficient_bytes_error_response(2);
    return; 
  }
  if ((parameter_length & 1) == 1)
  {
    send_app_error_response(PARAM_APP_ERROR_TYPE_BAD_PARAMETER_FORMAT,parameter_length);
    return; 
  }

  generate_response_start(RSP_OK,parameter_length/2);
  
  for (int i = 0; i < parameter_length; i+=2)
  {
    device_type = parameter_value[i];
    device_number = parameter_value[i+1];
    
    switch(device_type)
    {
    case PM_DEVICE_TYPE_SWITCH_INPUT:
    {
      if (!Device_InputSwitch::IsInUse(device_number))
      {
        send_app_error_response(PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER, i+1);
        return;
      }
      generate_response_data_addbyte(Device_InputSwitch::ReadState(device_number));
      break;
    }
    default:
      send_app_error_response(PARAM_APP_ERROR_TYPE_INVALID_DEVICE_TYPE, i);
      return;
    }
  }  
  
  generate_response_send();
}
  
void handle_set_output_switch_state_order()
{
  uint8_t device_type, device_number, device_state;

  if (parameter_length < 3)
  {
    send_insufficient_bytes_error_response(3);
    return; 
  }

  for (uint8_t i = 0; i < parameter_length; i+=3)
  {
    if (i + 3 > parameter_length)
    {
      send_app_error_response(PARAM_APP_ERROR_TYPE_BAD_PARAMETER_FORMAT,parameter_length);
      return; 
    }
    
    device_type = parameter_value[i];
    device_number = parameter_value[i+1];
    
    switch(device_type)
    {
    case PM_DEVICE_TYPE_SWITCH_OUTPUT:
    {
      if (!Device_OutputSwitch::IsInUse(device_number))
      {
        send_app_error_response(PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER, i+1);
        return;
      }
      break;
    }
    default:
      send_app_error_response(PARAM_APP_ERROR_TYPE_INVALID_DEVICE_TYPE,i);
      return;
    }
  }  

  // we only write the switches if all are valid
  for (uint8_t i = 0; i < parameter_length; i+=3)
  {
    device_type = parameter_value[i];
    device_number = parameter_value[i+1];
    device_state = parameter_value[i+2];
    
    switch(device_type)
    {
    case PM_DEVICE_TYPE_SWITCH_OUTPUT:
      Device_OutputSwitch::WriteState(device_number, device_state);
      break;
    }
  }
  
  send_OK_response();
}
   
void handle_set_pwm_output_state_order()
{
  uint8_t device_type, device_number;
  uint16_t device_state;
  
  if (parameter_length < 4)
  {
    send_insufficient_bytes_error_response(4);
    return; 
  }

  // TODO consider only allowing a singe write
  for (int i = 0; i < parameter_length; i+=4)
  {
    if (i + 4 > parameter_length)
    {
      send_app_error_response(PARAM_APP_ERROR_TYPE_BAD_PARAMETER_FORMAT,parameter_length);
      return; 
    }
    
    device_type = parameter_value[i];
    device_number = parameter_value[i+1];
    device_state = (parameter_value[i+2]<<8) & parameter_value[i+3];
    
    switch(device_type)
    {
    case PM_DEVICE_TYPE_PWM_OUTPUT:
    {
      if (!Device_PwmOutput::IsInUse(device_number))
      {
        send_app_error_response(PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER, i+1);
        return;
      }
      Device_PwmOutput::WriteState(device_number, device_state);
      break;
    }
    case PM_DEVICE_TYPE_BUZZER:
    {
      if (!Device_Buzzer::IsInUse(device_number))
      {
        send_app_error_response(PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER, i+1);
        return;
      }
      Device_Buzzer::WriteState(device_number, device_state);
      break;
    }
    default:
      send_app_error_response(PARAM_APP_ERROR_TYPE_INVALID_DEVICE_TYPE,i);
      return;
    }
  }  
  
  send_OK_response();
}
 
void handle_write_firmware_configuration_value_order()
{
  const uint8_t name_length = parameter_value[0];
  
  if (parameter_length < name_length + 1)
  {
    send_insufficient_bytes_error_response(name_length + 1);
    return; 
  }

  // make name null terminated
  memmove(&parameter_value[0], &parameter_value[1], name_length);
  parameter_value[name_length] = '\0';

  // this function will handle response generation
  // note: get_command() already makes the end of the command (ie. value) null-terminated
  handle_firmware_configuration_request((const char *)&parameter_value[0], (const char *)&parameter_value[name_length+1]);
} 
 
void handle_clear_command_queue_order()
{
  generate_response_start(RSP_APPLICATION_ERROR, 1);
  
  if (CommandQueue::GetQueueBufferLength() == 0)
  {  
    if (!allocate_command_queue_memory())
    {
      generate_response_data_addbyte(PARAM_APP_ERROR_TYPE_FIRMWARE_ERROR);
      generate_response_msg_addPGM(PMSG(MSG_ERR_INSUFFICIENT_MEMORY));
      generate_response_send(); 
      return;
    }
  }

  CommandQueue::FlushQueuedCommands();
  
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
