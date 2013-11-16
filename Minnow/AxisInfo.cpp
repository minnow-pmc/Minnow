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
 
#include "AxisInfo.h"
#include "response.h"
#include "Device_Stepper.h"
#include "Device_InputSwitch.h"

uint8_t AxisInfo::num_axes = 0;

AxisInfoInternal *AxisInfo::axis_info_array;

uint16_t AxisInfo::underrun_queue_low_level;
uint16_t AxisInfo::underrun_queue_high_level;
uint16_t AxisInfo::underrun_queue_low_time;
uint16_t AxisInfo::underrun_queue_high_time;

BITMASK(MAX_STEPPERS) AxisInfo::stepper_enable_state;
BITMASK(MAX_ENDSTOPS) AxisInfo::endstop_enable_state;
BITMASK(MAX_ENDSTOPS) AxisInfo::endstop_trigger_level;

//
// Methods
//

uint8_t AxisInfo::Init(uint8_t num_devices)
{
  if (num_devices == num_axes)
    return APP_ERROR_TYPE_SUCCESS;
  if (num_axes != 0)
  {
    generate_response_msg_addPGM(PMSG(MSG_ERR_ALREADY_INITIALIZED));
    return PARAM_APP_ERROR_TYPE_FAILED;
  }

  if (num_devices > MAX_STEPPERS)
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE;

  uint8_t *memory = (uint8_t*)malloc(num_devices * sizeof(AxisInfoInternal));
  if (memory == 0)
  {
    generate_response_msg_addPGM(PMSG(MSG_ERR_INSUFFICIENT_MEMORY));
    return PARAM_APP_ERROR_TYPE_FAILED;
  }

  axis_info_array = (AxisInfoInternal *) memory;

  memset(axis_info_array, 0, num_devices * sizeof(AxisInfoInternal));
  
  for (uint8_t i=0; i<num_devices; i++)
  {
    axis_info_array[i].stepper_number = i;
  }

  num_axes = num_devices;
  
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t AxisInfo::SetStepperEnablePin(uint8_t axis_number, uint8_t enable_pin)
{
  if (axis_number >= num_axes)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;
  axis_info_array[axis_number].stepper_enable_reg = (uint8_t*)portOutputRegister(digitalPinToPort(enable_pin));
  axis_info_array[axis_number].stepper_enable_bit = digitalPinToBitMask(enable_pin);
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t AxisInfo::SetStepperEnableInvert(uint8_t axis_number, bool enable_invert)
{
  if (axis_number >= num_axes)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;
  axis_info_array[axis_number].stepper_enable_invert = enable_invert;
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t AxisInfo::SetStepperDirectionPin(uint8_t axis_number, uint8_t direction_pin)
{
  if (axis_number >= num_axes)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;
  axis_info_array[axis_number].stepper_direction_reg = (uint8_t*)portOutputRegister(digitalPinToPort(direction_pin));
  axis_info_array[axis_number].stepper_direction_bit = digitalPinToBitMask(direction_pin);
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t AxisInfo::SetStepperDirectionInvert(uint8_t axis_number, bool direction_invert)
{
  if (axis_number >= num_axes)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;
  axis_info_array[axis_number].stepper_direction_invert = direction_invert;
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t AxisInfo::SetStepperStepPin(uint8_t axis_number, uint8_t step_pin)
{
  if (axis_number >= num_axes)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;
  axis_info_array[axis_number].stepper_step_reg = (uint8_t*)portOutputRegister(digitalPinToPort(step_pin));
  axis_info_array[axis_number].stepper_step_bit = digitalPinToBitMask(step_pin);
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t AxisInfo::SetStepperStepInvert(uint8_t axis_number, bool step_invert)
{
  if (axis_number >= num_axes)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;
  axis_info_array[axis_number].stepper_step_invert = step_invert;
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t AxisInfo::ClearEndstops(uint8_t axis_number)
{
  if (axis_number >= num_axes)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;
  axis_info_array[axis_number].min_endstops_configured = 0;
  axis_info_array[axis_number].max_endstops_configured = 0;
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t AxisInfo::SetMinEndstopDevice(uint8_t axis_number, uint8_t input_switch_number, bool trigger_level)
{
  if (axis_number >= num_axes)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;
  if (input_switch_number >= MAX_ENDSTOPS || !Device_InputSwitch::IsInUse(input_switch_number))
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE;
  axis_info_array[axis_number].min_endstops_configured |= (1 << input_switch_number);
  if (trigger_level)
    endstop_trigger_level |= (1 << input_switch_number);
  else
    endstop_trigger_level &= ~(1 << input_switch_number);
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t AxisInfo::SetMaxEndstopDevice(uint8_t axis_number, uint8_t input_switch_number, bool trigger_level)
{
  if (axis_number >= num_axes)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;
  if (input_switch_number >= MAX_ENDSTOPS || !Device_InputSwitch::IsInUse(input_switch_number))
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE;
  axis_info_array[axis_number].max_endstops_configured |= (1 << input_switch_number);
  if (trigger_level)
    endstop_trigger_level |= (1 << input_switch_number);
  else
    endstop_trigger_level &= ~(1 << input_switch_number);
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t AxisInfo::SetAxisMaxRate(uint8_t axis_number, uint16_t rate)
{
  uint8_t retval = Device_Stepper::ValidateConfig(axis_number);
  if (retval != APP_ERROR_TYPE_SUCCESS)
    return retval;
  axis_info_array[axis_number].max_rate = rate;
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t AxisInfo::SetUnderrunRate(uint8_t axis_number, uint16_t rate)
{
  uint8_t retval = Device_Stepper::ValidateConfig(axis_number);
  if (retval != APP_ERROR_TYPE_SUCCESS)
    return retval;
  axis_info_array[axis_number].underrun_max_rate = rate;
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t AxisInfo::SetUnderrunAccelRate(uint8_t axis_number, uint32_t accel_rate)
{
  uint8_t retval = Device_Stepper::ValidateConfig(axis_number);
  if (retval != APP_ERROR_TYPE_SUCCESS)
    return retval;
  axis_info_array[axis_number].underrun_accel_rate = accel_rate;
  return APP_ERROR_TYPE_SUCCESS;
}
