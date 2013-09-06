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

#include "response.h"

NonVolatileConfigurationStore NVConfigStore;

void 
NonVolatileConfigurationStore::Initialize()
{
  // TODO
}
  
void 
NonVolatileConfigurationStore::ResetDefault()
{
  // TODO
}

int8_t 
NonVolatileConfigurationStore::GetBoardIdentity(char *buffer, uint8_t buffer_len) const
{
#if USE_EEPROM
  // TODO
  return -1;
#else
  strncpy(buffer, DEFAULT_BOARD_IDENTITY, buffer_len);
  return min(strlen(DEFAULT_BOARD_IDENTITY), buffer_len);
#endif  
}

int8_t 
NonVolatileConfigurationStore::GetBoardSerialNumber(char *buffer, uint8_t buffer_len) const
{
#if USE_EEPROM
  // TODO
  return -1;
#else
  strncpy(buffer, DEFAULT_BOARD_IDENTITY, buffer_len);
  return min(strlen(DEFAULT_BOARD_IDENTITY), buffer_len);
#endif  
}

int8_t 
NonVolatileConfigurationStore::GetHardwareName(char *buffer, uint8_t buffer_len) const
{
#if USE_EEPROM
  // TODO
  return -1;
#else
  strncpy(buffer, DEFAULT_HARDWARE_NAME, buffer_len);
  return min(strlen(DEFAULT_HARDWARE_NAME), buffer_len);
#endif  
}

uint8_t 
NonVolatileConfigurationStore::GetHardwareType() const
{
#if USE_EEPROM
  // TODO
  return 0xFF;
#else
  return DEFAULT_BOARD_HARDWARE_TYPE;
#endif  
}

uint8_t 
NonVolatileConfigurationStore::GetHardwareRevision() const
{
#if USE_EEPROM
  // TODO
  return 0xFF;
#else
  return DEFAULT_BOARD_HARDWARE_REV;
#endif  
}

int8_t NonVolatileConfigurationStore::GetDeviceName(uint8_t device_type, uint8_t device_number, char *buffer, uint8_t buffer_len) const
{
#if USE_EEPROM
  // TODO
  return -1;
#else
  return -1;
#endif  
}
  
uint8_t NonVolatileConfigurationStore::SetBoardIdentity(const char *buffer)
{
#if USE_EEPROM
  // TODO
  return PARAM_APP_ERROR_TYPE_FIRMWARE_ERROR;
#else
  generate_response_msg_addPGM(PMSG(MSG_ERR_EEPROM_NOT_ENABLED));
  return PARAM_APP_ERROR_TYPE_FAILED;
#endif  
}

uint8_t NonVolatileConfigurationStore::SetBoardSerialNumber(const char *buffer)
{
#if USE_EEPROM
  // TODO
  return PARAM_APP_ERROR_TYPE_FIRMWARE_ERROR;
#else
  generate_response_msg_addPGM(PMSG(MSG_ERR_EEPROM_NOT_ENABLED));
  return PARAM_APP_ERROR_TYPE_FAILED;
#endif  
}

uint8_t NonVolatileConfigurationStore::SetHardwareName(const char *buffer)
{
#if USE_EEPROM
  // TODO
  return PARAM_APP_ERROR_TYPE_FIRMWARE_ERROR;
#else
  generate_response_msg_addPGM(PMSG(MSG_ERR_EEPROM_NOT_ENABLED));
  return PARAM_APP_ERROR_TYPE_FAILED;
#endif  
}

uint8_t NonVolatileConfigurationStore::SetHardwareType(uint8_t type)
{
#if USE_EEPROM
  // TODO
  return PARAM_APP_ERROR_TYPE_FIRMWARE_ERROR;
#else
  generate_response_msg_addPGM(PMSG(MSG_ERR_EEPROM_NOT_ENABLED));
  return PARAM_APP_ERROR_TYPE_FAILED;
#endif  
}

uint8_t NonVolatileConfigurationStore::SetHardwareRevision(uint8_t rev)
{
#if USE_EEPROM
  // TODO
  return PARAM_APP_ERROR_TYPE_FIRMWARE_ERROR;
#else
  generate_response_msg_addPGM(PMSG(MSG_ERR_EEPROM_NOT_ENABLED));
  return PARAM_APP_ERROR_TYPE_FAILED;
#endif  
}

uint8_t NonVolatileConfigurationStore::SetDeviceName(uint8_t device_type, uint8_t device_number, const char *buffer)
{
#if USE_EEPROM
  // TODO
  return PARAM_APP_ERROR_TYPE_FIRMWARE_ERROR;
#else
  generate_response_msg_addPGM(PMSG(MSG_ERR_EEPROM_NOT_ENABLED));
  return PARAM_APP_ERROR_TYPE_FAILED;
#endif  
}

