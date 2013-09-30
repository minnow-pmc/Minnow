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
// Firmware Configuration 
//

#define __STDC_LIMIT_MACROS

#include "firmware_configuration.h"

#include "ConfigTree.h"
#include "Minnow.h"
#include "response.h"
#include "NVConfigStore.h"
#include "CommandQueue.h"

#include "Device_InputSwitch.h"
#include "Device_OutputSwitch.h"
#include "Device_PwmOutput.h"
#include "Device_Buzzer.h"
#include "Device_Heater.h"
#include "Device_Stepper.h"
#include "Device_TemperatureSensor.h"

FORCE_INLINE static void generate_value(uint8_t node_type, uint8_t parent_instance_id,  uint8_t instance_id);
FORCE_INLINE static bool set_uint8_value(uint8_t node_type, uint8_t parent_instance_id,  uint8_t instance_id, uint8_t value);
FORCE_INLINE static bool set_int16_value(uint8_t node_type, uint8_t parent_instance_id,  uint8_t instance_id, int16_t value);
FORCE_INLINE static bool set_bool_value(uint8_t node_type, uint8_t parent_instance_id, uint8_t instance_id, bool value);
FORCE_INLINE static bool set_string_value(uint8_t node_type, uint8_t parent_instance_id,  uint8_t instance_id, const char *value);
FORCE_INLINE static bool setPin(uint8_t node_type, uint8_t device_number, uint8_t pin);

static bool read_number(long &number, const char *value);

void handle_firmware_configuration_traversal(const char *name)
{
  ConfigurationTree tree;

  ConfigurationTreeNode *node;
  if (name[0] == '\0')
  {
    node = tree.FindFirstLeafNode(tree.GetRootNode());
  }
  else
  {
    node = tree.FindNode(name);
    if (node == 0)
    {
      send_app_error_response(PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE,
                        PMSG(ERR_MSG_CONFIG_NODE_NOT_FOUND), name);
      return;
    }
    // currently only return nodes for devices which are in use
    node = tree.FindNextLeafNode(tree.GetRootNode());
  }
  
  if (node == 0)
  {
    generate_response_start(RSP_OK);
    generate_response_data_addbyte(LEAF_CLASS_INVALID);
    generate_response_data_addbyte(0);
    generate_response_send(); 
    return;
  }

  generate_response_start(RSP_OK);

  generate_response_data_addbyte(node->GetLeafClass());
  generate_response_data_addbyte(node->GetLeafOperations());
  
  char *response_data_buf = (char *)generate_response_data_ptr();
  uint8_t response_data_buf_len = generate_response_data_len();
  int8_t length;

  if ((length = tree.GetFullName(node, response_data_buf, response_data_buf_len)) > 0)
  {
    generate_response_data_addlen(length);
  }
  else
  {
    send_app_error_response(PARAM_APP_ERROR_TYPE_FIRMWARE_ERROR, 0);
    return;
  }      
  generate_response_send();
}

