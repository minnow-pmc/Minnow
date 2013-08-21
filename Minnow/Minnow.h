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

#ifndef MINNOW_H
#define MINNOW_H

#define  FORCE_INLINE __attribute__((always_inline)) inline

#include <stdint.h>

#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#ifndef AT90USB
#define  HardwareSerial_h // trick to disable the standard HardwareSerial
#endif

#include "Arduino.h"

#include "config.h"
#include "language.h"
#include "debug.h"

#include "HwSerial.h"

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#ifdef AT90USB
  #define PSERIAL Serial
#else
  extern HardwareSerial HwSerial;
  #define PSERIAL HwSerial
#endif

#include "debug.h"

#define MINNOW_FIRMWARE_NAME          "Minnow"
#define MINNOW_FIRMWARE_VERSION_MAJOR 0
#define MINNOW_FIRMWARE_VERSION_MINOR 1

//
// Public Routines
//

void emergency_stop();
void die();

//
// Public Variables
//

// Receive order related
extern uint8_t order_code;
extern uint8_t control_byte;
extern uint8_t parameter_length;
extern uint8_t * const parameter_value;

// Stopped state related
extern bool is_stopped;
extern bool stopped_is_acknowledged;
extern uint8_t stopped_type; 
extern uint8_t stopped_cause; 
extern const char *stopped_reason; 

//
// Utility Macros
//

#ifndef CRITICAL_SECTION_START
  #define CRITICAL_SECTION_START  unsigned char _sreg = SREG; cli();
  #define CRITICAL_SECTION_END    SREG = _sreg;
#endif //CRITICAL_SECTION_START

#endif
