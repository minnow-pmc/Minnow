/*
 Minnow Pacemaker client firmware.
    
 Copyright (C) 2013 Robert Fairlie-Cuninghame

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHIN ANY WARRANTY; within even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//
// Interface for input switch devices
//

#include "Device_InputSwitch.h"
#include "response.h"

uint8_t Device_InputSwitch::num_input_switches = 0;
uint8_t *Device_InputSwitch::input_switch_pins;

//
// Methods
//

uint8_t Device_InputSwitch::Init(uint8_t num_devices)
{
  if (num_input_switches != 0)
  {
    generate_response_msg_addPGM(PMSG(MSG_ERR_ALREADY_INITIALIZED));
    return PARAM_APP_ERROR_TYPE_FAILED;
  }
  if (num_devices == 0)
    return APP_ERROR_TYPE_SUCCESS;

  uint8_t *memory = (uint8_t*)malloc(num_devices * sizeof(*input_switch_pins));
  if (memory == 0)
  {
    generate_response_msg_addPGM(PMSG(MSG_ERR_INSUFFICIENT_MEMORY));
    return PARAM_APP_ERROR_TYPE_FAILED;
  }

  input_switch_pins = memory;
  
  memset(input_switch_pins, 0xFF, num_devices * sizeof(*input_switch_pins));

  num_input_switches = num_devices;
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t Device_InputSwitch::SetPin(uint8_t device_number, uint8_t pin)
{
  if (device_number >= num_input_switches)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;
  
  if (digitalPinToPort(pin) == NOT_A_PIN)
  {
    generate_response_msg_addPGM(PMSG(ERR_MSG_INVALID_PIN_NUMBER));
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE;
  }
  
  input_switch_pins[device_number] = pin;

  pinMode(pin, INPUT);

  return APP_ERROR_TYPE_SUCCESS;
}


    