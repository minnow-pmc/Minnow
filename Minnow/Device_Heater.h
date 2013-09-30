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
      && heater_info_array[device_number].temp_sensor != 0xFF
      && heater_info_array[device_number].heater_pin != 0xFF);
  }

  FORCE_INLINE static uint8_t GetHeaterPin(uint8_t device_number)
  {
    return heater_info_array[device_number].heater_pin;
  }
  
  FORCE_INLINE static uint8_t GetTempSensor(uint8_t device_number)
  {
    return heater_info_array[device_number].temp_sensor;
  }
  
  FORCE_INLINE static uint8_t GetControlMode(uint8_t device_number)
  {
    return heater_info_array[device_number].control_mode;
  }
  
  FORCE_INLINE static int16_t GetMaxTemperature(uint8_t device_number)
  {
    return heater_info_array[device_number].max_temp;
  }
  
  FORCE_INLINE static int16_t GetTargetTemperature(uint8_t device_number)
  {
    return heater_info_array[device_number].target_temp;
  }
  
  FORCE_INLINE static int16_t ReadCurrentTemperature(uint8_t device_number)
  {
    return Device_TemperatureSensor::ReadCurrentTemperature(heater_info_array[device_number].temp_sensor);
  }
  
  // these configuration functions return APP_ERROR_TYPE_SUCCESS or error code
  static uint8_t SetHeaterPin(uint8_t device_number, uint8_t heater_pin);
  static uint8_t SetTempSensor(uint8_t heater_device_number, uint8_t sensor_device_number);
  static uint8_t SetControlMode(uint8_t device_number, uint8_t mode);
  static uint8_t SetMaxTemperature(uint8_t device_number, int16_t temp);

  // This sets the pwm power level used when the heater is on.
  // Setting this to anything other than 255 enables a form of PWM (soft or hard)
  // So you shouldn't use it unless you are OK with PWM on your heater.
  static uint8_t SetPowerOnLevel(uint8_t device_number, uint8_t level);
  static uint8_t EnableSoftPwm(uint8_t device_number, bool enable);
  
  // At this level temp_range is in the standard 1/10th degC units 
  // Note however that devices.heater.x.bang_bang_hysteresis is specified in degrees C units.
  static uint8_t SetBangBangHysteresis(uint8_t device_number, uint8_t temp_range);

  static uint8_t ValidateTargetTemperature(uint8_t device_number, int16_t temp);

  FORCE_INLINE static void SetTargetTemperature(uint8_t device_number, int16_t temp)
  {
    HeaterInfo *heater_info = &heater_info_array[device_number];
    if (temp == PM_TEMPERATURE_INVALID || temp == 0)
      SetHeaterPower(heater_info, 0);
    heater_info->target_temp = temp;
  }
  
  static void UpdateHeaters();
private:

  friend void updateSoftPwm();

  struct BangBangInfo
  {
    // Hystersis to use when switching heater on an off (in tenths of degrees)
    uint8_t hysteresis;
  };
  
  struct PidInfo
  {
    // If the temperature difference between the target temperature and the actual temperature
    // is more than this value then the PID will be shut off and the heater will be set to min/max.
    uint8_t functional_range;

    float Kp;
    float Ki;
    float Kd;
    
    //
    // Advanced parameters
    //
    float advanced_pid_K1; //smoothing factor within the PID (dflt = 0.95) DEFAULT_K1
    uint8_t advanced_integral_drive_max; //limit for the integral term
    
    // Puts PID in open loop.      
    bool advanced_enable_open_loop_mode;
  };
  
  struct HeaterInfo
  {
    uint8_t device_number;
    uint8_t heater_pin;
    uint8_t temp_sensor;
    int16_t max_temp;
    uint8_t power_on_level;
    uint8_t control_mode;
    int16_t target_temp;
    bool is_heating;
    union 
    {
      BangBangInfo bangbang;
      PidInfo pid;
    } control_info;
  };
  
  FORCE_INLINE static void SetHeaterPower(HeaterInfo *heater_info, uint8_t power)
  {
    uint8_t device_number = heater_info->device_number;
    if (soft_pwm_device_bitmask == 0 || (soft_pwm_device_bitmask & (1<<device_number)) == 0)
      analogWrite(heater_info->heater_pin, power);
    else
      soft_pwm_state->SetPower(device_number, power);
    heater_info->is_heating = (power > 0);
  }
  
  static uint8_t num_heaters;
  static HeaterInfo *heater_info_array;
  
  // additional state to support soft pwm 
  static uint8_t soft_pwm_device_bitmask; // soft pwm is only supported on first 8 heaters
  static SoftPwmState *soft_pwm_state; // allocated on first use    
};


#endif


    