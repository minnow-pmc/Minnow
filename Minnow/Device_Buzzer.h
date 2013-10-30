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
#include "SoftPwmState.h"

//
// Currently just support directly connected buzzers. Although
// this allows multiple buzzers to be defined, the basic Tone library
// only support generating a tone on one buzzer at a time.
// This may also interfere with hardware PWM on pins 3 and 11 on Arduino 
// processors which don't have a separate PWM hardware interrupt 
// available (the Arduino megas don't have this problem)
// See Arduino tone() documentation.
//
class Device_Buzzer
{
public:

  static uint8_t Init(uint8_t num_buzzers);
  
  FORCE_INLINE static uint8_t GetNumDevices()
  {
    return num_buzzers;
  }
  
  FORCE_INLINE static bool IsInUse(uint8_t device_number)
  {
    return (device_number < num_buzzers 
      && buzzer_pins[device_number] != 0xFF);
  }

  FORCE_INLINE static bool GetActiveState(uint8_t device_number)
  {
    return !buzzer_disabled[device_number];
  }

  FORCE_INLINE static uint8_t GetPin(uint8_t device_number)
  {
    return buzzer_pins[device_number];
  }
  
  // returns APP_ERROR_TYPE_SUCCESS or error code
  static uint8_t SetPin(uint8_t device_number, uint8_t pin);

  FORCE_INLINE static void WriteState(uint8_t device_number, uint16_t frequency)
  {
    if (frequency > 0)
    {
      tone(buzzer_pins[device_number], frequency);
      buzzer_disabled[device_number] = false;
    }
    else
    {
      noTone(buzzer_pins[device_number]);
      buzzer_disabled[device_number] = true;
    }
  }

private:
  static uint8_t num_buzzers;
  static uint8_t *buzzer_pins;
  static bool *buzzer_disabled;
};

#endif


    