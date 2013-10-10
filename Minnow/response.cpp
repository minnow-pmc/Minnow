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
// Utility functions for sending responses
//

#include "response.h"
#include "protocol.h"
#include "crc8.h"

#include "Minnow.h"

//===========================================================================
//=============================imported variables============================
//===========================================================================


//===========================================================================
//=============================private variables=============================
//===========================================================================

static uint8_t reply_header[PM_HEADER_SIZE];
static uint8_t reply_buf[MAX_RESPONSE_PARAM_LENGTH]; // must follow directly after reply_header

//===========================================================================
//=============================public variables=============================
//===========================================================================

bool reply_sent = false;
bool reply_started = false;
uint8_t reply_control_byte = 0xFF;
uint8_t reply_expected_length_excluding_msg = 0;
uint8_t reply_data_len = 0;
uint8_t reply_msg_len = 0;

//===========================================================================
//============================= ROUTINES =============================
//===========================================================================

void generate_response_start(uint8_t response_code, uint8_t expected_length_excluding_msg)
{ 
  reply_started = true;
  reply_header[PM_ORDER_BYTE_OFFSET] = response_code;
  reply_control_byte = (control_byte & CONTROL_BYTE_SEQUENCE_NUMBER_MASK);
  reply_header[PM_CONTROL_BYTE_OFFSET] = reply_control_byte;
  reply_expected_length_excluding_msg = expected_length_excluding_msg;
  reply_data_len = 0;
  reply_msg_len = 0;
  
  // TODO improve error checking of msg (and handling when expected_length_excluding_msg == 0xFF)
}

void generate_response_transport_error_start(uint8_t transport_error, uint8_t local_control_byte)
{
  reply_started = true;
  reply_header[PM_ORDER_BYTE_OFFSET] = RSP_FRAME_RECEIPT_ERROR;
  reply_header[PM_CONTROL_BYTE_OFFSET] = (local_control_byte & CONTROL_BYTE_SEQUENCE_NUMBER_MASK);
  reply_buf[0] = transport_error;
  reply_expected_length_excluding_msg = 1;
  reply_data_len = 1;
  reply_msg_len = 0;
}

void generate_response_data_addbyte(uint8_t value)
{
  reply_buf[reply_data_len++] = value;
}
void generate_response_data_add(uint8_t value)
{
  reply_buf[reply_data_len++] = value;
}
void generate_response_data_add(int8_t value)
{
  reply_buf[reply_data_len++] = value;
}
void generate_response_data_add(uint16_t value)
{
  reply_buf[reply_data_len++] = value >> 8;
  reply_buf[reply_data_len++] = value & 0xFF;
}
void generate_response_data_add(int16_t value)
{
  reply_buf[reply_data_len++] = value >> 8;
  reply_buf[reply_data_len++] = value & 0xFF;
}
void generate_response_data_add(uint32_t value)
{
  reply_buf[reply_data_len++] = value >> 24;
  reply_buf[reply_data_len++] = (value >> 16) & 0xFF;
  reply_buf[reply_data_len++] = (value >> 8) & 0xFF;
  reply_buf[reply_data_len++] = value & 0xFF;
}
void generate_response_data_add(int32_t value)
{
  reply_buf[reply_data_len++] = value >> 24;
  reply_buf[reply_data_len++] = (value >> 16) & 0xFF;
  reply_buf[reply_data_len++] = (value >> 8) & 0xFF;
  reply_buf[reply_data_len++] = value & 0xFF;
}
void generate_response_data_add(const uint8_t *value, uint8_t length)
{
  memcpy(&reply_buf[reply_data_len], value, length);
  reply_data_len += length;
}
void generate_response_data_add(const char *str)
{
  while (*str != 0 && reply_data_len < sizeof(reply_buf))
    reply_buf[reply_data_len++] = *str++;
}
void generate_response_data_addPGM(const char *str)
{
  char ch;
  while ((ch = pgm_read_byte(str++)) != '\0' && reply_data_len < sizeof(reply_buf))
    reply_buf[reply_data_len++] = ch;
}
void generate_response_data_clear()
{
  reply_data_len = 0;
}
uint8_t *generate_response_data_ptr()
{
  return &reply_buf[reply_data_len];
}
uint8_t generate_response_data_len()
{
  return sizeof(reply_buf) - reply_data_len;
}
void generate_response_data_addlen(uint8_t len)
{
  reply_data_len += len;
  // TODO add error on overflow
}

