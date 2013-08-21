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

// Languages
// 1  English
#define LANGUAGE_CHOICE 1

//
// RAM sizes
//

#define STACK_SIZE                          500

#define MAX_RECV_BUF_LEN                    (PM_HEADER_SIZE + 265 + 1)
#define MAX_RESPONSE_PARAM_LENGTH           128

// Maximum devices which can configured (determines memory allocated to store device configuration)
#define MAX_INPUT_SWITCHES                  6
#define MAX_OUTPUT_SWITCHES                 4
#define MAX_PWM_OUTPUTS                     5
#define MAX_HEATERS                         4
#define MAX_BUZZERS                         1

// Only enable the watchdog for reset if you Arduino bootloader supports disabling the watchdog
//#define USE_WATCHDOG_FOR_RESET

#endif