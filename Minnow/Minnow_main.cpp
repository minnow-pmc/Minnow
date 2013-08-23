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

/*
 This firmware is a PaceMaker protocol implementation based on Marlin
 temperature and movement control code. (https://github.com/ErikZalm/Marlin)
 
 Github location: https://github.com/minnow-3dp/Minnow
 
 */

#include "Minnow.h"

#include "config.h"
#include "order_handlers.h"
#include "protocol.h"
#include "response.h"
#include "crc8.h"
#include "firmware_configuration.h"

#include "Device_InputSwitch.h"
#include "Device_OutputSwitch.h"
#include "Device_PwmOutput.h"
#include "Device_Heater.h"
#include "Device_Buzzer.h"

#include "CommandQueue.h"

#include <avr/pgmspace.h>
#ifdef USE_WATCHDOG_TO_RESET
  #include <avr/wdt.h>
#endif

//===========================================================================
//=============================imported variables============================
//===========================================================================

extern bool reply_sent;
extern bool reply_started;
extern uint8_t reply_control_byte;


//===========================================================================
//=============================private variables=============================
//===========================================================================

static uint8_t recv_buf_len = 0;
static uint8_t recv_buf[MAX_RECV_BUF_LEN];
static PROGMEM uint32_t autodetect_baudrates[] = AUTODETECT_BAUDRATES;
static uint8_t autodetect_baudrates_index = 0;

//===========================================================================
//=============================public variables=============================
//===========================================================================

uint32_t recv_count = 0;
uint16_t recv_errors = 0;

uint8_t order_code;
uint8_t control_byte;
uint8_t parameter_length;
uint8_t * const parameter_value = &recv_buf[PM_PARAMETER_OFFSET];

#if DEBUG_NOT_STOPPED_INITIALLY
  bool is_stopped = false; // doesn't require a resume after reset can make testing easier
#else
  bool is_stopped = true; // normally, the system is initially stopped with 'reset' cause
#endif  
bool stopped_is_acknowledged = false; 
uint8_t stopped_type = PARAM_STOPPED_TYPE_ONE_TIME_OR_CLEARED; 
uint8_t stopped_cause = PARAM_STOPPED_CAUSE_RESET; 
const char *stopped_reason = 0;

uint8_t reset_cause = 0; // cache of MCUSR register written by bootloader

uint16_t last_rcvd_time; // time a byte was last received


//===========================================================================
//=============================ROUTINES=============================
//===========================================================================

extern "C"{
  extern unsigned int __bss_end;
  extern unsigned int __heap_start;
  extern void *__brkval;

  uint16_t freeMemory() {
    int free_memory;

    if((int)__brkval == 0)
      free_memory = ((int)&free_memory) - ((int)&__bss_end);
    else
      free_memory = ((int)&free_memory) - ((int)__brkval);

    return free_memory;
  }

  uint8_t *startOfCommandQueueBuffer() {
    if((int)__brkval == 0)
      return ((uint8_t *)&__bss_end) + 1;
    else
      return ((uint8_t *)__brkval) + 1;
  }
}

uint8_t *startOfStack() 
{
  return startOfCommandQueueBuffer() + CommandQueue::GetQueueBufferLength();
}

// This function fills the unused stack with a known pattern after the 
// message queue. This later allows countStackLowWatermark() to later 
// report the stack low water mark.
void writeStackLowWaterMarkPattern() 
{
  uint8_t you_are_here;
  memset(startOfStack(), 0xc3, &you_are_here - startOfStack() - 16);
}

uint16_t countStackLowWatermark() 
{
  uint8_t *ptr = startOfStack();
  uint16_t cnt = 0;
  while (*ptr++ == 0xc3)
    cnt += 1;
  return cnt;  
}
  

void setup()
{
  reset_cause = MCUSR;   // only works if bootloader doesn' t set MCUSR to 0
  MCUSR=0;

#ifdef USE_WATCHDOG_TO_RESET
  wdt_disable();   // turn off watchdog 
#endif
  
  // allow command queue to use all remaining RAM
  CommandQueue::Init(startOfCommandQueueBuffer(), freeMemory() - STACK_SIZE);

  writeStackLowWaterMarkPattern();

  // initialize PaceMaker serial port to first value
  PSERIAL.begin(pgm_read_dword(&autodetect_baudrates[0]));
  if (sizeof(autodetect_baudrates)/sizeof(autodetect_baudrates[0]) < 2)
    autodetect_baudrates_index = 0xFF; // no need to autodetect

#if DEBUG_ENABLED  
  DSerial.begin();
#endif  

  Device_InputSwitch::Init();
  Device_OutputSwitch::Init();
  Device_PwmOutput::Init();
  Device_Heater::Init();
  Device_Buzzer::Init();

}

