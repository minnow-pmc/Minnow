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
// Interface for buzzer devices
//

#include "Device_Buzzer.h"
#include "response.h"

uint8_t Device_Buzzer::num_buzzers = 0;
uint8_t *Device_Buzzer::buzzer_pins;

uint8_t Device_Buzzer::soft_pwm_device_bitmask;
SoftPwmState *Device_Buzzer::soft_pwm_state;

//
// Methods
//

uint8_t Device_Buzzer::Init(uint8_t num_devices)
{
  if (num_buzzers != 0)
  {
    generate_response_msg_addPGM(PMSG(MSG_ERR_ALREADY_INITIALIZED));
    return PARAM_APP_ERROR_TYPE_FAILED;
  }
  if (num_devices == 0)
    return APP_ERROR_TYPE_SUCCESS;

  uint8_t *memory = (uint8_t*)malloc(num_devices * sizeof(*buzzer_pins));
  if (memory == 0)
  {
    generate_response_msg_addPGM(PMSG(MSG_ERR_INSUFFICIENT_MEMORY));
    return PARAM_APP_ERROR_TYPE_FAILED;
  }

  buzzer_pins = memory;
  
  memset(buzzer_pins, 0xFF, num_devices * sizeof(*buzzer_pins));

  soft_pwm_state = 0;
  soft_pwm_device_bitmask = 0;
  num_buzzers = num_devices;
  
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t Device_Buzzer::SetPin(uint8_t device_number, uint8_t pin)
{
  if (device_number >= num_buzzers)
  {
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;
  }

  if (digitalPinToTimer(pin) == NOT_ON_TIMER && digitalPinToPort(pin) == NOT_A_PIN)
  {
    generate_response_msg_addPGM(PMSG(ERR_MSG_INVALID_PIN_NUMBER));
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE;
  }
  
  buzzer_pins[device_number] = pin;
  pinMode(pin, OUTPUT);
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t Device_Buzzer::EnableSoftPwm(uint8_t device_number, bool enable)
{
  if (device_number >= num_buzzers || device_number >= sizeof(soft_pwm_device_bitmask)*8)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;

  if (buzzer_pins[device_number] == 0xFF)
  {
    generate_response_msg_addPGM(PMSG(ERR_MSG_INVALID_PIN_NUMBER));
    return PARAM_APP_ERROR_TYPE_FAILED;
  }

  if (soft_pwm_state == 0 && enable)
  {
    soft_pwm_state = (SoftPwmState *)malloc(sizeof(SoftPwmState));
    if (soft_pwm_state == 0 
        || !soft_pwm_state->Init(min(num_buzzers,sizeof(soft_pwm_device_bitmask)*8)))
    {
      generate_response_msg_addPGM(PMSG(MSG_ERR_INSUFFICIENT_MEMORY));
      return PARAM_APP_ERROR_TYPE_FAILED;
    }
  }
  
  if (soft_pwm_state != 0)
  {
    if (!soft_pwm_state->EnableSoftPwm(device_number, buzzer_pins[device_number], enable))
    {
      generate_response_msg_addPGM(PMSG(ERR_MSG_INVALID_PIN_NUMBER));
      return PARAM_APP_ERROR_TYPE_FAILED;
    }
  }
  
  if (enable)
    soft_pwm_device_bitmask |= (1<<device_number);
  else
    soft_pwm_device_bitmask &= ~(1<<device_number);
    
  return APP_ERROR_TYPE_SUCCESS;
}
