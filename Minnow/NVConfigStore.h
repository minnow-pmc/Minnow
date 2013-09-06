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
// Interface to non-volatile store of configuration items which don't 
// need to be held in RAM during execution (e.g., infrequently used attributes
// such as device names). 
//
// [Remember the primary intent of Pacemaker is that the Host holds the 
// majority of the configuration, not the client.]
//

#ifndef NV_CONFIG_STORE_H
#define NV_CONFIG_STORE_H

#include "config.h"
#include <stdint.h>

#define USE_EEPROM 1 // Set to 0 to use default/compiled in values.

#ifdef USE_EEPROM
  #define EEPROM_OFFSET 0 // Offset of first element
  #define EEPROM_NAME_STORE_OFFSET (EEPROM_OFFSET+200) // Start of device name store
  
  #define EEPROM_FORMAT_VERSION 0
#endif

// Maximum String Lengths
#define MAX_BOARD_ID_LENGTH         15
#define MAX_BOARD_SERIAL_NUM_LENGTH 15
#define MAX_HARDWARE_NAME_LENGTH    30
#define MAX_DEVICE_NAME_LENGTH      8

// Default Hardware Identifiers (these are used when resetting EEPROM config or  not using EEPROM)
#define DEFAULT_BOARD_IDENTITY      ""
#define DEFAULT_BOARD_SERIAL_NUMBER ""
#define DEFAULT_HARDWARE_NAME       "Generic Arduino Controller"
#define DEFAULT_HARDWARE_TYPE       1 // as defined in Pacemaker
#define DEFAULT_HARDWARE_REV        0xFF // == invalid

class NonVolatileConfigurationStore
{
public:

  void Initialize();
  
  void ResetDefault();

  int8_t GetBoardIdentity(char *buffer, uint8_t buffer_len) const;
  int8_t GetBoardSerialNumber(char *buffer, uint8_t buffer_len) const;
  int8_t GetHardwareName(char *buffer, uint8_t buffer_len) const;
  uint8_t GetHardwareType() const;
  uint8_t GetHardwareRevision() const;
  int8_t GetDeviceName(uint8_t device_type, uint8_t device_number, char *buffer, uint8_t buffer_len) const;
  
  uint8_t SetBoardIdentity(const char *buffer);
  uint8_t SetBoardSerialNumber(const char *buffer);
  uint8_t SetHardwareName(const char *buffer);
  uint8_t SetHardwareType(uint8_t type);
  uint8_t SetHardwareRevision(uint8_t rev);
  uint8_t SetDeviceName(uint8_t device_type, uint8_t device_number, const char *buffer);

};

extern NonVolatileConfigurationStore NVConfigStore;

#endif//NV_CONFIG_STORE_H
