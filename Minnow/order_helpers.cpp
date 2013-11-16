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
// This file provides shared helper/utility functions for handling orders
// 
 
#include "order_helpers.h" 
#include "Minnow.h" 
#include "protocol.h"
#include "response.h"
#include "NVConfigStore.h"

#include "Device_InputSwitch.h"
#include "Device_OutputSwitch.h"
#include "Device_PwmOutput.h" 
#include "Device_Heater.h" 
#include "Device_Buzzer.h" 
#include "Device_Stepper.h" 
#include "Device_TemperatureSensor.h" 

//===========================================================================
//=============================imported variables============================
//===========================================================================


//===========================================================================
//=============================private variables=============================
//===========================================================================


//===========================================================================
//=============================public variables=============================
//===========================================================================


//===========================================================================
//=============================ROUTINES=============================
//===========================================================================


int8_t get_num_devices(uint8_t device_type)
{
  switch(device_type)
  {
  case PM_DEVICE_TYPE_SWITCH_INPUT: return Device_InputSwitch::GetNumDevices();
  case PM_DEVICE_TYPE_SWITCH_OUTPUT: return Device_OutputSwitch::GetNumDevices();
  case PM_DEVICE_TYPE_PWM_OUTPUT: return Device_PwmOutput::GetNumDevices();
  case PM_DEVICE_TYPE_STEPPER: return Device_Stepper::GetNumDevices();
  case PM_DEVICE_TYPE_HEATER: return Device_Heater::GetNumDevices();
  case PM_DEVICE_TYPE_TEMP_SENSOR: return Device_TemperatureSensor::GetNumDevices();
  case PM_DEVICE_TYPE_BUZZER: return Device_Buzzer::GetNumDevices();
  default:
    return -1;
  }
}
 
bool is_pin_in_use(uint8_t pin)
{
  uint8_t j;
  for (j = 0; j < Device_InputSwitch::GetNumDevices(); j++)
  {
    if (Device_InputSwitch::GetPin(j) == pin)
      return true;
  }
  for (j = 0; j < Device_OutputSwitch::GetNumDevices(); j++)
  {
    if (Device_OutputSwitch::GetPin(j) == pin)
      return true;
  }
  for (j = 0; j < Device_PwmOutput::GetNumDevices(); j++)
  {
    if (Device_PwmOutput::GetPin(j) == pin)
      return true;
  }
  for (j = 0; j < Device_Stepper::GetNumDevices(); j++)
  {
    if (Device_Stepper::GetEnablePin(j) == pin || Device_Stepper::GetDirectionPin(j) == pin
        || Device_Stepper::GetStepPin(j) == pin)
      return true;
  }
  for (j = 0; j < Device_Heater::GetNumDevices(); j++)
  {
    if (Device_Heater::GetHeaterPin(j) == pin)
      return true;
  }
  for (j = 0; j < Device_Buzzer::GetNumDevices(); j++)
  {
    if (Device_Buzzer::GetPin(j) == pin)
      return true;
  }
  for (j = 0; j < Device_TemperatureSensor::GetNumDevices(); j++)
  {
    if (Device_TemperatureSensor::GetPin(j) == pin)
      return true;
  }
  return false;
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
