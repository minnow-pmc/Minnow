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

//
// Temperature Sensor Types
//

// Thermistor sensor types: (>0)
//
// 1 is 100k thermistor - best choice for EPCOS 100k (4.7k pullup)
// 2 is 200k thermistor - ATC Semitec 204GT-2 (4.7k pullup)
// 3 is mendel-parts thermistor (4.7k pullup)
// 4 is 10k thermistor !! do not use it for a hotend. It gives bad resolution at high temp. !!
// 5 is 100K thermistor - ATC Semitec 104GT-2 (Used in ParCan) (4.7k pullup)
// 6 is 100k EPCOS - Not as accurate as table 1 (created using a fluke thermocouple) (4.7k pullup)
// 7 is 100k Honeywell thermistor 135-104LAG-J01 (4.7k pullup)
// 71 is 100k Honeywell thermistor 135-104LAF-J01 (4.7k pullup)
// 8 is 100k 0603 SMD Vishay NTCS0603E3104FXT (4.7k pullup)
// 9 is 100k GE Sensing AL03006-58.2K-97-G1 (4.7k pullup)
// 10 is 100k RS thermistor 198-961 (4.7k pullup)
// 60 is 100k Maker's Tool Works Kapton Bed Thermister
//
//    1k ohm pullup tables - This is not normal, you would have to have changed out your 4.7k for 1k
//                          (but gives greater accuracy and more stable PID on hotend)
// 51 is 100k thermistor - EPCOS (1k pullup)
// 52 is 200k thermistor - ATC Semitec 204GT-2 (1k pullup)
// 55 is 100k thermistor - ATC Semitec 104GT-2 (Used in ParCan) (1k pullup)

// Thermocouple sensor types: (<0)
//
// -1 is thermocouple with AD595


#define FIRST_THERMISTOR_SENSOR_TYPE 1 // all temperature sensor types below this are not thermistors
#define LAST_THERMISTOR_SENSOR_TYPE 99 // all temperature sensor types above this are not thermistors
// see thermistortables.h for available thermistor types.

#define FIRST_THERMOCOUPLE_SENSOR_TYPE -30 // all temperature sensor types below this are not thermocouples
#define LAST_THERMOCOUPLE_SENSOR_TYPE -1 // all temperature sensor types above this are not thermocouples

//These defines help to calibrate the AD595 sensor in case you get wrong temperature measurements.
//The measured temperature is defined as "actualTemp = (measuredTemp * TEMP_SENSOR_AD595_GAIN) + TEMP_SENSOR_AD595_OFFSET"
#define TEMP_SENSOR_AD595_OFFSET 0.0
#define TEMP_SENSOR_AD595_GAIN   1.0

// TODO make these configuration

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
    if (temperature_sensor_current_temps[device_number] == SENSOR_TEMPERATURE_INVALID)
    {
      // TODO - improve this.
      DEBUGPGM("Invalid Temp: ");
      DEBUG((int)device_number);
      DEBUGPGM(" ");
      DEBUGLN(temperature_sensor_raw_values[device_number]);
    }
    return temperature_sensor_current_temps[device_number];
  }
  
  // these configuration functions return APP_ERROR_TYPE_SUCCESS or error code
  static uint8_t SetPin(uint8_t device_number, uint8_t pin);
  static uint8_t SetType(uint8_t device_number, int16_t type);

  static void UpdateTemperatureSensors();
  
private:

  friend void updateTemperatureSensorRawValues();
  
  static uint8_t num_temperature_sensors;
  
  static uint8_t *temperature_sensor_pins;
  static int8_t *temperature_sensor_types;

  static uint16_t *temperature_sensor_raw_values;  
  static uint16_t *temperature_sensor_isr_raw_values;  

  static float *temperature_sensor_current_temps;  
};


#endif


    