/*
  Minnow Pacemaker client firmware.
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
  
*/

//
// Stepper and Endstop 
//

#ifndef DEVICE_STEPPER_H
#define DEVICE_STEPPER_H

#include "AxisInfo.h"

// This class represents just the control information associate with the stepper 
// motor device - not to the associated movement infomation which is primary
// aggregated in AxisInfo.
class Device_Stepper
{
public:

  static uint8_t Init(uint8_t num_stepper_devices);
  
  FORCE_INLINE static uint8_t GetNumDevices()
  {
    return num_steppers;
  }
  
  FORCE_INLINE static bool IsInUse(uint8_t device_number)
  {
    return (device_number < num_steppers 
      && stepper_info_array[device_number].enable_pin != 0xFF);
  }

  FORCE_INLINE static bool ValidateConfig(uint8_t device_number)
  {
    if (device_number >= num_steppers)
      return false;
    StepperInfoInternal *stepper_info = &stepper_info_array[device_number];
    return (stepper_info->enable_pin != 0xFF 
        && stepper_info->direction_pin != 0xFF
        && stepper_info->step_pin != 0xFF);
  }

  FORCE_INLINE static uint8_t GetEnablePin(uint8_t device_number)
  {
    return stepper_info_array[device_number].enable_pin;
  }
  FORCE_INLINE static bool GetEnableInvert(uint8_t device_number)
  {
    return AxisInfo::GetStepperEnableInvert(device_number);
  }
  
  FORCE_INLINE static uint8_t GetDirectionPin(uint8_t device_number)
  {
    return stepper_info_array[device_number].direction_pin;
  }
  FORCE_INLINE static bool GetDirectionInvert(uint8_t device_number)
  {
    return AxisInfo::GetStepperDirectionInvert(device_number);
  }
  
  FORCE_INLINE static uint8_t GetStepPin(uint8_t device_number)
  {
    return stepper_info_array[device_number].step_pin;
  }
  FORCE_INLINE static bool GetStepInvert(uint8_t device_number)
  {
    return AxisInfo::GetStepperStepInvert(device_number);
  }
  
  FORCE_INLINE static void WriteEnableState(uint8_t device_number, bool enable)
  {
    // defer to AxisInfo
    AxisInfo::WriteStepperEnableState(device_number, enable);
  }
  
  // returns APP_ERROR_TYPE_SUCCESS or error code
  static uint8_t SetEnablePin(uint8_t device_number, uint8_t pin);
  static uint8_t SetDirectionPin(uint8_t device_number, uint8_t pin);
  static uint8_t SetStepPin(uint8_t device_number, uint8_t pin);

  FORCE_INLINE static uint8_t SetEnableInvert(uint8_t device_number, bool value)
  {
    return AxisInfo::SetStepperEnableInvert(device_number, value);
  }
  FORCE_INLINE static uint8_t SetDirectionInvert(uint8_t device_number, bool value)
  {
    return AxisInfo::SetStepperDirectionInvert(device_number, value);
  }
  FORCE_INLINE static uint8_t SetStepInvert(uint8_t device_number, bool value)
  {
    return AxisInfo::SetStepperStepInvert(device_number, value);
  }

  // TODO add get and sets for advanced state
private:

  struct StepperInfoInternal
  {
    uint8_t enable_pin;
    uint8_t direction_pin;
    uint8_t step_pin;
  };

  struct StepperAdvancedInfoInternal
  {
    uint8_t vref_digipot_pin;
    uint8_t vref_digitpot_value;
    uint8_t ms1_pin;
    uint8_t ms2_pin;
    uint8_t ms3_pin;
    uint8_t ms_value; // bitmask
    uint8_t fault_pin;  
  };
  
  static uint8_t num_steppers;
  static StepperInfoInternal *stepper_info_array;
  static StepperAdvancedInfoInternal *stepper_advanced_info_array;
};

#endif


    