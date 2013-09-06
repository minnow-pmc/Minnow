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
// Implementation for pwm output devices
//

#include "Device_PwmOutput.h"
#include "response.h"

uint8_t Device_PwmOutput::num_pwm_outputs = 0;
uint8_t *Device_PwmOutput::pwm_output_pins;

uint8_t Device_PwmOutput::soft_pwm_device_bitmask;
SoftPwmState *Device_PwmOutput::soft_pwm_state;

//
// Methods
//

uint8_t Device_PwmOutput::Init(uint8_t num_devices)
{
  if (num_pwm_outputs != 0)
  {
    generate_response_msg_addPGM(PMSG(MSG_ERR_ALREADY_INITIALIZED));
    return PARAM_APP_ERROR_TYPE_FAILED;
  }
  if (num_devices == 0)
    return APP_ERROR_TYPE_SUCCESS;

  uint8_t *memory = (uint8_t*)malloc(num_devices * sizeof(*pwm_output_pins));
  if (memory == 0)
  {
    generate_response_msg_addPGM(PMSG(MSG_ERR_INSUFFICIENT_MEMORY));
    return PARAM_APP_ERROR_TYPE_FAILED;
  }

  pwm_output_pins = memory;
  
  memset(pwm_output_pins, 0xFF, num_devices * sizeof(*pwm_output_pins));

  soft_pwm_state = 0;
  soft_pwm_device_bitmask = 0;
  num_pwm_outputs = num_devices;  
  return APP_ERROR_TYPE_SUCCESS;
}


uint8_t Device_PwmOutput::SetPin(uint8_t device_number, uint8_t pin)
{
  if (device_number >= num_pwm_outputs)
  {
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;
  }

  if (digitalPinToTimer(pin) == NOT_ON_TIMER)
  {
    generate_response_msg_addPGM(PMSG(ERR_MSG_INVALID_PIN_NUMBER));
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE;
  }
  
  pwm_output_pins[device_number] = pin;

  pinMode(pin, OUTPUT);

  return APP_ERROR_TYPE_SUCCESS;
}


uint8_t Device_PwmOutput::EnableSoftPwm(uint8_t device_number, bool enable)
{
  if (device_number >= num_pwm_outputs || device_number >= sizeof(soft_pwm_device_bitmask)*8)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;

  if (pwm_output_pins[device_number] == 0xFF)
  {
    generate_response_msg_addPGM(PMSG(ERR_MSG_INVALID_PIN_NUMBER));
    return PARAM_APP_ERROR_TYPE_FAILED;
  }

  if (soft_pwm_state == 0 && enable)
  {
    soft_pwm_state = (SoftPwmState *)malloc(sizeof(SoftPwmState));
    if (soft_pwm_state == 0 
        || soft_pwm_state->Init(min(num_pwm_outputs,sizeof(soft_pwm_device_bitmask)*8)))
    {
      generate_response_msg_addPGM(PMSG(MSG_ERR_INSUFFICIENT_MEMORY));
      return PARAM_APP_ERROR_TYPE_FAILED;
    }
  }

  if (soft_pwm_state != 0)
  {
    uint8_t retval = soft_pwm_state->EnableSoftPwm(device_number, pwm_output_pins[device_number], true);
    if (retval != APP_ERROR_TYPE_SUCCESS)
      return retval;
  }
    
  if (enable)
    soft_pwm_device_bitmask |= (1<<device_number);
  else
    soft_pwm_device_bitmask &= ~(1<<device_number);
    
  return APP_ERROR_TYPE_SUCCESS;
}
