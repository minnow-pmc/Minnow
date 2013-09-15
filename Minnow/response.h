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

#ifndef RESPONSE_H
#define RESPONSE_H

#include <stdint.h>

//
// Setup a new response buffer. 
//
// The expected_length_excluding_msg argument indicates the offset where an optional 
// trailing UTF8 string message can be built. This argument has no effect if none of the 
// generate_response_msg_add functions are called. 
//
// A value of 0xFF for expected_length_excluding_msg indicates that no message is expected
//
void generate_response_start(uint8_t response_code, uint8_t expected_length_excluding_msg = 0xFF);
void generate_response_transport_error_start(uint8_t transport_error, uint8_t control_byte);

//
// These method append data to the reponse Parameter.
//
void generate_response_data_addbyte(uint8_t value);
void generate_response_data_add(uint8_t value);
void generate_response_data_add(int8_t value);
void generate_response_data_add(uint16_t value);
void generate_response_data_add(int16_t value);
void generate_response_data_add(uint32_t value);
void generate_response_data_add(int32_t value);
void generate_response_data_add(const uint8_t *value, uint8_t length);
void generate_response_data_add(const char *str);
void generate_response_data_addPGM(const char *str);
void generate_response_data_addlen(uint8_t value);
void generate_response_data_clear();
uint8_t *generate_response_data_ptr();
uint8_t generate_response_data_len();

//
// The following methods allow for a trailing UTF8 string to be appended
// after the expected data length (expected_length_excluding_msg) in the 
// response Parameter buffer.
//
void generate_response_msg_addbyte(uint8_t value);
void generate_response_msg_add(uint8_t value);
void generate_response_msg_add(int8_t value);
void generate_response_msg_add(uint16_t value);
void generate_response_msg_add(int16_t value);
void generate_response_msg_add(uint32_t value);
void generate_response_msg_add(int32_t value);
void generate_response_msg_add(const uint8_t *value, uint8_t length);
void generate_response_msg_add(const char *str);
void generate_response_msg_addlen(uint8_t value);
void generate_response_msg_addPGM(const char *str);
void generate_response_msg_clear();
uint8_t *generate_response_msg_ptr();
uint8_t generate_response_msg_len();

//
// This sends the prepared response buffer
//
void generate_response_send();

//
// Convenience Functions
//

void send_OK_response();
void send_insufficient_bytes_error_response(uint8_t expected_num_bytes);
void send_insufficient_bytes_error_response(uint8_t expected_num_bytes, uint8_t rcvd_num_bytes);
void send_app_error_at_offset_response(uint8_t error_type, uint8_t parameter_offset);
void send_app_error_response(uint8_t error_type, const char *msg_pstr);
void send_app_error_response(uint8_t error_type, const char *msg_pstr, uint16_t msg_num_value);
void send_app_error_response(uint8_t error_type, const char *msg_pstr, const char *msg_str_value);
void send_failed_response(const char* reason);
void send_stopped_response();

#endif