static FORCE_INLINE bool get_command()
{
  char serial_char;
  
  if (millis() - last_rcvd_time > MAX_FRAME_COMPLETION_DELAY_MS)
  {
    // timeout waiting for frame completion
    if (recv_buf_len >= PM_HEADER_SIZE)
    {    
      generate_response_start(RSP_FRAME_RECEIPT_ERROR);
      generate_response_data_addbyte(PARAM_FRAME_RECEIPT_ERROR_TYPE_BAD_FRAME);
      generate_response_send();
      recv_errors += 1;
    }
    recv_buf_len = 0;
  }
  
  while(PSERIAL.available() > 0 && recv_buf_len < sizeof(recv_buf)) 
  {
    serial_char = PSERIAL.read();

    // ignore non-sync characters at the start
    if (recv_buf_len == 0 && serial_char != SYNC_BYTE_ORDER_VALUE)
        continue;
  
    // still need more bytes?
    if (recv_buf_len < PM_HEADER_SIZE - 1
        || recv_buf_len - PM_HEADER_SIZE < recv_buf[PM_LENGTH_BYTE_OFFSET])
    {
      recv_buf[recv_buf_len++] = serial_char;
      continue;
    }

    // we have enough bytes - check the crc
    if (serial_char != crc8(&recv_buf[PM_ORDER_CODE_OFFSET],recv_buf_len-2))
    {
#if !DEBUG_DONT_CHECK_CRC8_VALUE    
      generate_response_start(RSP_FRAME_RECEIPT_ERROR);
      generate_response_data_addbyte(PARAM_FRAME_RECEIPT_ERROR_TYPE_BAD_CHECK_CODE);
      generate_response_send();
      recv_buf_len = 0;
#endif 
      recv_errors += 1;
    }
          
    recv_buf[recv_buf_len] = '\0'; // null-terminate buffer
    recv_count += 1;
    return true;
  }

  if (recv_buf_len == sizeof(recv_buf)) // should only occur due to reduced buffer size
  {
    generate_response_start(RSP_FRAME_RECEIPT_ERROR);
    generate_response_data_addbyte(PARAM_FRAME_RECEIPT_ERROR_TYPE_BAD_FRAME);
    generate_response_send();
    recv_buf_len = 0;
  }
  
  last_rcvd_time = millis();
  return false;
}

void loop()
{
  // quickly scan through baudrates until we receive a valid packet within timeout packet period
  if (autodetect_baudrates_index != 0xFF)
  {
    if (millis() - last_rcvd_time > MAX_FRAME_COMPLETION_DELAY_MS * 1.5)
    {
      autodetect_baudrates_index += 1;
      if (autodetect_baudrates_index >= sizeof(autodetect_baudrates)/sizeof(autodetect_baudrates[0]))
        autodetect_baudrates_index = 0;
    }
    PSERIAL.begin(autodetect_baudrates[autodetect_baudrates_index]);
  }

  if (get_command())
  {
    autodetect_baudrates_index = 0xFF;
    order_code = recv_buf[PM_ORDER_CODE_OFFSET];
    control_byte = recv_buf[PM_CONTROL_BYTE_OFFSET];
    parameter_length = recv_buf[PM_LENGTH_BYTE_OFFSET];
  
    // does this match the sequence number of the last reply 
    if ((control_byte & CONTROL_BYTE_SEQUENCE_NUMBER_MASK) == 
            (reply_control_byte & CONTROL_BYTE_SEQUENCE_NUMBER_MASK) 
        && (control_byte & CONTROL_BYTE_ORDER_HOST_RESET_BIT) == 0)
    {
      if (!reply_started && reply_control_byte != 0xFF)
      { 
        // resend last response
        reply_started = true;
        generate_response_send();
      }
      else
      {
        // this is an unexpected error case (matching sequence number but nothing to send)
        generate_response_start(RSP_FRAME_RECEIPT_ERROR);
        generate_response_data_addbyte(PARAM_FRAME_RECEIPT_ERROR_TYPE_UNABLE_TO_ACCEPT);
        generate_response_msg_add(PSTR(MSG_ERR_NO_RESPONSE_TO_SEND));
        generate_response_send();
      }
      return;
    }
  
    reply_sent = false;
     
    process_command();
    
    if (!reply_sent)
    {
      generate_response_start(RSP_APPLICATION_ERROR, 1);
      generate_response_data_addbyte(PARAM_APP_ERROR_TYPE_FIRMWARE_ERROR);
      generate_response_msg_addPGM(PSTR(MSG_ERR_NO_RESPONSE_GENERATED));
      generate_response_send();
    }
    
    recv_buf_len = 0;
  }
  // TODO add idle loop stuff
}

void emergency_stop()
{
  // TODO
}

void die()
{
  cli(); // Stop interrupts
  // Unfortunately we have no way to do a hardware reset!
  //
  // Normally you'd be able to reset by enabling the watchdog but the
  // standard Arduino bootloader is retarded and doesn't disable the 
  // watchdog when it starts and indeed reduces the timeout to 15ms 
  // which causes the AVR to go into a reset loop.
  //
  // Some bootloaders do something smarter (in which case you can enable
  // the USE_WATCHDOG_TO_RESET flag) but we can't set this as default.
#ifdef USE_WATCHDOG_TO_RESET
  wdt_enable(WDTO_1S);
#endif  
  while (true) { } // wait for manual reset (or watchdog if enabled)
}

