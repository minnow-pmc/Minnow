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
// Interface for pwm output devices
//

#ifndef DEVICE_PWMOUTPUT_H
#define DEVICE_PWMOUTPUT_H

#include "Minnow.h"
#include "SoftPwmState.h"

class Device_PwmOutput
{
public:

  static uint8_t Init(uint8_t num_pwm_devices);
  
  FORCE_INLINE static uint8_t GetNumDevices()
  {
    return num_pwm_outputs;
  }
  
  FORCE_INLINE static bool IsInUse(uint8_t device_number)
  {
    return (device_number < num_pwm_outputs 
      && pwm_output_pins[device_number] != 0xFF);
  }

  FORCE_INLINE static bool GetActiveState(uint8_t device_number)
  {
    return !pwm_output_disabled[device_number];
  }

  FORCE_INLINE static uint8_t GetPin(uint8_t device_number)
  {
    return pwm_output_pins[device_number];
  }
  
  FORCE_INLINE static bool GetSoftPwmState(uint8_t device_number)
  {
    if (device_number < sizeof(soft_pwm_device_bitmask)*8)
      return (soft_pwm_device_bitmask & (1 << device_number)) != 0;
    else
      return false;
  }
  
  // returns APP_ERROR_TYPE_SUCCESS or error code
  static uint8_t SetPin(uint8_t device_number, uint8_t pin);
  static uint8_t EnableSoftPwm(uint8_t device_number, bool enable);

  FORCE_INLINE static void WriteState(uint8_t device_number, uint8_t power)
  {
    if (soft_pwm_device_bitmask == 0 || (soft_pwm_device_bitmask & (1 << device_number)) == 0)
      analogWrite(pwm_output_pins[device_number], power);
    else
      soft_pwm_state->SetPower(device_number, power);
    pwm_output_disabled[device_number] = (power == 0);
  }

private:
  friend void updateSoftPwm();

  static uint8_t num_pwm_outputs;
  static uint8_t *pwm_output_pins;
  static bool *pwm_output_disabled;
 
  // additional state to support soft pwm 
  static uint8_t soft_pwm_device_bitmask; // soft pwm is only supported on first 8 outputs
  static SoftPwmState *soft_pwm_state; // allocated on first use
  
};

#endif


    