void handle_firmware_configuration_request(const char *name, const char* value)
{
  ConfigurationTree tree;
  
  if (*name == '\0')
  {
    send_app_error_response(PARAM_APP_ERROR_TYPE_BAD_PARAMETER_FORMAT, 0);
    return; 
  }

  ConfigurationTreeNode *node = tree.FindNode(name);
  if (node == 0)
  {
    // TODO: improve on this error message
    send_app_error_response(PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE,
                      PMSG(ERR_MSG_CONFIG_NODE_NOT_FOUND), name);
    return;
  }

  if (!node->IsLeafNode())
  {
    // TODO: improve on this error message
    send_app_error_response(PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE,
                      PMSG(ERR_MSG_CONFIG_NODE_NOT_COMPLETE), name);
    return;
  }

  if (value == 0)
  {
    if ((node->GetLeafOperations() & LEAF_OPERATIONS_READABLE) == 0)
    {
      send_app_error_response(PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE,
                      PMSG(ERR_MSG_CONFIG_NODE_NOT_READABLE));
      return;
    }
    generate_value(node->GetNodeType(), tree.GetParentNode(node)->GetInstanceId(), node->GetInstanceId());
  }
  else
  {
    if ((node->GetLeafOperations() & LEAF_OPERATIONS_WRITEABLE) == 0)
    {
      send_app_error_response(PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE,
                      PMSG(ERR_MSG_CONFIG_NODE_NOT_WRITEABLE));
      return;
    }
    
    generate_response_start(RSP_APPLICATION_ERROR, 1);

    // ensure value has correct format and then attempt to set
    switch (node->GetLeafSetDataType())
    {
    case LEAF_SET_DATATYPE_UINT8:
    {
      long number;
      if (!read_number(number, value) 
        || number < 0 || number > UINT8_MAX)
      {
        send_app_error_response(PARAM_APP_ERROR_TYPE_BAD_PARAMETER_FORMAT,
                                PMSG(ERR_MSG_EXPECTED_UINT8_VALUE));
        return;
      }

      if (!set_uint8_value(node->GetNodeType(), tree.GetParentNode(node)->GetInstanceId(), 
          node->GetInstanceId(), number))
      {
        return; // assume that set function has generated an error response
      }
      break;
    }
    
    case LEAF_SET_DATATYPE_INT16:
    {
      long number;
      if (!read_number(number, value))
      {
        send_app_error_response(PARAM_APP_ERROR_TYPE_BAD_PARAMETER_FORMAT,
                                PMSG(ERR_MSG_EXPECTED_INT16_VALUE));
        return;
      }

      if (!set_int16_value(node->GetNodeType(), tree.GetParentNode(node)->GetInstanceId(), 
          node->GetInstanceId(), number))
      {
        return; // assume that set function has generated an error response
      }
      break;
    }    
    
    case LEAF_SET_DATATYPE_BOOL:
    {
      bool val;
      if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0)
      {
        val = true;
      }
      else if (strcmp(value, "false") == 0 || strcmp(value, "0") == 0)
      {
        val = false;
      }
      else
      {
        send_app_error_response(PARAM_APP_ERROR_TYPE_BAD_PARAMETER_FORMAT,
                                PMSG(ERR_MSG_EXPECTED_BOOL_VALUE));
        return;
      }
      
      if (!set_bool_value(node->GetNodeType(), tree.GetParentNode(node)->GetInstanceId(), 
          node->GetInstanceId(), val))
      {
        return; // assume that set function has generated an error response
      }
      break;
    }
    
    case LEAF_SET_DATATYPE_STRING:
    {
      if (!set_string_value(node->GetNodeType(), tree.GetParentNode(node)->GetInstanceId(), 
          node->GetInstanceId(), value))
      {
        return; // assume that set function has generated an error response
      }
      break;
    }
    
    default:
      send_app_error_response(PARAM_APP_ERROR_TYPE_BAD_PARAMETER_FORMAT,
                              PMSG(MSG_ERR_CANNOT_HANDLE_FIRMWARE_CONFIG_REQUEST), __LINE__);
      return;
    }

    send_OK_response();
  }
}

