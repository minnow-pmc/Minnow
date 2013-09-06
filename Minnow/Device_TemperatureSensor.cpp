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

// The Device_TemperatureSensor static are placed in the movement.cpp compilation unit to 
// allow potentiall better optimization in the ISR

extern volatile bool temp_meas_ready;

//
// Methods
//

FORCE_INLINE static int16_t convertRawTempValue(uint8_t type, uint16_t raw_value);

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

  for (int8_t i=0; i<num_temperature_sensors; i++)
  {
    temperature_sensor_current_temps[i] = 0x7FFF;
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
  
  // we don't plan to read it as digital input but 
  // this is better than no check.
  if (digitalPinToPort(pin) == NOT_A_PIN)
  {
    generate_response_msg_addPGM(PMSG(ERR_MSG_INVALID_PIN_NUMBER));
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE;
  }
  
  temperature_sensor_pins[device_number] = pin;

  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t Device_TemperatureSensor::SetType(uint8_t device_number, uint8_t type)
{
  if (device_number >= num_temperature_sensors)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;
  
  if (type > LAST_ADC_TEMP_SENSOR_TYPE || type == TEMP_SENSOR_TYPE_INVALID)
  {
    generate_response_msg_addPGM(PMSG(MSG_ERR_UNKNOWN_VALUE));
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE;
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
    temperature_sensor_current_temps[i] = convertRawTempValue(temperature_sensor_types[i], temperature_sensor_raw_values[i]);
  }

  CRITICAL_SECTION_START;
  temp_meas_ready = false;
  CRITICAL_SECTION_END;
}

//
// Utility function
//

int16_t convertRawTempValue(uint8_t type, uint16_t raw_value)
{
  return 0; // TODO
}
