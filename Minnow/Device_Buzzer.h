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
// Interface for buzzer devices
//

#ifndef DEVICE_BUZZER_H
#define DEVICE_BUZZER_H

#include "Minnow.h"

class Device_Buzzer
{
public:

  static void Init();
  static uint8_t GetNumDevices();
  
  FORCE_INLINE static bool IsInUse(uint8_t device_number)
  {
    return (device_number < MAX_BUZZERS 
      && buzzer_pins[device_number] != 0xFF);
  }

  FORCE_INLINE static uint8_t GetPin(uint8_t device_number)
  {
    return buzzer_pins[device_number];
  }
  
  static bool SetPin(uint8_t device_number, uint8_t pin);

  // Not: this write method is not used for time-critical output pins
  // such as stepper or heater pins
  // TODO look at using digitalWriteFast library
  FORCE_INLINE static void WriteState(uint8_t device_number, uint8_t state)
  {
    analogWrite(buzzer_pins[device_number], state >> 8);
  }

private:
  static uint8_t buzzer_pins[MAX_BUZZERS];
  static bool buzzer_disabled[MAX_BUZZERS];
};

#endif


    