void generate_value(uint8_t node_type, uint8_t parent_instance_id,  uint8_t instance_id)
{
    generate_response_start(RSP_OK);
    char *response_data_buf = (char *)generate_response_data_ptr();
    uint8_t response_data_buf_len = generate_response_data_len();
    int8_t length;

    switch(node_type)
    {

    case NODE_TYPE_CONFIG_LEAF_INPUT_SWITCH_FRIENDLY_NAME:
      if ((length = NVConfigStore::GetDeviceName(PM_DEVICE_TYPE_SWITCH_INPUT, parent_instance_id, response_data_buf, response_data_buf_len)) > 0)
        generate_response_data_addlen(length);
      break;
    case NODE_TYPE_CONFIG_LEAF_INPUT_SWITCH_PIN:
      utoa(Device_InputSwitch::GetPin(parent_instance_id), response_data_buf, 10);
      generate_response_data_addlen(strlen(response_data_buf));
      break;
      
    case NODE_TYPE_CONFIG_LEAF_OUTPUT_SWITCH_FRIENDLY_NAME:
      if ((length = NVConfigStore::GetDeviceName(PM_DEVICE_TYPE_SWITCH_OUTPUT, instance_id, response_data_buf, response_data_buf_len)) > 0)
        generate_response_data_addlen(length);
      break;
    case NODE_TYPE_CONFIG_LEAF_OUTPUT_SWITCH_PIN:
      utoa(Device_OutputSwitch::GetPin(parent_instance_id), response_data_buf, 10);
      generate_response_data_addlen(strlen(response_data_buf));
      break;
      
    case NODE_TYPE_CONFIG_LEAF_PWM_OUTPUT_FRIENDLY_NAME:
      if ((length = NVConfigStore::GetDeviceName(PM_DEVICE_TYPE_PWM_OUTPUT, instance_id, response_data_buf, response_data_buf_len)) > 0)
        generate_response_data_addlen(length);
      break;
    case NODE_TYPE_CONFIG_LEAF_PWM_OUTPUT_PIN:
      utoa(Device_PwmOutput::GetPin(parent_instance_id), response_data_buf, 10);
      generate_response_data_addlen(strlen(response_data_buf));
      break;
      
    case NODE_TYPE_CONFIG_LEAF_BUZZER_FRIENDLY_NAME:
      if ((length = NVConfigStore::GetDeviceName(PM_DEVICE_TYPE_BUZZER, instance_id, response_data_buf, response_data_buf_len)) > 0)
        generate_response_data_addlen(length);
      break;
    case NODE_TYPE_CONFIG_LEAF_BUZZER_PIN:
      utoa(Device_Buzzer::GetPin(parent_instance_id), response_data_buf, 10);
      generate_response_data_addlen(strlen(response_data_buf));
      break;
      
    case NODE_TYPE_CONFIG_LEAF_HEATER_FRIENDLY_NAME:
      if ((length = NVConfigStore::GetDeviceName(PM_DEVICE_TYPE_HEATER, instance_id, response_data_buf, response_data_buf_len)) > 0)
        generate_response_data_addlen(length);
      break;
    case NODE_TYPE_CONFIG_LEAF_HEATER_PIN:
      utoa(Device_Heater::GetHeaterPin(parent_instance_id), response_data_buf, 10);
      generate_response_data_addlen(strlen(response_data_buf));
      break;
    // TODO add other heater config
     
    // Statistics Related
    case NODE_TYPE_STATS_LEAF_RX_PACKET_COUNT:
    {
      extern uint32_t recv_count;
      ultoa(recv_count, response_data_buf, 10);
      generate_response_data_addlen(strlen(response_data_buf));
      break;
    }
    case NODE_TYPE_STATS_LEAF_RX_ERROR_COUNT:
    {
      extern uint16_t recv_errors;
      utoa(recv_errors, response_data_buf, 10);
      generate_response_data_addlen(strlen(response_data_buf));
      break;
    }
    case NODE_TYPE_STATS_LEAF_QUEUE_MEMORY:
    {
      utoa(CommandQueue::GetQueueBufferLength(), response_data_buf, 10);
      generate_response_data_addlen(strlen(response_data_buf));
      break;
    }
    case NODE_TYPE_DEBUG_LEAF_STACK_MEMORY:
    {
      extern uint8_t *startOfStack();
      uint8_t you_are_here;;
      ultoa((unsigned long)&you_are_here - (unsigned long)startOfStack(), response_data_buf, 10);
      generate_response_data_addlen(strlen(response_data_buf));
      break;
    }
    case NODE_TYPE_DEBUG_LEAF_STACK_LOW_WATER_MARK:
    {
      extern uint16_t countStackLowWatermark();
      utoa(countStackLowWatermark(), response_data_buf, 10);
      generate_response_data_addlen(strlen(response_data_buf));
      break;
    }

    case LEAF_SET_DATATYPE_INVALID:
    default:
      send_app_error_response(PARAM_APP_ERROR_TYPE_FIRMWARE_ERROR,
              PMSG(MSG_ERR_CANNOT_HANDLE_FIRMWARE_CONFIG_REQUEST), __LINE__);
      return;
    }  

  generate_response_send();
}

 
bool set_uint8_value(uint8_t node_type, uint8_t parent_instance_id,  uint8_t instance_id, uint8_t value)
{ 
  uint8_t retval;
  switch (node_type)
  {
  case NODE_TYPE_CONFIG_LEAF_INPUT_SWITCH_PIN:
  case NODE_TYPE_CONFIG_LEAF_OUTPUT_SWITCH_PIN:
  case NODE_TYPE_CONFIG_LEAF_PWM_OUTPUT_PIN:
  case NODE_TYPE_CONFIG_LEAF_BUZZER_PIN:
  case NODE_TYPE_CONFIG_LEAF_TEMP_SENSOR_PIN:
  case NODE_TYPE_CONFIG_LEAF_HEATER_PIN:
  case NODE_TYPE_CONFIG_LEAF_STEPPER_ENABLE_PIN:
  case NODE_TYPE_CONFIG_LEAF_STEPPER_DIRECTION_PIN:
  case NODE_TYPE_CONFIG_LEAF_STEPPER_STEP_PIN:
    return setPin(node_type, parent_instance_id, value);

  case NODE_TYPE_CONFIG_LEAF_TEMP_SENSOR_TYPE:
    retval = Device_TemperatureSensor::SetType(parent_instance_id, value);
    break;
    
  case NODE_TYPE_CONFIG_LEAF_SYSTEM_HARDWARE_TYPE:
    retval = NVConfigStore::SetHardwareType(value);
    break;
  case NODE_TYPE_CONFIG_LEAF_SYSTEM_HARDWARE_REV:
    retval = NVConfigStore::SetHardwareRevision(value);
    break;
    
  case NODE_TYPE_CONFIG_LEAF_SYSTEM_NUM_INPUT_SWITCHES:
    retval = Device_InputSwitch::Init(value);
    break;
  case NODE_TYPE_CONFIG_LEAF_SYSTEM_NUM_OUTPUT_SWITCHES:
    retval = Device_OutputSwitch::Init(value);
    break;
  case NODE_TYPE_CONFIG_LEAF_SYSTEM_NUM_PWM_OUTPUTS:
    retval = Device_PwmOutput::Init(value);
    break;
  case NODE_TYPE_CONFIG_LEAF_SYSTEM_NUM_BUZZERS:
    retval = Device_Buzzer::Init(value);
    break;
  case NODE_TYPE_CONFIG_LEAF_SYSTEM_NUM_TEMP_SENSORS:
    retval = Device_TemperatureSensor::Init(value);
    break;
  case NODE_TYPE_CONFIG_LEAF_SYSTEM_NUM_HEATERS:
    retval = Device_Heater::Init(value);
    break;
  case NODE_TYPE_CONFIG_LEAF_SYSTEM_NUM_STEPPERS:
    retval = Device_Stepper::Init(value);
    break;
      
  case NODE_TYPE_CONFIG_LEAF_HEATER_TEMP_SENSOR:
    retval = Device_Heater::SetTempSensor(parent_instance_id, value);
    break;
  case NODE_TYPE_CONFIG_LEAF_HEATER_BANG_BANG_HYSTERESIS:
    if (value > 25)
      retval = PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE;
    else
      retval = Device_Heater::SetBangBangHysteresis(parent_instance_id, value * 10);
    break;

  default: 
    send_app_error_response(PARAM_APP_ERROR_TYPE_FIRMWARE_ERROR,
         PMSG(MSG_ERR_CANNOT_HANDLE_FIRMWARE_CONFIG_REQUEST), __LINE__);
    return false;
  }
  
  if (retval == APP_ERROR_TYPE_SUCCESS)
    return true;
  
  generate_response_data_addbyte(retval);
  generate_response_send();
  return false;
}

