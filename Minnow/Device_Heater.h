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

//
// Interfaces for Heaters devices.
//
// Notes: It is expected that features such as redundant temperature sensors
// and temperature watch periods to detect sensor or heater failures can be
// implemented in the host as they do not require tight real time co-ordination. 
//
class Device_Heater
{
public:

// By default Soft PWM is enabled for heaters. A large current switched 
// at high frequency can overheat components (if not designed for it) and 
// will generate more electrical noise.
#define DEFAULT_HEATER_USE_SOFT_PWM              1 

#define DEFAULT_HEATER_POWER_ON_LEVEL            255 // == full current

// Supported Control Modes
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
  
  FORCE_INLINE static float GetMaxTemperature(uint8_t device_number)
  {
    return heater_info_array[device_number].max_temp;
  }
  
  FORCE_INLINE static bool GetSoftPwmState(uint8_t device_number)
  {
    if (device_number < sizeof(soft_pwm_device_bitmask)*8)
      return (soft_pwm_device_bitmask & (1 << device_number)) != 0;
    else
      return false;
  }

  FORCE_INLINE static float GetTargetTemperature(uint8_t device_number)
  {
    return heater_info_array[device_number].target_temp;
  }
  
  FORCE_INLINE static float ReadCurrentTemperature(uint8_t device_number)
  {
    return Device_TemperatureSensor::ReadCurrentTemperature(heater_info_array[device_number].temp_sensor);
  }
  
  // these configuration functions return APP_ERROR_TYPE_SUCCESS or error code
  static uint8_t SetHeaterPin(uint8_t device_number, uint8_t heater_pin);
  static uint8_t SetTempSensor(uint8_t heater_device_number, uint8_t sensor_device_number);
  static uint8_t SetControlMode(uint8_t device_number, uint8_t mode);
  static uint8_t SetMaxTemperature(uint8_t device_number, float temp);

  // This sets the pwm power level used when the heater is on.
  // Setting this to anything other than 255 enables a form of PWM (soft or hard)
  // So you shouldn't use it unless you are OK with PWM on your heater.
  static uint8_t SetPowerOnLevel(uint8_t device_number, uint8_t level);
  static uint8_t EnableSoftPwm(uint8_t device_number, bool enable);

  // Bang Bang Mode Specific Config Parameters
  FORCE_INLINE static uint8_t GetBangBangHysteresis(uint8_t device_number)
  {
    return heater_info_array[device_number].control_info.bangbang.hysteresis;
  }
  static uint8_t SetBangBangHysteresis(uint8_t device_number, uint8_t temp_range); // in degrees C

  //
  // PID Mode Specific Config Parameters
  //
  FORCE_INLINE static uint8_t GetPidFunctionalRange(uint8_t device_number)
  {
    return heater_info_array[device_number].control_info.pid->functional_range;
  }
  FORCE_INLINE static float GetPidDefaultKp(uint8_t device_number)
  {
    return heater_info_array[device_number].control_info.pid->Kp;
  }
  FORCE_INLINE static float GetPidDefaultKi(uint8_t device_number)
  {
    if (PID_dT != 0.0)
      return heater_info_array[device_number].control_info.pid->Ki / PID_dT;
    else
      return 0.0;
  }
  FORCE_INLINE static float GetPidDefaultKd(uint8_t device_number)
  {
    return heater_info_array[device_number].control_info.pid->Kd * PID_dT;
  }
  
  static uint8_t SetPidFunctionalRange(uint8_t device_number, uint8_t value); // in degrees C
  static uint8_t SetPidDefaultKp(uint8_t device_number, float value);
  static uint8_t SetPidDefaultKi(uint8_t device_number, float value);
  static uint8_t SetPidDefaultKd(uint8_t device_number, float value);
  // TODO add advanced PID parameters  
  
  static void DoPidAutotune(uint8_t device_number, float temp, int ncycles);
  
  static uint8_t ValidateTargetTemperature(uint8_t device_number, float temp);

