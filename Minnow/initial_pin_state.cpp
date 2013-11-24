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

#define BOOT_CONFIG_MACROS
 
#include "initial_pin_state.h" 
#include "Minnow.h" 
#include "protocol.h"
#include "response.h"
#include "order_helpers.h"

#include "Device_InputSwitch.h"
#include "Device_OutputSwitch.h"
#include "Device_PwmOutput.h" 
#include "Device_Heater.h" 
#include "Device_Buzzer.h" 
#include "Device_Stepper.h" 
#include "Device_TemperatureSensor.h" 

#include "NVConfigStore.h" 

//===========================================================================
//=============================imported variables============================
//===========================================================================


//===========================================================================
//=============================private variables=============================
//===========================================================================


//===========================================================================
//=============================public variables=============================
//===========================================================================



//===========================================================================
//=============================ROUTINES=============================
//===========================================================================

const char *stringify_initial_pin_state_value(uint8_t initial_state)
{
  if (initial_state == INITIAL_PIN_STATE_PULLUP)
  {
    return PSTR(CONFIG_STR(PULLUP));
  }
  else if (initial_state == INITIAL_PIN_STATE_HIGH)
  {
    return PSTR(CONFIG_STR(HIGH_STR));
  }
  else if (initial_state == INITIAL_PIN_STATE_LOW)
  {
    return PSTR(CONFIG_STR(LOW_STR));
  }
  else
  {
    return PSTR(CONFIG_STR(HIGHZ));
  }
}


bool parse_initial_pin_state_value(const char *str_value, uint8_t *initial_pin_state)
{
  if (strcmp_P(str_value, PSTR(CONFIG_STR(PULLUP))) == 0)
  {
    *initial_pin_state = INITIAL_PIN_STATE_PULLUP;
    return true;
  }
  else if (strcmp_P(str_value, PSTR(CONFIG_STR(HIGH_STR))) == 0)
  {
    *initial_pin_state = INITIAL_PIN_STATE_HIGH;
    return true;
  }
  else if (strcmp_P(str_value, PSTR(CONFIG_STR(LOW_STR))) == 0)
  {
    *initial_pin_state = INITIAL_PIN_STATE_LOW;
    return true;
  }
  else if (strcmp_P(str_value, PSTR(CONFIG_STR(HIGHZ))) == 0)
  {
    *initial_pin_state = INITIAL_PIN_STATE_HIGHZ;
    return true;
  }
  return false;
}


// returns true if initial state is hardcoded
bool retrieve_initial_pin_state(uint8_t pin, uint8_t* initial_state)
{
  uint8_t i;
  for (i = 0; i<sizeof(boot_initial_pullup_pins); i++)
  {
    if (pin == pgm_read_byte(boot_initial_pullup_pins + i))
    {
      *initial_state = INITIAL_PIN_STATE_PULLUP;
      return true;
    }
  }
  for (i = 0; i<sizeof(boot_initial_low_pins); i++)
  {
    if (pin == pgm_read_byte(boot_initial_low_pins + i))
    {
      *initial_state = INITIAL_PIN_STATE_LOW;
      return true;
    }
  }
  for (i = 0; i<sizeof(boot_initial_high_pins); i++)
  {
    if (pin == pgm_read_byte(boot_initial_high_pins + i))
    {
      *initial_state = INITIAL_PIN_STATE_HIGH;
      return true;
    }
  }
  *initial_state = NVConfigStore::GetInitialPinState(pin);
  return false;
}


