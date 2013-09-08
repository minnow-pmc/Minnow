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
 
#include "Device_TemperatureSensor.h"
#include "response.h"

#include "thermistortables.h"

// The Device_TemperatureSensor static are placed in the movement.cpp compilation unit to 
// allow potentiall better optimization in the ISR

extern volatile bool temp_meas_ready;

//
// Methods
//

FORCE_INLINE static int16_t convert_raw_temp_value(uint8_t type, uint16_t raw_value)
{
  if (type <= LAST_THERMISTOR_SENSOR_TYPE)
  {
    const int16_t (*tt)[2];
    uint8_t tt_len;
    
    switch (type)
    {
    case 1:  tt = TT_NAME(1); tt_len = NUM_ARRAY_ELEMENTS(TT_NAME(1)); break;
    case 2:  tt = TT_NAME(2); tt_len = NUM_ARRAY_ELEMENTS(TT_NAME(2)); break;
    case 3:  tt = TT_NAME(3); tt_len = NUM_ARRAY_ELEMENTS(TT_NAME(3)); break;
    case 4:  tt = TT_NAME(4); tt_len = NUM_ARRAY_ELEMENTS(TT_NAME(4)); break;
    case 5:  tt = TT_NAME(5); tt_len = NUM_ARRAY_ELEMENTS(TT_NAME(5)); break;
    case 6:  tt = TT_NAME(6); tt_len = NUM_ARRAY_ELEMENTS(TT_NAME(6)); break;
    case 7:  tt = TT_NAME(7); tt_len = NUM_ARRAY_ELEMENTS(TT_NAME(7)); break;
    case 8:  tt = TT_NAME(8); tt_len = NUM_ARRAY_ELEMENTS(TT_NAME(8)); break;
    case 9:  tt = TT_NAME(9); tt_len = NUM_ARRAY_ELEMENTS(TT_NAME(9)); break;
    case 10:  tt = TT_NAME(10); tt_len = NUM_ARRAY_ELEMENTS(TT_NAME(10)); break;
    case 51:  tt = TT_NAME(51); tt_len = NUM_ARRAY_ELEMENTS(TT_NAME(51)); break;
    case 52:  tt = TT_NAME(52); tt_len = NUM_ARRAY_ELEMENTS(TT_NAME(52)); break;
    case 55:  tt = TT_NAME(55); tt_len = NUM_ARRAY_ELEMENTS(TT_NAME(55)); break;
    case 60:  tt = TT_NAME(60); tt_len = NUM_ARRAY_ELEMENTS(TT_NAME(60)); break;
    case 71:  tt = TT_NAME(71); tt_len = NUM_ARRAY_ELEMENTS(TT_NAME(71)); break;
    default:
      return PM_TEMPERATURE_INVALID;
    }
    
    // remember raw values are inverse of temperatures for thermistors.
    if (raw_value < THERMMISTOR_RAW_HI_TEMP || raw_value > THERMMISTOR_RAW_LO_TEMP) 
      return PM_TEMPERATURE_INVALID;
    
    #define PGM_RD_W(x)   (int16_t)pgm_read_word(&x)
    // Derived from RepRap FiveD extruder::getTemperature()
    // For hot end temperature measurement.

    float celsius;
    uint8_t i;

    for (i=1; i<tt_len; i++)
    {
      if (PGM_RD_W(tt[i][0]) > (int16_t)raw_value)
      {
        celsius = PGM_RD_W(tt[i-1][1]) + 
          (raw_value - PGM_RD_W(tt[i-1][0])) * 
          (float)(PGM_RD_W(tt[i][1]) - PGM_RD_W(tt[i-1][1])) /
          (float)(PGM_RD_W(tt[i][0]) - PGM_RD_W(tt[i-1][0]));
        return (int16_t)celsius * 10;
      }
    }

    // Overflow: Set to last value in the table
    celsius = PGM_RD_W(tt[i-1][1]);
    return (int16_t)celsius * 10;
  }
  else
  {
    // TODO implement thermocouple types
    return PM_TEMPERATURE_INVALID;
  }
}