  FORCE_INLINE static void SetTargetTemperature(uint8_t device_number, float temp)
  {
    // this assumes that ValidateTargetTemperature has already been called
    HeaterInfo *heater_info = &heater_info_array[device_number];
    if (temp == SENSOR_TEMPERATURE_INVALID)
      SetHeaterPower(heater_info, 0);
    if (heater_info->control_mode == HEATER_CONTROL_MODE_PID
          && heater_info->target_temp == SENSOR_TEMPERATURE_INVALID
          && temp != SENSOR_TEMPERATURE_INVALID)
      InitializePidState(heater_info);
    heater_info->target_temp = temp;
  }
  
  static void UpdateHeaters();
private:

  friend void updateSoftPwm();

  struct BangBangInfo
  {
    #define DEFAULT_BANG_BANG_HYSTERESIS        0

    // Hystersis to use when switching heater on an off (in degrees)
    uint8_t hysteresis;
  };
  
  struct PidInfo
  {
    //
    // Basic PID Configuration Parameters
    //
    
    #define DEFAULT_PID_FUNCTIONAL_RANGE  10
    
    #define DEFAULT_PID_K1                0.95
    #define DEFAULT_MAX_PID_POWER_LEVEL   255 // == full current
    #define DEFAULT_INTEGRAL_DRIVE_MAX    255
  
    // If the temperature difference between the target temperature and the actual temperature
    // is more than this value then the PID will be shut off and the heater will be set to 0 or 
    // power_on_level.
    uint8_t functional_range;  // (default = 10degsC)

    // Primary PID configuration values.
    float Kp;
    float Ki;
    float Kd;
    
    //
    // Advanced Configuration Parameters
    //
    float advanced_K1; //smoothing factor within the PID (default = 0.95) 
    uint8_t advanced_max_pid_power_level; // maximum drive level when temperature within functional_range 
                                          // (default = 255 i.e., full current)
    uint8_t advanced_integral_drive_max; // limit for the integral term (default = 255)
    
    // 
    // Derived Configuration
    //
    
    float KdK2; // == Kd * (1 - K1)
    float iState_max; // == advanced_integral_drive_max / Ki
    
    // 
    // State 
    //
    
    float state_iState;
    float state_dTerm;
    float last_temp;
  };
  
  struct HeaterInfo
  {
    uint8_t device_number;
    uint8_t heater_pin;
    uint8_t temp_sensor;
    float max_temp;
    uint8_t power_on_level;
    uint8_t control_mode;
    float target_temp;
    bool is_heating;
    union 
    {
      BangBangInfo bangbang;
      PidInfo* pid;
    } control_info;
  };
  
  FORCE_INLINE static void SetHeaterPower(HeaterInfo *heater_info, uint8_t power)
  {
    uint8_t device_number = heater_info->device_number;
#if HEATER_DEBUG
    DEBUG("SetHeater("); DEBUG((int)device_number); DEBUG("): "); DEBUG((int)power); DEBUG("/"); DEBUGLN((int)Device_TemperatureSensor::ReadCurrentTemperature(heater_info->temp_sensor));
#endif    
    if ((soft_pwm_device_bitmask & (1<<device_number)) == 0)
      analogWrite(heater_info->heater_pin, power);
    else
      soft_pwm_state->SetPower(device_number, power);
    heater_info->is_heating = (power > 0);
  }
  
  FORCE_INLINE static void InitializePidState(HeaterInfo *heater_info)
  {
    // initialize PID state ready for use
    PidInfo *pid_info = heater_info->control_info.pid;
    pid_info->state_iState = 0.0;
    pid_info->state_dTerm = 0.0;
    pid_info->last_temp = Device_TemperatureSensor::ReadCurrentTemperature(heater_info->temp_sensor);
  }
  
  static void UpdatePidDerivedConfig(HeaterInfo *heater_info);
  
  static uint8_t num_heaters;
  static HeaterInfo *heater_info_array;
  
  // additional state to support soft pwm 
  static uint8_t soft_pwm_device_bitmask; // soft pwm is only supported on first 8 heaters
  static SoftPwmState *soft_pwm_state; // allocated on first use
  
  static float PID_dT; // based on sampling rate of temperature measurements
};


#endif


    