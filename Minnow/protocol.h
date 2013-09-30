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
// Definitions for Pacemaker Commands and Parameter Values
//

#ifndef PROTOCOL_H
#define PROTOCOL_H

//
// Packet Framing Defines
//

#define SYNC_BYTE_ORDER_VALUE       0x23
#define SYNC_BYTE_RESPONSE_VALUE    0x42

#define CONTROL_BYTE_SEQUENCE_NUMBER_MASK       0x0F
#define CONTROL_BYTE_ORDER_HOST_RESET_BIT       0x10
#define CONTROL_BYTE_RESPONSE_EVENT_BIT         0x10
#define CONTROL_BYTE_RESPONSE_DEBUG_BIT         0x80

#define PM_SYNC_BYTE_OFFSET         0
#define PM_ORDER_CODE_OFFSET        1
#define PM_CONTROL_BYTE_OFFSET      2
#define PM_LENGTH_BYTE_OFFSET       3
#define PM_PARAMETER_OFFSET         4
#define PM_HEADER_SIZE              PM_PARAMETER_OFFSET
    
#define MAX_FRAME_COMPLETION_DELAY_MS           20

//
// Protocol Defines
//

#define PM_PROTCOL_VERSION_MAJOR          0x0
#define PM_PROTCOL_VERSION_MINOR          0x1

#define PM_EXTENSION_STEPPER_CONTROL      0x0
#define PM_EXTENSION_QUEUED_CMD           0x1
#define PM_EXTENSION_BASIC_MOVE           0x2
#define PM_EXTENSION_ERROR_REPORTING      0x3

#define PM_DEVICE_TYPE_SWITCH_INPUT       0x1
#define PM_DEVICE_TYPE_SWITCH_OUTPUT      0x2
#define PM_DEVICE_TYPE_PWM_OUTPUT         0x3
#define PM_DEVICE_TYPE_STEPPER            0x4
#define PM_DEVICE_TYPE_HEATER             0x5
#define PM_DEVICE_TYPE_TEMP_SENSOR        0x6
#define PM_DEVICE_TYPE_BUZZER             0x7

#define PM_HARDWARE_TYPE_GENERIC_ARDUINO  0x1
#define PM_FIRMWARE_TYPE_MINNOW           0x2

#define PM_DEVICE_TYPE_INVALID            0x0
#define PM_DEVICE_NUMBER_INVALID          0Xff
#define PM_TEMPERATURE_INVALID            0x7fff

//
// Response Codes
//

// Transport Layer Responses (0x0 - 0xF)
#define RSP_FRAME_RECEIPT_ERROR           0x0

// Application Layer Responses
#define RSP_OK                            0x10
#define RSP_APPLICATION_ERROR             0x11
#define RSP_STOPPED                       0x12
#define RSP_ORDER_SPECIFIC_ERROR          0x13

//
// Response Parameter Value Definitions
//

// Frame Receipt Error Response Parameter values
#define PARAM_FRAME_RECEIPT_ERROR_TYPE_BAD_FRAME          0x0
#define PARAM_FRAME_RECEIPT_ERROR_TYPE_BAD_CHECK_CODE     0x1
#define PARAM_FRAME_RECEIPT_ERROR_TYPE_UNABLE_TO_ACCEPT   0x2

// Application Error Response Parameter values
#define PARAM_APP_ERROR_TYPE_UNKNOWN_ORDER                0x1
#define PARAM_APP_ERROR_TYPE_BAD_PARAMETER_FORMAT         0x2
#define PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE          0x3
#define PARAM_APP_ERROR_TYPE_INVALID_DEVICE_TYPE          0x4
#define PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER        0x5
#define PARAM_APP_ERROR_TYPE_INCORRECT_MODE               0x6
#define PARAM_APP_ERROR_TYPE_BUSY                         0x7
#define PARAM_APP_ERROR_TYPE_FAILED                       0x8
#define PARAM_APP_ERROR_TYPE_FIRMWARE_ERROR               0x9

// Stopped Response Parameter values
#define PARAM_STOPPED_STATE_UNACKNOWLEDGED                0x0
#define PARAM_STOPPED_STATE_ACKNOWLEDGED                  0x1

#define PARAM_STOPPED_TYPE_ONE_TIME_OR_CLEARED            0x0
#define PARAM_STOPPED_TYPE_PERSISTS                       0x1
#define PARAM_STOPPED_TYPE_UNRECOVERABLE                  0x2

#define PARAM_STOPPED_CAUSE_RESET                         0x0
#define PARAM_STOPPED_CAUSE_ENDSTOP_HIT                   0x1
#define PARAM_STOPPED_CAUSE_MOVEMENT_ERROR                0x2
#define PARAM_STOPPED_CAUSE_THERMAL_ERROR                 0x3
#define PARAM_STOPPED_CAUSE_DEVICE_FAULT                  0x4
#define PARAM_STOPPED_CAUSE_ELECTRICAL_ERROR              0x5
#define PARAM_STOPPED_CAUSE_FIRMWARE_ERROR                0x6
#define PARAM_STOPPED_CAUSE_USER_REQUEST                  0x7
#define PARAM_STOPPED_CAUSE_OTHER_ERROR                   0x8

