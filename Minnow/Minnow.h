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

void allocate_command_queue_memory();
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

// a macro to calculate a variable big enough to hold a bitmask of the specified size
#define BITMASK(count) BITMASK_SIZE(count)
#define BITMASK_SIZE(count) BITMASK_SIZE_##count
#define BITMASK_SIZE_1 uint8_t
#define BITMASK_SIZE_2 uint8_t
#define BITMASK_SIZE_3 uint8_t
#define BITMASK_SIZE_4 uint8_t
#define BITMASK_SIZE_5 uint8_t
#define BITMASK_SIZE_6 uint8_t
#define BITMASK_SIZE_7 uint8_t
#define BITMASK_SIZE_8 uint8_t
#define BITMASK_SIZE_9  uint16_t
#define BITMASK_SIZE_10 uint16_t
#define BITMASK_SIZE_11 uint16_t
#define BITMASK_SIZE_12 uint16_t
#define BITMASK_SIZE_13 uint16_t
#define BITMASK_SIZE_14 uint16_t
#define BITMASK_SIZE_15 uint16_t
#define BITMASK_SIZE_16 uint16_t
#define BITMASK_SIZE_17 uint32_t
#define BITMASK_SIZE_18 uint32_t
#define BITMASK_SIZE_19 uint32_t
#define BITMASK_SIZE_20 uint32_t
#define BITMASK_SIZE_21 uint32_t
#define BITMASK_SIZE_22 uint32_t
#define BITMASK_SIZE_23 uint32_t
#define BITMASK_SIZE_24 uint32_t
#define BITMASK_SIZE_25 uint32_t
#define BITMASK_SIZE_26 uint32_t
#define BITMASK_SIZE_27 uint32_t
#define BITMASK_SIZE_28 uint32_t
#define BITMASK_SIZE_29 uint32_t
#define BITMASK_SIZE_30 uint32_t
#define BITMASK_SIZE_31 uint32_t
#define BITMASK_SIZE_32 uint32_t

// calculates the number of elements in a non-zero length array   
#define NUM_ARRAY_ELEMENTS(array) (sizeof(array)/sizeof(array[0]))

#define STRINGIFY_(n) #n
#define STRINGIFY(n) STRINGIFY_(n)

#define ARRAY(...) __VA_ARGS__ 
   
//
// General Constants
//

#define APP_ERROR_TYPE_SUCCESS    0x0

#endif
