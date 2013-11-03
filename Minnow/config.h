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
#define AUTODETECT_BAUDRATES { 115200 }
//#define AUTODETECT_BAUDRATES { 115200, 250000 } # no currently working

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

// The following values determines the size of bitmasks used to store some state
// for these device states.
// Note: dynamic allocation of individual devices is still handled separately, so 
// there is no benfit of reducing below 8.
#define MAX_STEPPERS 8    
#define MAX_ENDSTOPS 8
                                                      

//
// Advanced Configuration Options
//


// Only enable the watchdog for reset if you Arduino bootloader supports disabling the watchdog
#define USE_WATCHDOG_FOR_RESET 0

// Incrementing this by 1 will double the software PWM frequency,
// affecting heaters, and the fan if FAN_SOFT_PWM is enabled.
// However, control resolution will be halved for each increment;
// At 0: there are 128 effective control positions with a
// ~8Hz switching period; at 1: there are 64 effective control 
// positions with a ~16Hz switching period.
#define DEFAULT_SOFT_PWM_SCALE 1

// A host timeout is declared if the host does not send a request within this period
#define HOST_TIMEOUT_SECS      30

// Does the Arduino use an AT90USB USB Serial UART?
#if defined (__AVR_AT90USB1287__) || defined (__AVR_AT90USB1286__) || defined (__AVR_AT90USB646__) || defined(__AVR_AT90USB647__)
  #define AT90USB
#endif  

#define MAX_STEP_FREQUENCY 40000 // Max step frequency (5000 pps / half step)

#endif