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
// Interface for heater devices
//

#ifndef DEVICE_HEATER_H
#define DEVICE_HEATER_H

#include "Minnow.h"

class Device_Heater
{
public:

#define HEATER_CONTROL_MODE_INVALID       0
#define HEATER_CONTROL_MODE_PID           1
#define HEATER_CONTROL_MODE_BANG_BANG     2

  static void Init();
  static uint8_t GetNumDevices();
  
  FORCE_INLINE static bool IsInUse(uint8_t device_number)
  {
    return (device_number < MAX_HEATERS 
      && heater_pins[device_number] != 0xFF);
  }

  FORCE_INLINE static uint8_t GetHeaterPin(uint8_t device_number)
  {
    return heater_pins[device_number];
  }
  
  FORCE_INLINE static uint8_t GetThermisterPin(uint8_t device_number)
  {
    return heater_thermistor_pins[device_number];
  }
  
  FORCE_INLINE static uint8_t GetThermisterType(uint8_t device_number)
  {
    return heater_thermistor_types[device_number];
  }
  
  FORCE_INLINE static uint8_t GetControlMode(uint8_t device_number)
  {
    return heater_control_modes[device_number];
  }
  
  FORCE_INLINE static int16_t GetMaxTemperature(uint8_t device_number)
  {
    return heater_max_temps[device_number];
  }
  
  FORCE_INLINE static int16_t GetTargetTemperature(uint8_t device_number)
  {
    return heater_target_temps[device_number];
  }
  
  FORCE_INLINE static int16_t ReadCurrentTemperature(uint8_t device_number)
  {
    return heater_current_temps[device_number];
  }
  
  static bool SetHeaterPin(uint8_t device_number, uint8_t pin);
  static bool SetThermistorPin(uint8_t device_number, uint8_t pin);
  static bool SetThermistorType(uint8_t device_number, uint8_t type);
  static bool SetControlMode(uint8_t device_number, uint8_t mode);
  static bool SetMaxTemperature(uint8_t device_number, int16_t temp);
  
  static bool ValidateTargetTemperature(uint8_t device_number, int16_t temp);

  FORCE_INLINE static void SetTargetTemperature(uint8_t device_number, int16_t temp)
  {
    heater_target_temps[device_number] = temp;
  }
  
private:

  friend void movement_ISR();

  static uint8_t heater_pins[MAX_HEATERS];
  static uint8_t heater_thermistor_pins[MAX_HEATERS];
  static uint8_t heater_thermistor_types[MAX_HEATERS];
  static uint8_t heater_control_modes[MAX_HEATERS];
  static int16_t heater_max_temps[MAX_HEATERS];

  static int16_t heater_target_temps[MAX_HEATERS];
  static int16_t heater_current_temps[MAX_HEATERS];
  
};


#endif


    