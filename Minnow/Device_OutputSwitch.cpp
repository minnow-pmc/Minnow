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
#include "initial_pin_state.h"

extern uint8_t checkDigitalPin(uint8_t pin);

uint8_t Device_OutputSwitch::num_output_switches = 0;
uint8_t *Device_OutputSwitch::output_switch_pins;
bool *Device_OutputSwitch::output_switch_disabled;

//
// Methods
//

uint8_t Device_OutputSwitch::Init(uint8_t num_devices)
{
  if (num_devices == num_output_switches)
    return APP_ERROR_TYPE_SUCCESS;
  if (num_output_switches != 0)
  {
    generate_response_msg_addPGM(PMSG(MSG_ERR_ALREADY_INITIALIZED));
    return PARAM_APP_ERROR_TYPE_FAILED;
  }

  uint8_t *memory = (uint8_t*)malloc(num_devices * 
      (sizeof(*output_switch_pins) + sizeof(*output_switch_disabled)));
  if (memory == 0)
  {
    generate_response_msg_addPGM(PMSG(MSG_ERR_INSUFFICIENT_MEMORY));
    return PARAM_APP_ERROR_TYPE_FAILED;
  }

  output_switch_pins = memory;
  output_switch_disabled = (bool*)(output_switch_pins + num_devices);
  
  memset(output_switch_pins, 0xFF, num_devices * sizeof(*output_switch_pins));

  memset(output_switch_disabled, true, num_devices * sizeof(*output_switch_disabled));

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

  uint8_t initial_state = INITIAL_PIN_STATE_HIGHZ;
  (void) retrieve_initial_pin_state(pin, &initial_state);
  output_switch_disabled[device_number] =
    (initial_state == INITIAL_PIN_STATE_PULLUP
      || initial_state == INITIAL_PIN_STATE_HIGHZ);

  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t Device_OutputSwitch::SetInitialState(uint8_t device_number, uint8_t initial_state)
{
  if (device_number >= num_output_switches)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;

  uint8_t pin = output_switch_pins[device_number];
  if (pin == 0xFF)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;

  uint8_t retval = set_initial_pin_state(pin, initial_state);
  if (retval != APP_ERROR_TYPE_SUCCESS)
    return retval;

  output_switch_disabled[device_number] =
    (initial_state == INITIAL_PIN_STATE_PULLUP
      || initial_state == INITIAL_PIN_STATE_HIGHZ);
  
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t Device_OutputSwitch::GetInitialState(uint8_t device_number)
{
  if (device_number >= num_output_switches)
    return INITIAL_PIN_STATE_HIGHZ;

  uint8_t pin = output_switch_pins[device_number];
  if (pin == 0xFF)
    return INITIAL_PIN_STATE_HIGHZ;
  
  uint8_t state = INITIAL_PIN_STATE_HIGHZ;
  (void)retrieve_initial_pin_state(pin, &state);
  return state;
}



    