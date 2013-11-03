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
// Interface for movement axis_number information
//

#ifndef AXISINFO_H
#define AXISINFO_H

#include "Minnow.h"
#include "movement_ISR.h"
#include "Device_Stepper.h"

//
// Structure used internally to store all axis-specific information
// used by movement routines
//
struct AxisInfoInternal
{
  // Configuration
  uint8_t stepper_number;
  
  uint8_t *stepper_enable_reg;
  uint8_t stepper_enable_bit;
  uint8_t stepper_enable_invert; // 0 = active high, 1 = active low

  uint8_t *stepper_direction_reg;
  uint8_t stepper_direction_bit;
  uint8_t stepper_direction_invert; // 0 = high is increasing, 1 = high is decreasing
  
  uint8_t *stepper_step_reg;
  uint8_t stepper_step_bit;
  uint8_t stepper_step_invert; // 0 = active high, 1 = active low
  
  BITMASK(MAX_ENDSTOPS) min_endstops_configured;
  BITMASK(MAX_ENDSTOPS) max_endstops_configured;

  uint16_t max_rate;
  uint16_t underrun_max_rate;
  uint32_t underrun_accel_rate;
  
  // used by ISR
  uint16_t step_event_counter;
};

//
// Configuration interfaces for axis information
//
class AxisInfo
{
public:

  static uint8_t Init(uint8_t num_axes);
  
  FORCE_INLINE static uint8_t GetNumAxes()
  {
    return num_axes;
  }
  
  FORCE_INLINE static bool IsInUse(uint8_t axis_number)
  {
    return Device_Stepper::ValidateConfig(axis_number);
  }

  FORCE_INLINE static bool GetStepperEnableInvert(uint8_t axis_number)
  {
    return axis_info_array[axis_number].stepper_enable_invert;
  }

  FORCE_INLINE static bool GetStepperDirectionInvert(uint8_t axis_number)
  {
    return axis_info_array[axis_number].stepper_enable_invert;
  }

  FORCE_INLINE static bool GetStepperStepInvert(uint8_t axis_number)
  {
    return axis_info_array[axis_number].stepper_enable_invert;
  }

  FORCE_INLINE static uint16_t GetAxisMaxRate(uint8_t axis_number)
  {
    return axis_info_array[axis_number].max_rate;
  }

  FORCE_INLINE static BITMASK(MAX_ENDSTOPS) GetAxisMinEndstops(uint8_t axis_number)
  {
    return axis_info_array[axis_number].min_endstops_configured;
  }
  
  FORCE_INLINE static BITMASK(MAX_ENDSTOPS) GetAxisMaxEndstops(uint8_t axis_number)
  {
    return axis_info_array[axis_number].max_endstops_configured;
  }
 
  // Stepper Configuration (set from Device_Stepper.h)
  static uint8_t SetStepperEnablePin(uint8_t axis_number, uint8_t pin);
  static uint8_t SetStepperDirectionPin(uint8_t axis_number, uint8_t pin);
  static uint8_t SetStepperStepPin(uint8_t axis_number, uint8_t pin);
  static uint8_t SetStepperEnableInvert(uint8_t axis_number, bool invert);
  static uint8_t SetStepperDirectionInvert(uint8_t axis_number, bool invert);
  static uint8_t SetStepperStepInvert(uint8_t axis_number, bool invert);

  // Endstop Configuration
  static uint8_t ClearEndstops(uint8_t axis_number);
  static uint8_t SetMaxEndstopDevice(uint8_t axis_number, uint8_t input_switch_number, bool trigger_level);
  static uint8_t SetMinEndstopDevice(uint8_t axis_number, uint8_t input_switch_number, bool trigger_level);

  // Movement rate configuration
  static uint8_t SetAxisMaxRate(uint8_t axis_number, uint16_t rate);

  // Underrun Avoidance Configuration
  FORCE_INLINE static uint16_t GetUnderrunRate(uint8_t axis_number)
  {
    return axis_info_array[axis_number].underrun_max_rate;
  }
  FORCE_INLINE static uint32_t GetUnderrunAccelRate(uint8_t axis_number)
  {
    return axis_info_array[axis_number].underrun_accel_rate;
  }

  static uint8_t SetUnderrunRate(uint8_t axis_number, uint16_t accel_rate);
  static uint8_t SetUnderrunAccelRate(uint8_t axis_number, uint32_t accel_rate);

  
  static uint8_t SetUnderrunQueueTimeLowWaterMark(uint16_t millis);
  static uint8_t SetUnderrunQueueTimeHighWaterMark(uint16_t millis);
  static uint8_t SetUnderrunQueueStepsLowWaterMark(uint8_t level);
  static uint8_t SetUnderrunQueueStepsHighWaterMark(uint8_t level);
  
  static FORCE_INLINE uint16_t GetUnderrunDisableQueueTime()
  {
    return underrun_disable_queue_time;
  }

  static FORCE_INLINE uint8_t GetUnderrunQueueLowWaterMark()
  {
    return underrun_queue_low_level;
  }

  static FORCE_INLINE uint8_t GetUnderrunQueueHighWaterMark()
  {
    return underrun_queue_high_level;
  }
  
  // Enable/disable state
  static FORCE_INLINE bool WriteStepperEnableState(uint8_t axis_number, bool enable)
  {
    if (axis_number >= num_axes)
      return false;
    AxisInfoInternal *axis_info = &axis_info_array[axis_number];
    BITMASK(MAX_STEPPERS) axis_bit = (1 << axis_info->stepper_number);
    if (enable != ((stepper_enable_state & axis_bit) != 0))
    {
      if (enable)
      {
        if (!axis_info->stepper_enable_invert)
          *((volatile uint8_t *)axis_info->stepper_enable_reg) |= axis_info->stepper_enable_bit;
        else
          *((volatile uint8_t *)axis_info->stepper_enable_reg) &= ~(axis_info->stepper_enable_bit);
        stepper_enable_state |= axis_bit;
      }
      else
      {
        if (!axis_info->stepper_enable_invert)
          *((volatile uint8_t *)axis_info->stepper_enable_reg) &= ~(axis_info->stepper_enable_bit);
        else
          *((volatile uint8_t *)axis_info->stepper_enable_reg) |= axis_info->stepper_enable_bit;
        stepper_enable_state &= ~axis_bit;
      }
    }
    return true;
  }
  static FORCE_INLINE bool GetStepperEnableState(uint8_t axis_number)
  {
    return stepper_enable_state & (1 << axis_number);
  }
  
  static FORCE_INLINE void WriteEndstopEnableState(uint8_t endstop_number, bool enable)
  {
    if (enable)
      endstop_enable_state |= (1 << endstop_number);
    else
      endstop_enable_state &=  ~(1 << endstop_number);
  }

private:

  friend void check_endstops();
  friend bool check_underrun_condition();
  friend uint8_t enqueue_linear_move_command(const uint8_t *parameter, uint8_t parameter_length);
  friend void update_directions_and_initial_counts();

  static uint8_t num_axes;

  static AxisInfoInternal *axis_info_array; 

  static BITMASK(MAX_STEPPERS) stepper_enable_state;
  
  // endstop information which is stored independently of axis_number
  static BITMASK(MAX_ENDSTOPS) endstop_enable_state;
  static BITMASK(MAX_ENDSTOPS) endstop_trigger_level;
  
  // underrun info
  static uint16_t underrun_disable_queue_time;
  static uint16_t underrun_queue_low_level;
  static uint16_t underrun_queue_high_level;
  static uint16_t underrun_queue_low_time;
  static uint16_t underrun_queue_high_time;
};


#endif


    