bool set_int16_value(uint8_t node_type, uint8_t parent_instance_id,  uint8_t instance_id, int16_t value)
{ 
  uint8_t retval;

  switch (node_type)
  {
  case NODE_TYPE_CONFIG_LEAF_HEATER_MAX_TEMP:
    retval = Device_Heater::SetMaxTemperature(parent_instance_id, value);
    break;

  default: 
    send_app_error_response(PARAM_APP_ERROR_TYPE_FIRMWARE_ERROR,
         PMSG(MSG_ERR_CANNOT_HANDLE_FIRMWARE_CONFIG_REQUEST), __LINE__);
    return false;
  }
  
  if (retval == APP_ERROR_TYPE_SUCCESS)
    return true;
  
  generate_response_data_addbyte(retval);
  generate_response_send();
  return false;
}

bool set_bool_value(uint8_t node_type, uint8_t parent_instance_id,  uint8_t instance_id, bool value)
{ 
  uint8_t retval;

  switch (node_type)
  {
  case NODE_TYPE_CONFIG_LEAF_PWM_OUTPUT_USE_SOFT_PWM:
    retval = Device_PwmOutput::EnableSoftPwm(parent_instance_id, value);
    break;
  case NODE_TYPE_CONFIG_LEAF_BUZZER_USE_SOFT_PWM:
    retval = Device_Buzzer::EnableSoftPwm(parent_instance_id, value);
    break;
  case NODE_TYPE_CONFIG_LEAF_HEATER_USE_SOFT_PWM:
    retval = Device_Heater::EnableSoftPwm(parent_instance_id, value);
    break;

  case NODE_TYPE_CONFIG_LEAF_HEATER_USE_BANG_BANG:
    retval = Device_Heater::SetControlMode(parent_instance_id, HEATER_CONTROL_MODE_BANG_BANG);
    break;
  case NODE_TYPE_CONFIG_LEAF_HEATER_USE_PID:
    retval = Device_Heater::SetControlMode(parent_instance_id, HEATER_CONTROL_MODE_PID);
    break;
    
  case NODE_TYPE_CONFIG_LEAF_STEPPER_ENABLE_INVERT:
    retval = Device_Stepper::SetEnableInvert(parent_instance_id, value);
    break;
  case NODE_TYPE_CONFIG_LEAF_STEPPER_DIRECTION_INVERT:
    retval = Device_Stepper::SetDirectionInvert(parent_instance_id, value);
    break;
  case NODE_TYPE_CONFIG_LEAF_STEPPER_STEP_INVERT:
    retval = Device_Stepper::SetStepInvert(parent_instance_id, value);
    break;

  default: 
    send_app_error_response(PARAM_APP_ERROR_TYPE_FIRMWARE_ERROR,
         PMSG(MSG_ERR_CANNOT_HANDLE_FIRMWARE_CONFIG_REQUEST), __LINE__);
    return false;
  }
  
  if (retval == APP_ERROR_TYPE_SUCCESS)
    return true;
  
  generate_response_data_addbyte(retval);
  generate_response_send();
  return false;
}

