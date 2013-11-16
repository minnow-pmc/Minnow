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

// EEPROM Contents and Layout
#define FORMAT_NUMBER_OFFSET            0
#define FORMAT_NUMBER_LENGTH            1
#define DATA_LENGTH_OFFSET              (FORMAT_NUMBER_OFFSET+FORMAT_NUMBER_LENGTH)
#define DATA_LENGTH_LENGTH              2
#define DEVICE_NAME_TABLE_OFFSET_OFFSET (DATA_LENGTH_OFFSET+DATA_LENGTH_LENGTH)
#define DEVICE_NAME_TABLE_OFFSET_LENGTH 2
#define DEVICE_NAME_LENGTH_OFFSET       (DEVICE_NAME_TABLE_OFFSET_OFFSET+DEVICE_NAME_TABLE_OFFSET_LENGTH)
#define DEVICE_NAME_LENGTH_LENGTH       1
#define HARDWARE_TYPE_OFFSET            (DEVICE_NAME_LENGTH_OFFSET+DEVICE_NAME_LENGTH_LENGTH)
#define HARDWARE_TYPE_LENGTH            1
#define HARDWARE_REV_OFFSET             (HARDWARE_TYPE_OFFSET+HARDWARE_TYPE_LENGTH)
#define HARDWARE_REV_LENGTH             1
#define HARDWARE_NAME_OFFSET            (HARDWARE_REV_OFFSET+HARDWARE_REV_LENGTH)
#define HARDWARE_NAME_LENGTH            30
#define BOARD_ID_OFFSET                 (HARDWARE_NAME_OFFSET+HARDWARE_NAME_LENGTH)
#define BOARD_ID_LENGTH                 15
#define BOARD_SERIAL_NUM_OFFSET         (BOARD_ID_OFFSET+BOARD_ID_LENGTH)
#define BOARD_SERIAL_NUM_LENGTH         15
#define INITIAL_PIN_STATE_TABLE_OFFSET_OFFSET    (BOARD_SERIAL_NUM_OFFSET+BOARD_SERIAL_NUM_LENGTH)
#define INITIAL_PIN_STATE_TABLE_OFFSET_LENGTH    2
#define INITIAL_PIN_STATE_TABLE_LENGTH_OFFSET   (INITIAL_PIN_STATE_TABLE_OFFSET_OFFSET+INITIAL_PIN_STATE_TABLE_OFFSET_LENGTH)
#define INITIAL_PIN_STATE_TABLE_LENGTH_LENGTH   1

// This is the length of all the above data
#define EEPROM_DATA_LENGTH          (INITIAL_PIN_STATE_TABLE_LENGTH_OFFSET+INITIAL_PIN_STATE_TABLE_LENGTH_LENGTH)

// Device Name store information
#if E2END == 511  
  #define MAX_DEVICE_NAME_LENGTH        8
  #define MAX_NUM_DEVICE_NAMES          20 // taking up top 200 bytes
  #define MAX_NUM_INITIAL_PIN_STATES    20 // taking up top 40 bytes
#elif E2END == 1023
  #define MAX_DEVICE_NAME_LENGTH        10
  #define MAX_NUM_DEVICE_NAMES          50 // taking up top 600 bytes
  #define MAX_NUM_INITIAL_PIN_STATES    50 // taking up top 100 bytes
#else  
  #define MAX_DEVICE_NAME_LENGTH        16
  #define MAX_NUM_DEVICE_NAMES          60 // taking up top 1080 bytes
  #define MAX_NUM_INITIAL_PIN_STATES    60 // taking up top 120 bytes
#endif  

#define DEVICE_NAME_TABLE_OFFSET        (E2END+1-((MAX_DEVICE_NAME_LENGTH+2)*MAX_NUM_DEVICE_NAMES)) // Start of device name store
#define INITIAL_PIN_STATE_TABLE_OFFSET    (DEVICE_NAME_TABLE_OFFSET-(MAX_NUM_INITIAL_PIN_STATES*2)) // Start of pin initial states store

#define EEPROM_FORMAT_VERSION       1

// Default Hardware Identifiers (these are used when resetting EEPROM config or  not using EEPROM)
#define DEFAULT_BOARD_IDENTITY      ""
#define DEFAULT_BOARD_SERIAL_NUMBER ""
#define DEFAULT_HARDWARE_NAME       "Generic Arduino Controller"
#define DEFAULT_HARDWARE_TYPE       1 // as defined in Pacemaker
#define DEFAULT_HARDWARE_REV        0xFF // == invalid

class NVConfigStore
{
public:

  static void Initialize();
  
  static void WriteDefaults(bool reset_all);

  static int8_t GetBoardIdentity(char *buffer, uint8_t buffer_len);
  static int8_t GetBoardSerialNumber(char *buffer, uint8_t buffer_len);
  static int8_t GetHardwareName(char *buffer, uint8_t buffer_len);
  static uint8_t GetHardwareType();
  static uint8_t GetHardwareRevision();
  
  static int8_t GetDeviceName(uint8_t device_type, uint8_t device_number, char *buffer, uint8_t buffer_len);

  static uint8_t GetInitialPinState(uint8_t pin); 
  static uint8_t SetInitialPinState(uint8_t pin, uint8_t initial_state); 
  static uint8_t GetNextInitialPinState(uint8_t* index, uint8_t *initial_state); 
  
  static uint8_t SetBoardIdentity(const char *buffer);
  static uint8_t SetBoardSerialNumber(const char *buffer);
  static uint8_t SetHardwareName(const char *buffer);
  static uint8_t SetHardwareType(uint8_t type);
  static uint8_t SetHardwareRevision(uint8_t rev);
  
  static uint8_t SetDeviceName(uint8_t device_type, uint8_t device_number, const char *buffer);

};

#endif//NV_CONFIG_STORE_H