//
// Order Codes
//

// Basic Orders
#define ORDER_RESUME                           0x00
#define ORDER_REQUEST_INFORMATION              0x01
#define ORDER_DEVICE_NAME                      0x02
#define ORDER_REQUEST_TEMPERATURE_READING      0x03
#define ORDER_GET_HEATER_CONFIGURATION         0x04
#define ORDER_CONFIGURE_HEATER                 0x05
#define ORDER_SET_HEATER_TARGET_TEMP           0x06
#define ORDER_GET_INPUT_SWITCH_STATE           0x07
#define ORDER_SET_OUTPUT_SWITCH_STATE          0x08
#define ORDER_SET_PWM_OUTPUT_STATE             0x09
#define ORDER_WRITE_FIRMWARE_CONFIG_VALUE      0x0a
#define ORDER_READ_FIRMWARE_CONFIG_VALUE       0x0b
#define ORDER_TRAVERSE_FIRMWARE_CONFIG         0x18
#define ORDER_GET_FIRMWARE_CONFIG_PROPERTIES   0x1a
#define ORDER_EMERGENCY_STOP                   0x0c

#define ORDER_RESET                            0x7f

// Stepper Control
#define ORDER_ACTIVATE_STEPPER_CONTROL         0x0d
#define ORDER_ENABLE_DISABLE_STEPPERS          0x0e
#define ORDER_CONFIGURE_ENDSTOPS               0x0f
#define ORDER_ENABLE_DISABLE_ENDSTOPS          0x10

// Queued Command Extension
#define ORDER_QUEUE_COMMAND_BLOCKS             0x12
#define ORDER_CLEAR_COMMAND_QUEUE              0x17

// Movement Orders
#define ORDER_CONFIGURE_AXIS_MOVEMENT_RATES    0x13
#define ORDER_CONFIGURE_UNDERRUN_PARAMS        0x19

//
// Order Parameter Values
//

// Resume Order
#define PARAM_RESUME_TYPE_QUERY                           0x0
#define PARAM_RESUME_TYPE_ACKNOWLEDGE                     0x1
#define PARAM_RESUME_TYPE_CLEAR                           0x2

// Request Information Order
#define PARAM_REQUEST_INFO_FIRMWARE_NAME                  0x0
#define PARAM_REQUEST_INFO_BOARD_SERIAL_NUMBER            0x1
#define PARAM_REQUEST_INFO_BOARD_NAME                     0x2
#define PARAM_REQUEST_INFO_GIVEN_NAME                     0x3
#define PARAM_REQUEST_INFO_PROTO_VERSION_MAJOR            0x4
#define PARAM_REQUEST_INFO_PROTO_VERSION_MINOR            0x5
#define PARAM_REQUEST_INFO_SUPPORTED_EXTENSIONS           0x6
#define PARAM_REQUEST_INFO_FIRMWARE_TYPE                  0x7
#define PARAM_REQUEST_INFO_FIRMWARE_VERSION_MAJOR         0x8
#define PARAM_REQUEST_INFO_FIRMWARE_VERSION_MINOR         0x9
#define PARAM_REQUEST_INFO_HARDWARE_TYPE                  0xa
#define PARAM_REQUEST_INFO_HARDWARE_REVISION              0xb
#define PARAM_REQUEST_INFO_NUM_STEPPERS                   0xc
#define PARAM_REQUEST_INFO_NUM_HEATERS                    0xd
#define PARAM_REQUEST_INFO_NUM_PWM_OUTPUTS                0xe
#define PARAM_REQUEST_INFO_NUM_TEMP_SENSORS               0xf
#define PARAM_REQUEST_INFO_NUM_SWITCH_INPUTS              0x10
#define PARAM_REQUEST_INFO_NUM_SWITCH_OUTPUTS             0x11
#define PARAM_REQUEST_INFO_NUM_BUZZERS                    0x12

// Get Heater Configuration
#define PARAM_HEATER_CONFIG_INTERNAL_SENSOR_CONFIG        0x0
#define PARAM_HEATER_CONFIG_HOST_SENSOR_CONFIG            0x1

//
// Device Status Values
#define DEVICE_STATUS_ACTIVE                              0x0
#define DEVICE_STATUS_INACTIVE                            0x1
#define DEVICE_STATUS_CONFIG_ERROR                        0x2
#define DEVICE_STATUS_DEVICE_FAULT                        0x3

//
// Queue Command Types
//
#define QUEUE_COMMAND_ORDER_WRAPPER                       1
#define QUEUE_COMMAND_DELAY                               2
#define QUEUE_COMMAND_LINEAR_MOVE                         3
#define QUEUE_COMMAND_SET_ACTIVE_TOOLHEAD                 4

//
// Queue Command Error Types
//
#define QUEUE_COMMAND_ERROR_TYPE_QUEUE_FULL               0x1       
#define QUEUE_COMMAND_ERROR_TYPE_UNKNOWN_COMMAND_BLOCK    0x2
#define QUEUE_COMMAND_ERROR_TYPE_MALFORMED_BLOCK          0x3
#define QUEUE_COMMAND_ERROR_TYPE_ERROR_IN_COMMAND_BLOCK   0x4


#endif