bool set_string_value(uint8_t node_type, uint8_t parent_instance_id,  uint8_t instance_id, const char *value)
{ 
  uint8_t retval;

  generate_response_start(RSP_APPLICATION_ERROR, 1);
  
  switch (node_type)
  {
  case NODE_TYPE_CONFIG_LEAF_INPUT_SWITCH_FRIENDLY_NAME:
    retval = NVConfigStore::SetDeviceName(PM_DEVICE_TYPE_SWITCH_INPUT, parent_instance_id, value);
    break;
  case NODE_TYPE_CONFIG_LEAF_OUTPUT_SWITCH_FRIENDLY_NAME:
    retval = NVConfigStore::SetDeviceName(PM_DEVICE_TYPE_SWITCH_OUTPUT, parent_instance_id, value);
    break;
  case NODE_TYPE_CONFIG_LEAF_PWM_OUTPUT_FRIENDLY_NAME:
    retval = NVConfigStore::SetDeviceName(PM_DEVICE_TYPE_PWM_OUTPUT, parent_instance_id, value);
    break;
  case NODE_TYPE_CONFIG_LEAF_BUZZER_FRIENDLY_NAME:
    retval = NVConfigStore::SetDeviceName(PM_DEVICE_TYPE_BUZZER, parent_instance_id, value);
    break;
  case NODE_TYPE_CONFIG_LEAF_TEMP_SENSOR_FRIENDLY_NAME:
    retval = NVConfigStore::SetDeviceName(PM_DEVICE_TYPE_TEMP_SENSOR, parent_instance_id, value);
    break;
  case NODE_TYPE_CONFIG_LEAF_HEATER_FRIENDLY_NAME:
    retval = NVConfigStore::SetDeviceName(PM_DEVICE_TYPE_HEATER, parent_instance_id, value);
    break;
  case NODE_TYPE_CONFIG_LEAF_STEPPER_FRIENDLY_NAME:
    retval = NVConfigStore::SetDeviceName(PM_DEVICE_TYPE_HEATER, parent_instance_id, value);
    break;

  case NODE_TYPE_CONFIG_LEAF_SYSTEM_HARDWARE_NAME:
    retval = NVConfigStore::SetHardwareName(value);
    break;
  case NODE_TYPE_CONFIG_LEAF_SYSTEM_BOARD_IDENTITY:
    retval = NVConfigStore::SetBoardIdentity(value);
    break;
  case NODE_TYPE_CONFIG_LEAF_SYSTEM_BOARD_SERIAL_NUM:
    retval = NVConfigStore::SetBoardSerialNumber(value);
    break;
    
  default: 
    send_app_error_response(PARAM_APP_ERROR_TYPE_FIRMWARE_ERROR,
         PMSG(MSG_ERR_CANNOT_HANDLE_FIRMWARE_CONFIG_REQUEST), __LINE__);
    return false;
  }
  
  if (retval == APP_ERROR_TYPE_SUCCESS)
    return true;
  
  generate_response_data_addbyte(retval);
  generate_response_send();
  return false;
}