void generate_response_msg_addbyte(uint8_t value)
{
  if ((uint16_t)(reply_expected_length_excluding_msg + reply_msg_len) < sizeof(reply_buf))
    reply_buf[reply_expected_length_excluding_msg + reply_msg_len++] = value;
}
void generate_response_msg_add(uint8_t value)
{
  if ((uint16_t)(reply_expected_length_excluding_msg + reply_msg_len) < sizeof(reply_buf))
    reply_buf[reply_expected_length_excluding_msg + reply_msg_len++] = value;
}
void generate_response_msg_add(int8_t value)
{
  if ((uint16_t)(reply_expected_length_excluding_msg + reply_msg_len) < sizeof(reply_buf))
    reply_buf[reply_expected_length_excluding_msg + reply_msg_len++] = value;
}
void generate_response_msg_add(uint16_t value)
{
  if ((uint16_t)(reply_expected_length_excluding_msg + reply_msg_len) < sizeof(reply_buf)-1)
  {
    reply_buf[reply_expected_length_excluding_msg + reply_msg_len++] = value >> 8;
    reply_buf[reply_expected_length_excluding_msg + reply_msg_len++] = value & 0xFF;
  }
}
void generate_response_msg_add(int16_t value)
{
  if ((uint16_t)(reply_expected_length_excluding_msg + reply_msg_len) < sizeof(reply_buf)-1)
  {
    reply_buf[reply_expected_length_excluding_msg + reply_msg_len++] = value >> 8;
    reply_buf[reply_expected_length_excluding_msg + reply_msg_len++] = value & 0xFF;
  }
}
void generate_response_msg_add(uint32_t value)
{
  if ((uint16_t)(reply_expected_length_excluding_msg + reply_msg_len) < sizeof(reply_buf)-3)
  {
    reply_buf[reply_expected_length_excluding_msg + reply_msg_len++] = value >> 24;
    reply_buf[reply_expected_length_excluding_msg + reply_msg_len++] = (value >> 16) & 0xFF;
    reply_buf[reply_expected_length_excluding_msg + reply_msg_len++] = (value >> 8) & 0xFF;
    reply_buf[reply_expected_length_excluding_msg + reply_msg_len++] = value & 0xFF;
  }
}
void generate_response_msg_add(int32_t value)
{
  if ((uint16_t)(reply_expected_length_excluding_msg + reply_msg_len) < sizeof(reply_buf)-3)
  {
    reply_buf[reply_expected_length_excluding_msg + reply_msg_len++] = value >> 24;
    reply_buf[reply_expected_length_excluding_msg + reply_msg_len++] = (value >> 16) & 0xFF;
    reply_buf[reply_expected_length_excluding_msg + reply_msg_len++] = (value >> 8) & 0xFF;
    reply_buf[reply_expected_length_excluding_msg + reply_msg_len++] = value & 0xFF;
  }
}
void generate_response_msg_add(const uint8_t *value, uint8_t length)
{
  if ((uint16_t)(reply_expected_length_excluding_msg + reply_msg_len + length) <= sizeof(reply_buf))
  {
    uint8_t copy_length = min(length,sizeof(reply_buf)-(reply_expected_length_excluding_msg + reply_msg_len));
    memcpy(&reply_buf[reply_expected_length_excluding_msg + reply_msg_len], value, copy_length);
    reply_msg_len += copy_length;
  }
}
void generate_response_msg_add(const char *str)
{
  while (*str != 0 && reply_expected_length_excluding_msg + reply_msg_len < sizeof(reply_buf))
    reply_buf[reply_expected_length_excluding_msg + reply_msg_len++] = *str++;
}
void generate_response_msg_addPGM(const char *str)
{
  char ch;
  while ((ch = pgm_read_byte(str++)) != '\0' && reply_expected_length_excluding_msg + reply_msg_len < sizeof(reply_buf))
    reply_buf[reply_expected_length_excluding_msg + reply_msg_len++] = ch;
}
void generate_response_msg_clear()
{
  reply_msg_len = 0;
}
uint8_t *generate_response_msg_ptr()
{
  return &reply_buf[reply_expected_length_excluding_msg + reply_msg_len];
}
uint8_t generate_response_msg_len()
{
  return sizeof(reply_buf) - (reply_expected_length_excluding_msg + reply_msg_len);
}
void generate_response_msg_addlen(uint8_t len)
{
  reply_msg_len += len;
  // TODO add error on overflow
}

