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
// Implementation of non-volatile store of configuration items which don't 
// need to be held in RAM during execution (e.g., infrequently used attributes
// such as device names).
//

#include "NVConfigStore.h"
#include <stdint.h>

#include "Minnow.h"
#include "response.h"
#include "initial_pin_state.h"

#if USE_EEPROM
#include <avr/eeprom.h>
#endif

#include <avr/pgmspace.h>

#if USE_EEPROM

// replacements for functions missing from avr library
FORCE_INLINE void eeprom_update_byte(uint8_t *address, uint8_t value)
{
  if (eeprom_read_byte(address) != value)
    eeprom_write_byte(address, value);
}

// replacements for functions missing from avr library
FORCE_INLINE void eeprom_update_word(uint16_t *address, uint16_t value)
{
  if (eeprom_read_word(address) != value)
    eeprom_write_word(address, value);
}

uint8_t read_eeprom_string(uint8_t *address, uint8_t max_length, char *buffer, uint8_t buffer_len)
{
  uint8_t i = 0;
  while (i < max_length && i < buffer_len)
  {
    char ch = (char)eeprom_read_byte(address + i);
    if (ch == 0xFF || ch == '\0')
    {
      buffer[i] = '\0';
      return i; 
    }
    buffer[i] = ch;
    i += 1;
  }
  if (i < buffer_len)
    buffer[i] = '\0';
  return i;
}

