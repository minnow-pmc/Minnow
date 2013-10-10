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
// Interface for temperature_sensor devices
//

#ifndef DEVICE_TEMPERATURE_SENSOR_H
#define DEVICE_TEMPERATURE_SENSOR_H

#include "Minnow.h"

class Device_TemperatureSensor
{
public:

#define SENSOR_TEMPERATURE_INVALID 0.0

#define TEMP_SENSOR_TYPE_INVALID     0

// see thermistortables.h for available thermistor types.

#define LAST_THERMISTOR_SENSOR_TYPE 99 // all temperature sensor types above this are not thermistors


  static uint8_t Init(uint8_t num_temperature_sensors);
  
  static uint8_t GetNumDevices()
  {
    return num_temperature_sensors;
  }
  
  FORCE_INLINE static bool IsInUse(uint8_t device_number)
  {
    return (device_number < num_temperature_sensors 
      && temperature_sensor_pins[device_number] != 0xFF
      && temperature_sensor_types[device_number] != TEMP_SENSOR_TYPE_INVALID);
  }

  FORCE_INLINE static uint8_t GetPin(uint8_t device_number)
  {
    return temperature_sensor_pins[device_number];
  }
  
  FORCE_INLINE static uint8_t GetType(uint8_t device_number)
  {
    return temperature_sensor_types[device_number];
  }
  
  FORCE_INLINE static float ReadCurrentTemperature(uint8_t device_number)
  {
    return temperature_sensor_current_temps[device_number];
  }
  
  // these configuration functions return APP_ERROR_TYPE_SUCCESS or error code
  static uint8_t SetPin(uint8_t device_number, uint8_t pin);
  static uint8_t SetType(uint8_t device_number, uint8_t type);

  static void UpdateTemperatureSensors();
  
private:

  friend void updateTemperatureSensorRawValues();
  
  static uint8_t num_temperature_sensors;
  
  static uint8_t *temperature_sensor_pins;
  static uint8_t *temperature_sensor_types;

  static uint16_t *temperature_sensor_raw_values;  
  static uint16_t *temperature_sensor_isr_raw_values;  

  static float *temperature_sensor_current_temps;  
};


#endif


    