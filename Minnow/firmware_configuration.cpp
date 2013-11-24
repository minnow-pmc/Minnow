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
#include "initial_pin_state.h"

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
FORCE_INLINE static bool set_float_value(uint8_t node_type, uint8_t parent_instance_id,  uint8_t instance_id, float value);
FORCE_INLINE static bool setPin(uint8_t node_type, uint8_t device_number, uint8_t pin);

static bool read_number(long &number, const char *value);

void handle_firmware_configuration_value_properties(const char *name)
{
  ConfigurationTree tree;

  if (name[0] == '\0')
  {
    send_app_error_response(PARAM_APP_ERROR_TYPE_BAD_PARAMETER_FORMAT,
                      PMSG(ERR_MSG_CONFIG_NODE_NOT_FOUND));
    return;
  }
  
  ConfigurationTreeNode *node = tree.FindNode(name);
  if (node == 0)
  {
    send_app_error_response(PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE,
                      PMSG(ERR_MSG_CONFIG_NODE_NOT_FOUND), name);
    return;
  }

  if (!node->IsLeafNode())
  {
    send_app_error_response(PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE,
                      PMSG(ERR_MSG_CONFIG_NODE_NOT_COMPLETE), name);
    return;
  }
  
  generate_response_start(RSP_OK, 2);
  generate_response_data_addbyte(node->GetLeafClass());
  generate_response_data_addbyte(node->GetLeafOperations());

  // determine default value status here where necessary.

  // determine whether configuration node is associated with a device
  // (we search for a parent instance node where the device type has been set)
  uint8_t device_type = PM_DEVICE_TYPE_INVALID;
  uint8_t device_number = 0;
  while ((node = tree.GetParentNode(node)) != 0)
  {
    if (node->IsInstanceNode() 
        && node->GetLeafSetDataType() != PM_DEVICE_TYPE_INVALID)
    {
      device_type = node->GetLeafSetDataType();
      device_number = node->GetInstanceId();
    }
  }
  generate_response_data_addbyte(device_type);
  generate_response_data_addbyte(device_number);
  
  generate_response_send();
}  

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
    send_OK_response(); 
    return;
  }

  generate_response_start(RSP_OK);

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
    if ((node->GetLeafOperations() & FIRMWARE_CONFIG_OPS_READABLE) == 0)
    {
      send_app_error_response(PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE,
                      PMSG(ERR_MSG_CONFIG_NODE_NOT_READABLE));
      return;
    }
    generate_value(node->GetNodeType(), tree.GetParentNode(node)->GetInstanceId(), node->GetInstanceId());
  }
  else
  {
    if ((node->GetLeafOperations() & FIRMWARE_CONFIG_OPS_WRITEABLE) == 0)
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
    
    case LEAF_SET_DATATYPE_FLOAT:
    {
      char *end;
      float val = strtod(value, &end);
      if (end == value || *end != '\0')
      {
        send_app_error_response(PARAM_APP_ERROR_TYPE_BAD_PARAMETER_FORMAT,
                                PMSG(ERR_MSG_EXPECTED_FLOAT_VALUE));
        return;
      }

      if (!set_float_value(node->GetNodeType(), tree.GetParentNode(node)->GetInstanceId(), 
          node->GetInstanceId(), val))
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
    uint8_t value;

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
    case NODE_TYPE_CONFIG_LEAF_INPUT_SWITCH_ENABLE_PULLUP:
      generate_response_data_addbyte(Device_InputSwitch::GetEnablePullup(parent_instance_id) ? '1' : '0');
      break;
      
    case NODE_TYPE_CONFIG_LEAF_OUTPUT_SWITCH_FRIENDLY_NAME:
      if ((length = NVConfigStore::GetDeviceName(PM_DEVICE_TYPE_SWITCH_OUTPUT, instance_id, response_data_buf, response_data_buf_len)) > 0)
        generate_response_data_addlen(length);
      break;
    case NODE_TYPE_CONFIG_LEAF_OUTPUT_SWITCH_PIN:
      utoa(Device_OutputSwitch::GetPin(parent_instance_id), response_data_buf, 10);
      generate_response_data_addlen(strlen(response_data_buf));
      break;
    case NODE_TYPE_CONFIG_LEAF_OUTPUT_SWITCH_INITIAL_STATE:
      value = Device_OutputSwitch::GetInitialState(instance_id);
      strncpy_P(response_data_buf, stringify_initial_pin_state_value(value), response_data_buf_len);
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
    case NODE_TYPE_CONFIG_LEAF_PWM_OUTPUT_USE_SOFT_PWM:
      generate_response_data_addbyte(Device_PwmOutput::GetSoftPwmState(parent_instance_id) ? '1' : '0');
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
    case NODE_TYPE_CONFIG_LEAF_HEATER_USE_SOFT_PWM:
      generate_response_data_addbyte(Device_Heater::GetSoftPwmState(parent_instance_id) ? '1' : '0');
      break;
    case NODE_TYPE_CONFIG_LEAF_HEATER_USE_BANG_BANG:
      generate_response_data_addbyte(
        (Device_Heater::GetControlMode(parent_instance_id) == HEATER_CONTROL_MODE_BANG_BANG) ? '1' : '0');
      break;
    case NODE_TYPE_CONFIG_LEAF_HEATER_USE_PID:
      generate_response_data_addbyte(
        (Device_Heater::GetControlMode(parent_instance_id) == HEATER_CONTROL_MODE_PID) ? '1' : '0');
      break;
    case NODE_TYPE_CONFIG_LEAF_HEATER_BANG_BANG_HYSTERESIS:
      if (Device_Heater::GetControlMode(parent_instance_id) == HEATER_CONTROL_MODE_BANG_BANG)
        generate_response_data_addbyte(Device_Heater::GetBangBangHysteresis(parent_instance_id));
      break;
    case NODE_TYPE_CONFIG_LEAF_HEATER_PID_FUNCTIONAL_RANGE:
      if (Device_Heater::GetControlMode(parent_instance_id) == HEATER_CONTROL_MODE_PID)
        generate_response_data_addbyte(Device_Heater::GetPidFunctionalRange(parent_instance_id));
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
      
  case NODE_TYPE_CONFIG_LEAF_OUTPUT_SWITCH_INITIAL_STATE:
    retval = Device_OutputSwitch::SetInitialState(parent_instance_id, value);
    break;

  case NODE_TYPE_CONFIG_LEAF_HEATER_TEMP_SENSOR:
    retval = Device_Heater::SetTempSensor(parent_instance_id, value);
    break;
  case NODE_TYPE_CONFIG_LEAF_HEATER_POWER_ON_LEVEL:
    retval = Device_Heater::SetPowerOnLevel(parent_instance_id, value);
    break;
  case NODE_TYPE_CONFIG_LEAF_HEATER_BANG_BANG_HYSTERESIS:
    retval = Device_Heater::SetBangBangHysteresis(parent_instance_id, value);
    break;
  case NODE_TYPE_CONFIG_LEAF_HEATER_PID_FUNCTIONAL_RANGE:
    retval = Device_Heater::SetPidFunctionalRange(parent_instance_id, value);
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

  case NODE_TYPE_CONFIG_LEAF_TEMP_SENSOR_TYPE:
    retval = Device_TemperatureSensor::SetType(parent_instance_id, value);
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
  case NODE_TYPE_CONFIG_LEAF_INPUT_SWITCH_ENABLE_PULLUP:
    retval = Device_InputSwitch::SetEnablePullup(parent_instance_id, value);
    break;
  
  case NODE_TYPE_CONFIG_LEAF_PWM_OUTPUT_USE_SOFT_PWM:
    retval = Device_PwmOutput::EnableSoftPwm(parent_instance_id, value);
    break;
  case NODE_TYPE_CONFIG_LEAF_HEATER_USE_SOFT_PWM:
    retval = Device_Heater::EnableSoftPwm(parent_instance_id, value);
    break;

  case NODE_TYPE_CONFIG_LEAF_HEATER_USE_BANG_BANG:
    if (value)
      retval = Device_Heater::SetControlMode(parent_instance_id, HEATER_CONTROL_MODE_BANG_BANG);
    else
      retval = APP_ERROR_TYPE_SUCCESS;
    break;
  case NODE_TYPE_CONFIG_LEAF_HEATER_USE_PID:
    if (value)
      retval = Device_Heater::SetControlMode(parent_instance_id, HEATER_CONTROL_MODE_PID);
    else
      retval = APP_ERROR_TYPE_SUCCESS;
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
    retval = NVConfigStore::SetDeviceName(PM_DEVICE_TYPE_STEPPER, parent_instance_id, value);
    break;

  case NODE_TYPE_CONFIG_LEAF_OUTPUT_SWITCH_INITIAL_STATE:
  {
    uint8_t pin_state;
    if (!parse_initial_pin_state_value(value, &pin_state))
    {
      generate_response_data_addbyte(PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE);
      generate_response_msg_addPGM(PMSG(MSG_ERR_UNKNOWN_VALUE));
      generate_response_send();
      return false;
    }
    retval = Device_OutputSwitch::SetInitialState(parent_instance_id, pin_state);
    break;
  }
  case NODE_TYPE_CONFIG_LEAF_SYSTEM_HARDWARE_NAME:
    retval = NVConfigStore::SetHardwareName(value);
    break;
  case NODE_TYPE_CONFIG_LEAF_SYSTEM_BOARD_IDENTITY:
    retval = NVConfigStore::SetBoardIdentity(value);
    break;
  case NODE_TYPE_CONFIG_LEAF_SYSTEM_BOARD_SERIAL_NUM:
    retval = NVConfigStore::SetBoardSerialNumber(value);
    break;
    
  case NODE_TYPE_CONFIG_LEAF_HEATER_PID_DO_AUTOTUNE:
  {
    char *end;
    long temp, cycles;
    if ((temp = strtol(value, &end, 10)) == 0 || *end != ',' || 
        (cycles = strtol(end+1, &end, 10)) == 0 || *end != '\0')
    {
      generate_response_data_addbyte(PARAM_APP_ERROR_TYPE_BAD_PARAMETER_FORMAT);
      generate_response_msg_addPGM(PMSG(MSG_EXPECTING));
      generate_response_msg_addPGM(PMSG(ERR_MSG_BAD_PID_AUTOTUNE_FORMAT));
      generate_response_send();
      return false;
    }
    if (!Device_Heater::IsInUse(parent_instance_id)
        || !Device_TemperatureSensor::IsInUse(Device_Heater::GetTempSensor(parent_instance_id)))
    {
      send_app_error_response(PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER,
                              PMSG(ERR_MSG_DEVICE_NOT_IN_USE));
      return false;
    }
    send_OK_response(); // send response now as it takes a while to complete
    Device_Heater::DoPidAutotune(parent_instance_id, temp, cycles);
    return false;
  }  
  case NODE_TYPE_OPERATION_LEAF_RESET_EEPROM:
    NVConfigStore::WriteDefaults(true);
    retval = APP_ERROR_TYPE_SUCCESS;
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

bool set_float_value(uint8_t node_type, uint8_t parent_instance_id,  uint8_t instance_id, float value)
{ 
  uint8_t retval;

  switch (node_type)
  {
  case NODE_TYPE_CONFIG_LEAF_HEATER_PID_KP:
    retval = Device_Heater::SetPidDefaultKp(parent_instance_id, value);
    break;
  case NODE_TYPE_CONFIG_LEAF_HEATER_PID_KI:
    retval = Device_Heater::SetPidDefaultKi(parent_instance_id, value);
    break;
  case NODE_TYPE_CONFIG_LEAF_HEATER_PID_KD:
    retval = Device_Heater::SetPidDefaultKd(parent_instance_id, value);
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

void update_firmware_configuration(bool final)
{
  uint8_t i;
  
  // For the steppers, we delay pin initialization to allow the invert state
  // to be set.
  for (i = 0; i < Device_Stepper::GetNumDevices(); i++)
  {
    // we would be cleverer to only update as required but this is sufficient
    // for now. It only updates the device if the device is disabled (i.e., in
    // initial state).
    Device_Stepper::UpdateInitialPinState(i);
  }

  if (final)
  {
    // remove from the EEPROM settings any pins which have not been used anymore
    cleanup_initial_pin_state();
  }
}

void apply_initial_configuration()
{
#ifdef START_MINNOW_INITIAL_CONFIGURATION
  const char *pstr = boot_configuration_string;
  const char *pstr_end = boot_configuration_string + sizeof(boot_configuration_string);
  
  while (pstr < pstr_end)
  {
    // there are multiple substrings within the whole boot_configuration_string/.
    // find the length of the next command.
    apply_firmware_configuration_string_P(pstr);
    
    pstr += strlen_P(pstr) + 1;
  }
#endif  
}

void apply_firmware_configuration_string_P(const char *pstr)
{
  extern uint16_t recv_buf_len;
  extern uint8_t recv_buf[MAX_RECV_BUF_LEN];

  // Note: this function overwrites recv_buf contents so it is primarily intended for 
  // development & boot time usage
  char * const buf_start = (char *)&recv_buf[PM_PARAMETER_OFFSET]; // just to preserve header information 
  recv_buf_len = 0;

  // copy next command into recv_buf
  // (we use one less than the size so that the last byte of recv_buf buffer always remains zero)
  strncpy_P(buf_start, pstr, sizeof(recv_buf) - PM_PARAMETER_OFFSET - 1); 
  
  const char *value = 0;
  char *ptr = buf_start;
  char ch;
  
  // find end of name
  while ((ch = *ptr) != '\0' && ch != ' ' && ch != '=') 
    ptr += 1;

  if (ptr == buf_start || buf_start[0] == '#')
    return; // comment or blank line.
  
  // find end of name/value separator
  if (ch != '\0')
  {
    *ptr = '\0'; // make name null terminated
    
    ptr += 1;
    while ((ch = *ptr) == '\0' || ch == ' ' || ch == '=') 
      ptr += 1;
  }    
  
  value = ptr;
  
  // find end of value
  while (*ptr != '\0') 
    ptr += 1;
    
  DEBUGPGM("Applying firmware configuration: ");
  DEBUG(buf_start);
  DEBUG_CH('=');
  DEBUG(value);
  DEBUGPGM(" ... ");

  extern bool response_squelch;
  extern uint8_t reply_header[PM_HEADER_SIZE];
  extern uint8_t reply_buf[MAX_RESPONSE_PARAM_LENGTH];
  extern uint8_t reply_msg_len;

  response_squelch = true;
  handle_firmware_configuration_request(buf_start, value);
  if (reply_header[PM_ORDER_BYTE_OFFSET] == RSP_OK)
  {
    DEBUGLNPGM("ok"); 
  }
  else
  {
    DEBUGPGM("failed (code="); 
    DEBUG((int)reply_buf[0]);
    if (reply_msg_len > 0)
    {
      DEBUGPGM(" ,reason="); 
      reply_buf[reply_msg_len + 1] = '\0';
      DEBUG((char *)&reply_buf[1]);
    }
    DEBUG_CH(')');
    DEBUG_EOL();
  }
  response_squelch = false;
  
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
