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

#include "SoftPwmState.h"

 
bool SoftPwmState::Init(uint8_t _num_devices)
{
  uint8_t *memory = (uint8_t*)malloc(_num_devices * 
    (sizeof(*output_reg) + sizeof(*output_bit) + sizeof(*pwm_power) + sizeof(*pwm_count)));
  if (memory == 0)
    return false;

  pwm_power = memory;
  pwm_count = pwm_power + _num_devices;
  output_bit = pwm_count + _num_devices;
  output_reg = (uint8_t **)(output_bit + _num_devices);
     
  memset(pwm_power, 0, _num_devices * sizeof(*pwm_power));
  memset(output_bit, 0, _num_devices * sizeof(*output_bit));
  memset(pwm_count, 0, _num_devices * sizeof(*pwm_count));

  num_devices = _num_devices;
  
  return true;
}
  
bool SoftPwmState::EnableSoftPwm(uint8_t device_number, uint8_t pin, bool enable)
{
  if (device_number >= num_devices)
    return false;
  if (enable)
  {
    uint8_t port = digitalPinToPort(pin);
    if (port == NOT_A_PIN)
      return false;
    output_reg[device_number] = (uint8_t *)portOutputRegister(port);
    output_bit[device_number] = digitalPinToBitMask(pin);
  }
  else
  {
    output_bit[device_number] = 0;
  }
  return true;
}