uint8_t Device_TemperatureSensor::Init(uint8_t num_devices)
{
  if (num_temperature_sensors != 0)
  {
    generate_response_msg_addPGM(PMSG(MSG_ERR_ALREADY_INITIALIZED));
    return PARAM_APP_ERROR_TYPE_FAILED;
  }
  if (num_devices == 0)
    return APP_ERROR_TYPE_SUCCESS;

  uint8_t *memory = (uint8_t *)malloc(num_devices *
      (sizeof(*temperature_sensor_pins) + sizeof(*temperature_sensor_types) 
              + sizeof(*temperature_sensor_current_temps)
              + sizeof(*temperature_sensor_isr_raw_values) + sizeof(*temperature_sensor_raw_values)));

  if (memory == 0)
  {
    generate_response_msg_addPGM(PMSG(MSG_ERR_INSUFFICIENT_MEMORY));
    return PARAM_APP_ERROR_TYPE_FAILED;
  }

  temperature_sensor_pins = memory;
  temperature_sensor_types = temperature_sensor_pins + num_devices;
  temperature_sensor_isr_raw_values = (uint16_t *)(temperature_sensor_types + num_devices);
  temperature_sensor_raw_values = temperature_sensor_isr_raw_values + num_devices;
  temperature_sensor_current_temps = (int16_t *)(temperature_sensor_raw_values + num_devices);
  
  memset(temperature_sensor_pins, 0xFF, num_devices * sizeof(*temperature_sensor_pins));
  memset(temperature_sensor_types, TEMP_SENSOR_TYPE_INVALID, num_devices * sizeof(*temperature_sensor_types));
  memset(temperature_sensor_raw_values, 0, num_devices * sizeof(*temperature_sensor_raw_values));
  memset(temperature_sensor_isr_raw_values, 0, num_devices * sizeof(*temperature_sensor_isr_raw_values));

  for (int8_t i=0; i<num_devices; i++)
  {
    temperature_sensor_current_temps[i] = PM_TEMPERATURE_INVALID;
  }

  // Set analog inputs
  ADCSRA = 1<<ADEN | 1<<ADSC | 1<<ADIF | 0x07;
  DIDR0 = 0;
  #ifdef DIDR2
    DIDR2 = 0;
  #endif
  
  num_temperature_sensors = num_devices;
  
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t Device_TemperatureSensor::SetPin(uint8_t device_number, uint8_t pin)
{
  if (device_number >= num_temperature_sensors)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;
  
  temperature_sensor_pins[device_number] = pin;

  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t Device_TemperatureSensor::SetType(uint8_t device_number, uint8_t type)
{
  if (device_number >= num_temperature_sensors)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;
  
  if (type > LAST_THERMISTOR_SENSOR_TYPE || type == TEMP_SENSOR_TYPE_INVALID)
  {
    generate_response_msg_addPGM(PMSG(MSG_ERR_UNKNOWN_VALUE));
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE;
  }
  
  if (type <= LAST_THERMISTOR_SENSOR_TYPE)
  {
    // for now, do conversion as simple way of checking type validity
    if (convert_raw_temp_value(type, THERMMISTOR_RAW_HI_TEMP) == PM_TEMPERATURE_INVALID)
    {
      generate_response_msg_addPGM(PMSG(MSG_ERR_UNKNOWN_VALUE));
      return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE;
    }
  }
  
  if (temperature_sensor_pins[device_number] == 0xFF)
  {
    generate_response_msg_addPGM(PMSG(ERR_MSG_INVALID_PIN_NUMBER));
    return PARAM_APP_ERROR_TYPE_FAILED;
  }

  const uint8_t pin = temperature_sensor_pins[device_number];
  if (pin < 8)
  {
    DIDR0 |= 1 << pin; 
  }
  else
  {
#ifdef DIDR2
    DIDR2 |= 1<<(pin - 8); 
#else    
    generate_response_msg_addPGM(PMSG(ERR_MSG_INVALID_PIN_NUMBER));
    return PARAM_APP_ERROR_TYPE_FAILED;
#endif  
  }
  
  temperature_sensor_types[device_number] = type;

  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t Device_TemperatureSensor::ValidateConfig(uint8_t device_number)
{
  if (device_number >= num_temperature_sensors
      || temperature_sensor_pins[device_number] == 0xFF
      || temperature_sensor_types[device_number] == TEMP_SENSOR_TYPE_INVALID)
    return false;

  return true;
}

void Device_TemperatureSensor::UpdateTemperatureSensors()
{
  if (!temp_meas_ready)
    return;

  for(uint8_t i=0; i<num_temperature_sensors;i++)
  {
    temperature_sensor_current_temps[i] = convert_raw_temp_value(temperature_sensor_types[i], temperature_sensor_raw_values[i]);
  }

  CRITICAL_SECTION_START;
  temp_meas_ready = false;
  CRITICAL_SECTION_END;
}

