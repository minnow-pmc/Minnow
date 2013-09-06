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
// Static configuration defines
//

#ifndef CONFIG_H
#define CONFIG_H

#include "protocol.h"

// This is the serial port that the firmware uses to communicate with the host
#define SERIAL_PORT 0

// On startup, the firmware will cycle through the following baudrates 
// until it finds a match.
#define AUTODETECT_BAUDRATES { 115200, 250000 }

// Languages (uncmment one of the following languages
#define LANGUAGE_CHOICE ENGLISH
//#define LANGUAGE_CHOICE DEUTSCH

// Firmware Config Language (using English allows you to use standard configuration profiles)
#define CONFIG_STRING_LANGUAGE_CHOICE ENGLISH
//#define CONFIG_STRING_LANGUAGE_CHOICE DEUTSCH

//
// RAM sizes
//

#define MIN_STACK_SIZE                          200   // code will ensure that the stack is at least this size
#define MAX_STACK_SIZE                          400   // code will use all space in excess of this for the command queue

#define MAX_RECV_BUF_LEN                        (PM_HEADER_SIZE + 265 + 1)
#define MAX_RESPONSE_PARAM_LENGTH               128

#define MIN_QUEUE_SIZE                          150   // If the firmware cannot allocate this much memory then it will fail
                                                      // (otherwise all excess memory is allocated to the queue)


//
// Advanced Configuration Options
//

// Only enable the watchdog for reset if you Arduino bootloader supports disabling the watchdog
#define USE_WATCHDOG_FOR_RESET 0

// Incrementing this by 1 will double the software PWM frequency,
// affecting heaters, and the fan if FAN_SOFT_PWM is enabled.
// However, control resolution will be halved for each increment;
// at zero value, there are 128 effective control positions.
#define SOFT_PWM_SCALE 0

// Does the Arduino use an AT90USB USB Serial UART?
#if defined (__AVR_AT90USB1287__) || defined (__AVR_AT90USB1286__) || defined (__AVR_AT90USB646__) || defined(__AVR_AT90USB647__)
  #define AT90USB
#endif  

#endif