uint8_t set_initial_pin_state(uint8_t pin, uint8_t new_initial_state, bool write_to_eeprom)
{
  if (write_to_eeprom)
  {
#if TRACE_INITIAL_PIN_STATE    
    DEBUGPGM("Setting new state for pin ");
    DEBUG((int)pin);
    DEBUG_CH(' ');
    DEBUGLNPGM_P(stringify_initial_pin_state_value(new_initial_state));
#endif    

    uint8_t old_state;
    if (!retrieve_initial_pin_state(pin, &old_state))
    {
      uint8_t retval = NVConfigStore::SetInitialPinState(pin, new_initial_state);
      if (retval != APP_ERROR_TYPE_SUCCESS)
        return retval;
    }
    else
    {
      if (old_state != new_initial_state)    
        return PARAM_APP_ERROR_TYPE_FAILED;
    }
  }

  if (new_initial_state == INITIAL_PIN_STATE_PULLUP)
  {
    pinMode(pin, INPUT);
    digitalWrite(pin, HIGH);
  }
  else if (new_initial_state == INITIAL_PIN_STATE_HIGH)
  {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
  }
  else if (new_initial_state == INITIAL_PIN_STATE_LOW)
  {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
  else
  {
    pinMode(pin, INPUT); // == HIGHZ
    digitalWrite(pin, LOW);
  }
  return APP_ERROR_TYPE_SUCCESS;
}

void cleanup_initial_pin_state()
{
  uint8_t pin;
  uint8_t pin_state;
  uint8_t index = 0;
  // for each pin in the EEPROM
  while ((pin = NVConfigStore::GetNextInitialPinState(&index, &pin_state)) != 0xFF)
  {
    // if in hardcoded list or not found in configuration then remove from EEPROM
    // (by setting to HIGHZ)
    if (retrieve_initial_pin_state(pin, &pin_state) || !is_pin_in_use(pin))
    {
#if TRACE_INITIAL_PIN_STATE
      DEBUGPGM("Removing initial pin state for pin ");
      DEBUGLN((int)pin);
#endif    
      set_initial_pin_state(pin, INITIAL_PIN_STATE_HIGHZ);
    }
  }
}


void apply_boot_initial_pin_state()
{
  uint8_t i, pin, state;
#if TRACE_INITIAL_PIN_STATE    
  const char *msg = PSTR("Setting boot pin ");
#endif  
  for (i = 0; i<sizeof(boot_initial_pullup_pins); i++)
  {
    pin = pgm_read_byte(boot_initial_pullup_pins + i);
#if TRACE_INITIAL_PIN_STATE    
    DEBUGPGM_P(msg);
    DEBUG((int)pin);
    DEBUG_CH(' ');
    DEBUGLNPGM_P(stringify_initial_pin_state_value(INITIAL_PIN_STATE_PULLUP));
#endif    
    set_initial_pin_state(pin, INITIAL_PIN_STATE_PULLUP, false);
  }
  for (i = 0; i<sizeof(boot_initial_low_pins); i++)
  {
    pin = pgm_read_byte(boot_initial_low_pins + i);
#if TRACE_INITIAL_PIN_STATE    
    DEBUGPGM_P(msg);
    DEBUG((int)pin);
    DEBUG_CH(' ');
    DEBUGLNPGM_P(stringify_initial_pin_state_value(INITIAL_PIN_STATE_LOW));
#endif    
    set_initial_pin_state(pin, INITIAL_PIN_STATE_LOW, false);
  }
  for (i = 0; i<sizeof(boot_initial_high_pins); i++)
  {
    pin = pgm_read_byte(boot_initial_high_pins + i);
#if TRACE_INITIAL_PIN_STATE    
    DEBUGPGM_P(msg);
    DEBUG((int)pin);
    DEBUG_CH(' ');
    DEBUGLNPGM_P(stringify_initial_pin_state_value(INITIAL_PIN_STATE_HIGH));
#endif
    set_initial_pin_state(pin, INITIAL_PIN_STATE_HIGH, false);
  }
  i = 0;
  while ((pin = NVConfigStore::GetNextInitialPinState(&i, &state)) != 0xFF)
  {
#if TRACE_INITIAL_PIN_STATE    
    DEBUGPGM("Setting config pin ");
    DEBUG((int)pin);
    DEBUG_CH(' ');
    DEBUGLNPGM_P(stringify_initial_pin_state_value(state));
#endif    
    set_initial_pin_state(pin, state, false);
  }
}


