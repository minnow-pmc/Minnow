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
// This file handles each of the Pacemaker Orders
// 

#ifndef INITIAL_PIN_STATE_H
#define INITIAL_PIN_STATE_H

#include <stdint.h>

// Initial pin state value
#define INITIAL_PIN_STATE_HIGHZ 0xFF
#define INITIAL_PIN_STATE_LOW 0
#define INITIAL_PIN_STATE_HIGH 1
#define INITIAL_PIN_STATE_PULLUP 2

//
// The following defines the macros used but config.h to hardcode the initial 
// state of pins at boot time (this overrides any initial pin state value stored
// in EEPROM)
//

// (the PROGMEM variables are compiled out in units which don't reference them)

#include <avr/pgmspace.h>
 
#define MINNOW_INITIAL_LOW_PINS PROGMEM const uint8_t boot_initial_low_pins[] =
#define MINNOW_INITIAL_HIGH_PINS PROGMEM const uint8_t boot_initial_high_pins[] =
#define MINNOW_INITIAL_PULLUP_PINS PROGMEM const uint8_t boot_initial_pullup_pins[] =

// This is also defined here for convenience of config.h
// This operates by separating commands by a null string and relies on the compilers 
// automatic concatenation of string constants
#define START_MINNOW_INITIAL_CONFIGURATION PROGMEM const char boot_configuration_string[] = ""
#define END_MINNOW_INITIAL_CONFIGURATION ;
#define COMMAND(str) str "\0"

// Convert initial pin state values to/from string representation
const char *stringify_initial_pin_state_value(uint8_t initial_state);
bool parse_initial_pin_state_value(const char *str_value, uint8_t *initial_pin_state);

// Returns true if initial state is hardcoded (ie., through config.h)
bool retrieve_initial_pin_state(uint8_t pin, uint8_t* state);

// This applies the pin state to the pin as well as to EEPROM
uint8_t set_initial_pin_state(uint8_t pin, uint8_t pin_state, bool write_to_eeprom = true);

// Cleanup unused initial EEPROM entries after configuration is complete
void cleanup_initial_pin_state();

// Apply the config.h hardcoded pin state to the pins.
void apply_boot_initial_pin_state();

#endif