uint8_t update_eeprom_string(uint8_t *address, uint8_t max_length, const char *str)
{
  uint8_t i = 0;
  while (i < max_length)
  {
    char ch = str[i];
    eeprom_update_byte(address + i, ch);
    if (ch == '\0')
      break;
    i += 1;
  }
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t update_eeprom_string_P(uint8_t *address, uint8_t max_length, const char *pstr)
{
  uint8_t i = 0;
  while (i < max_length)
  {
    char ch = pgm_read_byte(pstr+i);
    eeprom_update_byte(address + i, ch);
    if (ch == '\0')
      break;
    i += 1;
  }
  return APP_ERROR_TYPE_SUCCESS;
}
#endif

void 
NVConfigStore::Initialize()
{
#if USE_EEPROM
  uint8_t format_num = eeprom_read_byte((uint8_t*)FORMAT_NUMBER_OFFSET);
  uint16_t data_length_num = eeprom_read_word((uint16_t*)DATA_LENGTH_OFFSET);
  uint16_t device_name_table_offset = eeprom_read_word((uint16_t*)DEVICE_NAME_TABLE_OFFSET_OFFSET);
  uint8_t device_name_length = eeprom_read_byte((uint8_t*)DEVICE_NAME_LENGTH_OFFSET);
  uint16_t initial_pin_state_table_offset = eeprom_read_word((uint16_t*)INITIAL_PIN_STATE_TABLE_OFFSET_OFFSET);
  uint8_t initial_pin_state_table_length = eeprom_read_byte((uint8_t*)INITIAL_PIN_STATE_TABLE_LENGTH_OFFSET);
  
  if ((format_num < EEPROM_FORMAT_VERSION || format_num == 0xFF)
      || data_length_num < EEPROM_DATA_LENGTH
      || device_name_table_offset != DEVICE_NAME_TABLE_OFFSET
      || device_name_length != MAX_DEVICE_NAME_LENGTH
      || initial_pin_state_table_offset != INITIAL_PIN_STATE_TABLE_OFFSET
      || initial_pin_state_table_length != 2*MAX_NUM_INITIAL_PIN_STATES)
  {
    WriteDefaults(false);
  }
#endif  
}
  
void 
NVConfigStore::WriteDefaults(bool reset_all)
{
#if USE_EEPROM
  // we can improve this to only reset new elements when format number is increased later
  // but as we only have one format version for now, just reset everything
  eeprom_update_byte((uint8_t*)FORMAT_NUMBER_OFFSET, EEPROM_FORMAT_VERSION);
  eeprom_update_word((uint16_t*)DATA_LENGTH_OFFSET, EEPROM_DATA_LENGTH);

  // do we need to reset the device name cache?
  uint16_t old_device_name_table_offset = eeprom_read_word((uint16_t*)DEVICE_NAME_TABLE_OFFSET);
  uint8_t old_device_name_length = eeprom_read_byte((uint8_t*)DEVICE_NAME_LENGTH_OFFSET);
  if (old_device_name_table_offset != DEVICE_NAME_TABLE_OFFSET
      || old_device_name_length != MAX_DEVICE_NAME_LENGTH
      || reset_all)
  {
    for (uint8_t* addr=(uint8_t*)DEVICE_NAME_TABLE_OFFSET; addr<(uint8_t*)E2END; addr += MAX_DEVICE_NAME_LENGTH + 2)
      eeprom_update_byte(addr,0xFF); // we only need to reset the device type to 0xFF
  }
  eeprom_update_word((uint16_t*)DEVICE_NAME_TABLE_OFFSET, DEVICE_NAME_TABLE_OFFSET);
  eeprom_update_byte((uint8_t*)DEVICE_NAME_LENGTH_OFFSET, MAX_DEVICE_NAME_LENGTH);
  
  // do we need to reset the initial pin state cache?
  uint16_t old_initial_pin_state_table_offset = eeprom_read_word((uint16_t*)INITIAL_PIN_STATE_TABLE_OFFSET_OFFSET);
  uint8_t old_initial_pin_state_table_length = eeprom_read_byte((uint8_t*)INITIAL_PIN_STATE_TABLE_LENGTH_OFFSET);
  if (old_initial_pin_state_table_offset != INITIAL_PIN_STATE_TABLE_OFFSET
      || old_initial_pin_state_table_length != 2*MAX_NUM_INITIAL_PIN_STATES
      || reset_all)
  {
    for (uint8_t* addr=(uint8_t*)INITIAL_PIN_STATE_TABLE_OFFSET; 
      addr<(uint8_t*)INITIAL_PIN_STATE_TABLE_OFFSET + (2*MAX_NUM_INITIAL_PIN_STATES); addr += 2)
    {
      eeprom_update_byte(addr,0xFF); // we only need to reset the pin to 0xFF
    }
  }
  eeprom_update_word((uint16_t*)INITIAL_PIN_STATE_TABLE_OFFSET_OFFSET, INITIAL_PIN_STATE_TABLE_OFFSET);
  eeprom_update_byte((uint8_t*)INITIAL_PIN_STATE_TABLE_LENGTH_OFFSET, 2*MAX_NUM_INITIAL_PIN_STATES);

  // reset other values
  eeprom_update_byte((uint8_t*)HARDWARE_TYPE_OFFSET, DEFAULT_HARDWARE_TYPE);
  eeprom_update_byte((uint8_t*)HARDWARE_REV_OFFSET, DEFAULT_HARDWARE_REV);
  update_eeprom_string_P((uint8_t*)HARDWARE_NAME_OFFSET, HARDWARE_NAME_LENGTH, PSTR(DEFAULT_HARDWARE_NAME)); 
  update_eeprom_string_P((uint8_t*)BOARD_ID_OFFSET, BOARD_ID_LENGTH, PSTR(DEFAULT_BOARD_IDENTITY)); 
  update_eeprom_string_P((uint8_t*)BOARD_SERIAL_NUM_OFFSET, BOARD_SERIAL_NUM_LENGTH, PSTR(DEFAULT_BOARD_SERIAL_NUMBER)); 
#endif  
}

int8_t 
NVConfigStore::GetBoardIdentity(char *buffer, uint8_t buffer_len)
{
#if USE_EEPROM
  return read_eeprom_string((uint8_t*)BOARD_ID_OFFSET, BOARD_ID_LENGTH, buffer, buffer_len);
#else
  const char *pstr = PSTR(DEFAULT_BOARD_IDENTITY);
  strncpy_P(buffer, pstr, buffer_len);
  return min(strlen(pstr), buffer_len);
#endif  
}

int8_t 
NVConfigStore::GetBoardSerialNumber(char *buffer, uint8_t buffer_len)
{
#if USE_EEPROM
  return read_eeprom_string((uint8_t*)BOARD_SERIAL_NUM_OFFSET, BOARD_SERIAL_NUM_LENGTH, buffer, buffer_len);
#else
  const char *pstr = PSTR(DEFAULT_BOARD_SERIAL_NUMBER);
  strncpy_P(buffer, pstr, buffer_len);
  return min(strlen(pstr), buffer_len);
#endif  
}

int8_t 
NVConfigStore::GetHardwareName(char *buffer, uint8_t buffer_len)
{
#if USE_EEPROM
  return read_eeprom_string((uint8_t*)HARDWARE_NAME_OFFSET, HARDWARE_NAME_LENGTH, buffer, buffer_len);
#else
  const char *pstr = PSTR(DEFAULT_HARDWARE_NAME);
  strncpy_P(buffer, pstr, buffer_len);
  return min(strlen(pstr), buffer_len);
#endif  
}

uint8_t 
NVConfigStore::GetHardwareType()
{
#if USE_EEPROM
  return eeprom_read_byte((uint8_t*)HARDWARE_TYPE_OFFSET);
#else
  return DEFAULT_HARDWARE_TYPE;
#endif  
}

uint8_t 
NVConfigStore::GetHardwareRevision()
{
#if USE_EEPROM
  return eeprom_read_byte((uint8_t*)HARDWARE_REV_OFFSET);
#else
  return DEFAULT_HARDWARE_REV;
#endif  
}

int8_t NVConfigStore::GetDeviceName(uint8_t device_type, uint8_t device_number, char *buffer, uint8_t buffer_len)
{
#if USE_EEPROM
  uint8_t *address = (uint8_t *)DEVICE_NAME_TABLE_OFFSET;
  while (address < (uint8_t*)E2END)
  {
    if (device_type == eeprom_read_byte(address) && device_number == eeprom_read_byte(address+1))
      return read_eeprom_string(address+2, MAX_DEVICE_NAME_LENGTH, buffer, buffer_len);
    address += MAX_DEVICE_NAME_LENGTH + 2;
  }
  return -1;
#else
  return -1;
#endif  
}
  
uint8_t NVConfigStore::SetBoardIdentity(const char *buffer)
{
#if USE_EEPROM
  return update_eeprom_string((uint8_t*)BOARD_ID_OFFSET, BOARD_ID_LENGTH, buffer);
#else
  generate_response_msg_addPGM(PMSG(MSG_ERR_EEPROM_NOT_ENABLED));
  return PARAM_APP_ERROR_TYPE_FAILED;
#endif  
}

uint8_t NVConfigStore::SetBoardSerialNumber(const char *buffer)
{
#if USE_EEPROM
  return update_eeprom_string((uint8_t*)BOARD_SERIAL_NUM_OFFSET, BOARD_SERIAL_NUM_LENGTH, buffer);
#else
  generate_response_msg_addPGM(PMSG(MSG_ERR_EEPROM_NOT_ENABLED));
  return PARAM_APP_ERROR_TYPE_FAILED;
#endif  
}

uint8_t NVConfigStore::SetHardwareName(const char *buffer)
{
#if USE_EEPROM
  return update_eeprom_string((uint8_t*)HARDWARE_NAME_OFFSET, HARDWARE_NAME_LENGTH, buffer);
#else
  generate_response_msg_addPGM(PMSG(MSG_ERR_EEPROM_NOT_ENABLED));
  return PARAM_APP_ERROR_TYPE_FAILED;
#endif  
}

uint8_t NVConfigStore::SetHardwareType(uint8_t type)
{
#if USE_EEPROM
  eeprom_update_byte((uint8_t*)HARDWARE_TYPE_OFFSET, type);
  return APP_ERROR_TYPE_SUCCESS;
#else
  generate_response_msg_addPGM(PMSG(MSG_ERR_EEPROM_NOT_ENABLED));
  return PARAM_APP_ERROR_TYPE_FAILED;
#endif  
}

uint8_t NVConfigStore::SetHardwareRevision(uint8_t rev)
{
#if USE_EEPROM
  eeprom_update_byte((uint8_t*)HARDWARE_REV_OFFSET, rev);
  return APP_ERROR_TYPE_SUCCESS;
#else
  generate_response_msg_addPGM(PMSG(MSG_ERR_EEPROM_NOT_ENABLED));
  return PARAM_APP_ERROR_TYPE_FAILED;
#endif  
}

uint8_t NVConfigStore::SetDeviceName(uint8_t device_type, uint8_t device_number, const char *buffer)
{
#if USE_EEPROM
  uint8_t *address = (uint8_t*)DEVICE_NAME_TABLE_OFFSET;
  while (address < (uint8_t*)E2END)
  {
    uint8_t type = eeprom_read_byte(address);
    if (type == 0xFF)
    {
      eeprom_update_byte(address,device_type);
      eeprom_update_byte(address+1,device_number);
      return update_eeprom_string(address+2, MAX_DEVICE_NAME_LENGTH, buffer);
    }
    else if (type == device_type && eeprom_read_byte(address+1) == device_number)
    {
      return update_eeprom_string(address+2, MAX_DEVICE_NAME_LENGTH, buffer);
    }
    address += MAX_DEVICE_NAME_LENGTH + 2;
  }
  return PARAM_APP_ERROR_TYPE_FAILED;
#else
  generate_response_msg_addPGM(PMSG(MSG_ERR_EEPROM_NOT_ENABLED));
  return PARAM_APP_ERROR_TYPE_FAILED;
#endif  
}

uint8_t NVConfigStore::GetInitialPinState(uint8_t pin)
{
#if USE_EEPROM
  for (uint8_t *address=(uint8_t*)INITIAL_PIN_STATE_TABLE_OFFSET; 
          address<(uint8_t*)INITIAL_PIN_STATE_TABLE_OFFSET+(2*MAX_NUM_INITIAL_PIN_STATES); address+=2)
  {
    uint8_t read_pin = eeprom_read_byte(address);
    if (read_pin == pin)
      return eeprom_read_byte(address + 1);
  }
  return INITIAL_PIN_STATE_HIGHZ;
#else
  return INITIAL_PIN_STATE_HIGHZ;
#endif  
}

uint8_t NVConfigStore::SetInitialPinState(uint8_t pin, uint8_t initial_state)
{
#if USE_EEPROM
  uint8_t *free_slot_address = 0;
  for (uint8_t *address=(uint8_t*)INITIAL_PIN_STATE_TABLE_OFFSET; 
          address<(uint8_t*)INITIAL_PIN_STATE_TABLE_OFFSET+(2*MAX_NUM_INITIAL_PIN_STATES); address+=2)
  {
    uint8_t read_pin = eeprom_read_byte(address);
    if (read_pin == 0xFF)
    {
      if (free_slot_address == 0)
        free_slot_address = address;
    }
    else if (read_pin == pin)
    {
      if (initial_state == INITIAL_PIN_STATE_HIGHZ)
      {
        // as this is the default initial pin state we actually just remove the entry
        eeprom_update_byte(address,0xFF);
      }
      else
      {
        eeprom_update_byte(address + 1, initial_state);
      }
      return APP_ERROR_TYPE_SUCCESS;
    }
  }
  if (free_slot_address == 0)
    return PARAM_APP_ERROR_TYPE_FAILED;
  eeprom_update_byte(free_slot_address, pin);
  eeprom_update_byte(free_slot_address + 1, initial_state);
  return APP_ERROR_TYPE_SUCCESS;
#else
  generate_response_msg_addPGM(PMSG(MSG_ERR_EEPROM_NOT_ENABLED));
  return PARAM_APP_ERROR_TYPE_FAILED;
#endif  
}

uint8_t NVConfigStore::GetNextInitialPinState(uint8_t *index, uint8_t *initial_state)
{
#if USE_EEPROM
  for (uint8_t *address=(uint8_t*)INITIAL_PIN_STATE_TABLE_OFFSET + (2 * *index); 
          address<(uint8_t*)INITIAL_PIN_STATE_TABLE_OFFSET+(2*MAX_NUM_INITIAL_PIN_STATES); address+=2)
  {
    uint8_t read_pin = eeprom_read_byte(address);
    if (read_pin == 0xFF)
      continue;
    // found next entry
    *index = ((address - (uint8_t*)INITIAL_PIN_STATE_TABLE_OFFSET) / 2) + 1;
    *initial_state = eeprom_read_byte(address + 1);
    return read_pin;
  }
  return 0xFF;
#else
  return 0xFF;
#endif  
}