bool setPin(uint8_t node_type, uint8_t device_number, uint8_t pin)
{
  uint8_t retval = false;
  
  switch (node_type)
  {
  case NODE_TYPE_CONFIG_LEAF_INPUT_SWITCH_PIN:
    retval = Device_InputSwitch::SetPin(device_number, pin);
    break;
    
  case NODE_TYPE_CONFIG_LEAF_OUTPUT_SWITCH_PIN:
    retval = Device_OutputSwitch::SetPin(device_number, pin);
    break;
    
  case NODE_TYPE_CONFIG_LEAF_PWM_OUTPUT_PIN:
    retval = Device_PwmOutput::SetPin(device_number, pin);
    break;
    
  case NODE_TYPE_CONFIG_LEAF_BUZZER_PIN:
    retval = Device_Buzzer::SetPin(device_number, pin);
    break;
    
  case NODE_TYPE_CONFIG_LEAF_TEMP_SENSOR_PIN:
    retval = Device_TemperatureSensor::SetPin(device_number, pin);
    break;
    
  case NODE_TYPE_CONFIG_LEAF_HEATER_PIN:
    retval = Device_Heater::SetHeaterPin(device_number, pin);
    break;

  case NODE_TYPE_CONFIG_LEAF_STEPPER_ENABLE_PIN:
    retval = Device_Stepper::SetEnablePin(device_number, pin);
    break;

  case NODE_TYPE_CONFIG_LEAF_STEPPER_DIRECTION_PIN:
    retval = Device_Stepper::SetDirectionPin(device_number, pin);
    break;

  case NODE_TYPE_CONFIG_LEAF_STEPPER_STEP_PIN:
    retval = Device_Stepper::SetStepPin(device_number, pin);
    break;

  default: 
    send_app_error_response(PARAM_APP_ERROR_TYPE_FIRMWARE_ERROR,
         PMSG(MSG_ERR_CANNOT_HANDLE_FIRMWARE_CONFIG_REQUEST), __LINE__);
    return false;
  }    
  
  if (retval == APP_ERROR_TYPE_SUCCESS)
    return true;

  generate_response_data_addbyte(retval);
  generate_response_msg_addPGM(PMSG(ERR_MSG_INVALID_PIN_NUMBER));
  generate_response_send();
  return false;
}

//
// Utility Functions
//

bool read_number(long &number, const char *value)
{
  char *end;
  number = strtol(value, &end, 10); 
  if (end == value || *end != '\0')
    return false;
  return true;
}

uint8_t checkDigitalPin(uint8_t pin)
{
  if (digitalPinToPort(pin) == NOT_A_PIN)
  {
    generate_response_msg_addPGM(PMSG(ERR_MSG_INVALID_PIN_NUMBER));
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE;
  }
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t checkAnalogOrDigitalPin(uint8_t pin)
{
  if (digitalPinToTimer(pin) == NOT_ON_TIMER && digitalPinToPort(pin) == NOT_A_PIN)
  {
    generate_response_msg_addPGM(PMSG(ERR_MSG_INVALID_PIN_NUMBER));
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE;
  }
  return APP_ERROR_TYPE_SUCCESS;
}