void generate_response_send()
{
  uint8_t param_length;
  uint8_t i;
  
  if (reply_msg_len == 0)
    param_length = reply_data_len;
  else
    param_length = reply_expected_length_excluding_msg + reply_msg_len;
  
  reply_header[PM_LENGTH_BYTE_OFFSET] = param_length;

  // TODO set event flag if required

#if TRACE_RESPONSE
  DEBUGPGM("\nResp(");  
  DEBUG_F(reply_header[PM_ORDER_BYTE_OFFSET], HEX);  
  DEBUGPGM(", len=");  
  DEBUG_F(param_length, DEC);  
  DEBUGPGM(", cb=");  
  DEBUG_F(control_byte, DEC);  
  DEBUGPGM(", rcb=");  
  DEBUG_F(reply_control_byte, DEC);  
  DEBUGPGM("):");  
  for (i = 0; i < param_length; i++)
  {
    DEBUG(' '); 
    DEBUG_F(reply_buf[i], HEX);  
  }
  DEBUG_EOL();
#endif  
  
  PSERIAL.write(SYNC_BYTE_RESPONSE_VALUE);
  for (i = 1; i < PM_HEADER_SIZE; i++)
    PSERIAL.write(reply_header[i]);
  for (i = 0; i < param_length; i++)
    PSERIAL.write(reply_buf[i]);
  PSERIAL.write(crc8(&reply_header[PM_ORDER_BYTE_OFFSET], 
                        (PM_HEADER_SIZE-PM_ORDER_BYTE_OFFSET)+param_length));
  reply_started = false;
  reply_sent = true;
}

//
// Convenience functions
//

void send_OK_response()
{
  generate_response_start(RSP_OK);
  generate_response_send();
}

void send_insufficient_bytes_error_response(uint8_t expected_num_bytes)
{
  send_insufficient_bytes_error_response(expected_num_bytes, parameter_length);
}

void send_insufficient_bytes_error_response(uint8_t expected_num_bytes, uint8_t actual_bytes)
{
  generate_response_start(RSP_APPLICATION_ERROR, 1);
  generate_response_data_addbyte(PARAM_APP_ERROR_TYPE_BAD_PARAMETER_FORMAT);
  generate_response_msg_addPGM(PMSG(ERR_MSG_INSUFFICENT_BYTES));
  generate_response_msg_add(actual_bytes); // TODO print as ascii
  generate_response_msg_add(", ");
  generate_response_msg_addPGM(PMSG(MSG_EXPECTING));
  generate_response_msg_addbyte(expected_num_bytes); // TODO print as ascii
  generate_response_send();
}

void send_app_error_at_offset_response(uint8_t error_type, uint8_t parameter_offset)
{
  generate_response_start(RSP_APPLICATION_ERROR, 1);
  generate_response_data_addbyte(error_type);
  generate_response_msg_addPGM(PMSG(ERR_MSG_GENERIC_APP_AT_OFFSET));
  generate_response_msg_add(parameter_offset); // TODO print as ascii
  generate_response_send();
}

void send_app_error_response(uint8_t error_type, const char *msg_pstr)
{
  generate_response_start(RSP_APPLICATION_ERROR, 1);
  generate_response_data_addbyte(error_type);
  if (msg_pstr != 0)
    generate_response_msg_addPGM(msg_pstr);
  generate_response_send();
}

void send_app_error_response(uint8_t error_type, const char *msg_pstr, uint16_t msg_num_value)
{
  generate_response_start(RSP_APPLICATION_ERROR, 1);
  generate_response_data_addbyte(error_type);
  if (msg_pstr != 0)
    generate_response_msg_addPGM(msg_pstr);
  generate_response_msg_add(msg_num_value);    
  generate_response_send();
}

void send_app_error_response(uint8_t error_type, const char *msg_pstr, const char *msg_str_value)
{
  generate_response_start(RSP_APPLICATION_ERROR, 1);
  generate_response_data_addbyte(error_type);
  if (msg_pstr != 0)
    generate_response_msg_addPGM(msg_pstr);
  generate_response_msg_add(msg_str_value);    
  generate_response_send();
}

void send_failed_response(const char* reason)
{
  generate_response_start(RSP_APPLICATION_ERROR, 0);
  generate_response_data_addbyte(PARAM_APP_ERROR_TYPE_FAILED);
  generate_response_msg_addPGM(PMSG(ERR_MSG_GENERIC_APP_AT_OFFSET));
  generate_response_send();
}

void send_stopped_response()
{
  if (is_stopped)
  {
    generate_response_start(RSP_STOPPED, 3);
    generate_response_data_addbyte(stopped_is_acknowledged);
    generate_response_data_addbyte(stopped_type);
    generate_response_data_addbyte(stopped_cause);
    // might want to add some argument information but this will do for now
    if (stopped_reason != 0)
      generate_response_msg_addPGM(stopped_reason);
    generate_response_send();
  }
  else
  {
    send_OK_response();
  }
}

