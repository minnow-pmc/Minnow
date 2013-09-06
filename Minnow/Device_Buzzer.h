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

#ifndef DEVICE_BUZZER_H
#define DEVICE_BUZZER_H

#include "Minnow.h"
#include "SoftPwmState.h"

class Device_Buzzer
{
public:

  static uint8_t Init(uint8_t num_buzzers);
  
  FORCE_INLINE static uint8_t GetNumDevices()
  {
    return num_buzzers;
  }
  
  FORCE_INLINE static bool IsInUse(uint8_t device_number)
  {
    return (device_number < num_buzzers 
      && buzzer_pins[device_number] != 0xFF);
  }

  FORCE_INLINE static uint8_t GetPin(uint8_t device_number)
  {
    return buzzer_pins[device_number];
  }
  
  // returns APP_ERROR_TYPE_SUCCESS or error code
  static uint8_t SetPin(uint8_t device_number, uint8_t pin);
  static uint8_t EnableSoftPwm(uint8_t device_number, bool enable);

  FORCE_INLINE static void WriteState(uint8_t device_number, uint8_t power)
  {
    if (soft_pwm_device_bitmask == 0 || (soft_pwm_device_bitmask & (1<<device_number)) == 0)
      analogWrite(buzzer_pins[device_number], power);
    else
      soft_pwm_state->SetPower(device_number, power);
  }

private:
  friend void updateSoftPwm();

  static uint8_t num_buzzers;
  static uint8_t *buzzer_pins;
  
  // additional state to support soft pwm 
  static uint8_t soft_pwm_device_bitmask; // soft pwm is only supported on first 8 device numbers
  static SoftPwmState *soft_pwm_state; // allocated on first use
};

#endif


    