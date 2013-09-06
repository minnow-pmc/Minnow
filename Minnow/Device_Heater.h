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
#include "SoftPwmState.h"
#include "Device_TemperatureSensor.h"

class Device_Heater
{
public:

#define HEATER_CONTROL_MODE_INVALID       0
#define HEATER_CONTROL_MODE_PID           1
#define HEATER_CONTROL_MODE_BANG_BANG     2

  static uint8_t Init(uint8_t num_pwm_devices);
  
  FORCE_INLINE static uint8_t GetNumDevices()
  {
    return num_heaters;
  }
  
  FORCE_INLINE static bool IsInUse(uint8_t device_number)
  {
    return (device_number < num_heaters 
      && heater_pins[device_number] != 0xFF);
  }

  FORCE_INLINE static uint8_t GetHeaterPin(uint8_t device_number)
  {
    return heater_pins[device_number];
  }
  
  FORCE_INLINE static uint8_t GetTempSensor(uint8_t device_number)
  {
    return heater_temp_sensors[device_number];
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
    return Device_TemperatureSensor::ReadCurrentTemperature(heater_temp_sensors[device_number]);
  }
  
  // these configuration functions return APP_ERROR_TYPE_SUCCESS or error code
  static uint8_t SetHeaterPin(uint8_t device_number, uint8_t pin);
  static uint8_t SetTempSensor(uint8_t heater_device_number, uint8_t sensor_device_number);
  static uint8_t SetControlMode(uint8_t device_number, uint8_t mode);
  static uint8_t SetMaxTemperature(uint8_t device_number, int16_t temp);
  static uint8_t EnableSoftPwm(uint8_t device_number, bool enable);

  static uint8_t ValidateTargetTemperature(uint8_t device_number, int16_t temp);

  FORCE_INLINE static void SetTargetTemperature(uint8_t device_number, int16_t temp)
  {
    heater_target_temps[device_number] = temp;
  }
  
  static void UpdateHeaters();
private:

  friend void updateSoftPwm();

  static uint8_t num_heaters;
  static uint8_t *heater_pins;
  static uint8_t *heater_temp_sensors;
  static uint8_t *heater_control_modes;
  static int16_t *heater_max_temps;

  static int16_t *heater_target_temps;
  
  // additional state to support soft pwm 
  static uint8_t soft_pwm_device_bitmask; // soft pwm is only supported on first 8 heaters
  static SoftPwmState *soft_pwm_state; // allocated on first use    
};


#endif


    