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
 
#include "Device_Heater.h"

uint8_t Device_Heater::heater_pins[MAX_HEATERS];
uint8_t Device_Heater::heater_thermistor_pins[MAX_HEATERS];
uint8_t Device_Heater::heater_thermistor_types[MAX_HEATERS];
uint8_t Device_Heater::heater_control_modes[MAX_HEATERS];
int16_t Device_Heater::heater_max_temps[MAX_HEATERS];

int16_t Device_Heater::heater_target_temps[MAX_HEATERS];
int16_t Device_Heater::heater_current_temps[MAX_HEATERS];

//
// Methods
//

void Device_Heater::Init()
{
  memset(heater_pins, 0xFF, sizeof(heater_pins));
}

uint8_t Device_Heater::GetNumDevices()
{
  for (int8_t i=MAX_HEATERS-1; i>=0; i--)
  {
    if (heater_pins[i] != 0xFF)
      return i+1;
  }    
  return 0;
}


bool Device_Heater::SetHeaterPin(uint8_t device_number, uint8_t pin)
{
  if (device_number >= MAX_HEATERS)
    return false;
  
  // TODO check pin
  heater_pins[device_number] = pin;
  heater_thermistor_pins[device_number] = 0xFF;
  heater_thermistor_types[device_number] = 0xFF;
  heater_max_temps[device_number] = PM_TEMPERATURE_INVALID;
  heater_control_modes[device_number] = HEATER_CONTROL_MODE_INVALID;
  
  heater_target_temps[device_number] = PM_TEMPERATURE_INVALID;
  heater_current_temps[device_number] = PM_TEMPERATURE_INVALID;

  return true;
}

bool Device_Heater::SetThermistorPin(uint8_t device_number, uint8_t pin)
{
  if (device_number >= MAX_HEATERS)
    return false;
  
  // TODO check pin
  heater_thermistor_pins[device_number] = pin;

  return true;
}

bool Device_Heater::SetThermistorType(uint8_t device_number, uint8_t type)
{
  if (device_number >= MAX_HEATERS)
    return false;
  
  // TODO check type
  heater_thermistor_types[device_number] = type;

  return true;
}

bool Device_Heater::SetControlMode(uint8_t device_number, uint8_t mode)
{
  if (device_number >= MAX_HEATERS)
    return false;
  
  // TODO check more
  heater_control_modes[device_number] = mode;

  return true;
}

bool Device_Heater::SetMaxTemperature(uint8_t device_number, int16_t temp)
{
  if (device_number >= MAX_HEATERS)
    return false;
  
  // TODO check more
  heater_max_temps[device_number] = temp;

  return true;
}

bool Device_Heater::ValidateTargetTemperature(uint8_t device_number, int16_t temp)
{
  if (device_number >= MAX_HEATERS
      || heater_pins[device_number] == 0xFF
      || heater_thermistor_pins[device_number] == 0xFF 
      || heater_thermistor_types[device_number] == 0xFF
      || heater_max_temps[device_number] == PM_TEMPERATURE_INVALID
      || (heater_control_modes[device_number] != HEATER_CONTROL_MODE_PID
            && heater_control_modes[device_number] != HEATER_CONTROL_MODE_BANG_BANG))
    return false;
    
  if (temp > heater_max_temps[device_number] || temp < 0)
    return false;
    
  return true;
}
