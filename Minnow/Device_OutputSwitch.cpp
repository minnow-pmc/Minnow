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
// Implementation for output switch devices
//

#include "Device_OutputSwitch.h"
#include "response.h"

extern uint8_t checkDigitalPin(uint8_t pin);

uint8_t Device_OutputSwitch::num_output_switches = 0;
uint8_t *Device_OutputSwitch::output_switch_pins;
bool *Device_OutputSwitch::output_switch_disabled;

//
// Methods
//

uint8_t Device_OutputSwitch::Init(uint8_t num_devices)
{
  if (num_output_switches != 0)
  {
    generate_response_msg_addPGM(PMSG(MSG_ERR_ALREADY_INITIALIZED));
    return PARAM_APP_ERROR_TYPE_FAILED;
  }
  if (num_devices == 0)
    return APP_ERROR_TYPE_SUCCESS;

  uint8_t *memory = (uint8_t*)malloc(num_devices * sizeof(*output_switch_pins));
  if (memory == 0)
  {
    generate_response_msg_addPGM(PMSG(MSG_ERR_INSUFFICIENT_MEMORY));
    return PARAM_APP_ERROR_TYPE_FAILED;
  }

  output_switch_pins = memory;
  
  memset(output_switch_pins, 0xFF, num_devices * sizeof(*output_switch_pins));

  num_output_switches = num_devices;
  
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t Device_OutputSwitch::SetPin(uint8_t device_number, uint8_t pin)
{
  if (device_number >= num_output_switches)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;
  
  uint8_t retval = checkDigitalPin(pin);
  if (retval != APP_ERROR_TYPE_SUCCESS)
    return retval;
  
  output_switch_pins[device_number] = pin;
  output_switch_disabled[device_number] = true;

  return APP_ERROR_TYPE_SUCCESS;
}



    