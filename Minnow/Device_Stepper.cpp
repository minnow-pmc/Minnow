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
 
#include "Device_Stepper.h"
#include "response.h"

extern uint8_t checkDigitalPin(uint8_t pin);

uint8_t Device_Stepper::num_steppers = 0;

Device_Stepper::StepperInfoInternal *Device_Stepper::stepper_info_array;
Device_Stepper::StepperAdvancedInfoInternal *Device_Stepper::stepper_advanced_info_array;

//
// Methods
//


uint8_t Device_Stepper::Init(uint8_t num_devices)
{
  if (num_steppers != 0)
  {
    generate_response_msg_addPGM(PMSG(MSG_ERR_ALREADY_INITIALIZED));
    return PARAM_APP_ERROR_TYPE_FAILED;
  }
  if (num_devices == 0)
    return APP_ERROR_TYPE_SUCCESS;

  if (num_devices > MAX_STEPPERS)
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE;

  uint8_t retval = AxisInfo::Init(num_devices);
  if (retval != APP_ERROR_TYPE_SUCCESS)
    return retval;
  
  uint8_t *memory = (uint8_t*)malloc(num_devices * sizeof(StepperInfoInternal));
  if (memory == 0)
  {
    generate_response_msg_addPGM(PMSG(MSG_ERR_INSUFFICIENT_MEMORY));
    return PARAM_APP_ERROR_TYPE_FAILED;
  }

  stepper_info_array = (StepperInfoInternal *) memory;
  stepper_advanced_info_array = 0; // initialized on demand only

  memset(stepper_info_array, 0xFF, num_devices * sizeof(StepperInfoInternal));

  num_steppers = num_devices;
  
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t Device_Stepper::SetEnablePin(uint8_t device_number, uint8_t enable_pin)
{
  if (device_number >= num_steppers)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;
  
  uint8_t retval = checkDigitalPin(enable_pin);
  if (retval != APP_ERROR_TYPE_SUCCESS)
    return retval;
  
  retval = AxisInfo::SetStepperEnablePin(device_number, enable_pin);
  if (retval != APP_ERROR_TYPE_SUCCESS)
    return retval;
  
  stepper_info_array[device_number].enable_pin = enable_pin;
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t Device_Stepper::SetDirectionPin(uint8_t device_number, uint8_t direction_pin)
{
  if (device_number >= num_steppers)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;
  
  uint8_t retval = checkDigitalPin(direction_pin);
  if (retval != APP_ERROR_TYPE_SUCCESS)
    return retval;
   
  retval = AxisInfo::SetStepperDirectionPin(device_number, direction_pin);
  if (retval != APP_ERROR_TYPE_SUCCESS)
    return retval;
  
  stepper_info_array[device_number].direction_pin = direction_pin;
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t Device_Stepper::SetStepPin(uint8_t device_number, uint8_t step_pin)
{
  if (device_number >= num_steppers)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;
  
  uint8_t retval = checkDigitalPin(step_pin);
  if (retval != APP_ERROR_TYPE_SUCCESS)
    return retval;
  
  retval = AxisInfo::SetStepperStepPin(device_number, step_pin);
  if (retval != APP_ERROR_TYPE_SUCCESS)
    return retval;
  
  stepper_info_array[device_number].step_pin = step_pin;
  return APP_ERROR_TYPE_SUCCESS;
}
