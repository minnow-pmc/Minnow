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
 
 Github location: https://github.com/minnow-pmc/Minnow
 
 */

#include "Minnow.h"

#include "config.h"
#include "order_handlers.h"
#include "protocol.h"
#include "response.h"
#include "crc8.h"
#include "firmware_configuration.h"

#include "movement_ISR.h"
#include "temperature_ISR.h"

#include "Device_TemperatureSensor.h"
#include "Device_Heater.h"
#include "Device_Stepper.h"
#include "Device_OutputSwitch.h"
#include "Device_PwmOutput.h"
#include "CommandQueue.h"
#include "NVConfigStore.h"

#include <avr/pgmspace.h>
#if USE_WATCHDOG_FOR_RESET
  #include <avr/wdt.h>
#endif

//===========================================================================
//=============================imported variables============================
//===========================================================================

extern bool reply_sent;
extern bool reply_started;
extern uint8_t reply_control_byte;
extern volatile bool temp_meas_ready;


//===========================================================================
//=============================private variables=============================
//===========================================================================

static uint16_t recv_buf_len = 0;
static uint8_t recv_buf[MAX_RECV_BUF_LEN];
static PROGMEM const uint32_t autodetect_baudrates[] = AUTODETECT_BAUDRATES;
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

uint32_t first_rcvd_time; // time a byte was first received in the frame

extern "C" {
  extern unsigned int __bss_end;
  extern unsigned int __heap_start;
  extern size_t __malloc_margin;
  extern void *__brkval;
}

//===========================================================================
//=============================ROUTINES=============================
//===========================================================================

uint16_t freeMemory() 
{
  int free_memory;

  if((int)__brkval == 0)
    free_memory = ((int)&free_memory) - ((int)&__bss_end);
  else
    free_memory = ((int)&free_memory) - ((int)__brkval);

  return free_memory;
}

uint8_t *startOfStack() 
{
  if((int)__brkval == 0)
    return (uint8_t *)&__bss_end;
  else
    return (uint8_t *)__brkval;
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

//
// The queue is allocated the first time that it is flushed or something
// is enqueued. Normally this is done after all other memory allocations have
// occurred as the queue will take up all remaining RAM above configured thresholds.
//
bool allocate_command_queue_memory()
{
  uint16_t memory_size = 0; 
  uint8_t *memory; 
  
  if (freeMemory() > MAX_STACK_SIZE)
  {
    memory_size = max(freeMemory() - MAX_STACK_SIZE, MIN_QUEUE_SIZE);
  }
  else if (freeMemory() >= MIN_STACK_SIZE + MIN_QUEUE_SIZE)
  {
    memory_size = max(freeMemory() - (MIN_STACK_SIZE * 1.3), MIN_QUEUE_SIZE);
  }
  
  if (memory_size == 0 || (memory = (uint8_t *)malloc(memory_size)) == 0)
  {
    return false;
  }
  CommandQueue::Init(memory, memory_size);
  return true;
}

void applyDebugConfiguration()
{
#if APPLY_DEBUG_CONFIGURATION

  DEBUG_COMMAND_ARRAY("Clear stopped", ORDER_RESUME, ARRAY({ PARAM_RESUME_TYPE_CLEAR }));

  DEBUG_COMMAND_ARRAY("Request Board Name", ORDER_REQUEST_INFORMATION, ARRAY({ PARAM_REQUEST_INFO_BOARD_NAME }) ); 
  DEBUG_COMMAND_ARRAY("Request Hardware Type", ORDER_REQUEST_INFORMATION, ARRAY({ PARAM_REQUEST_INFO_HARDWARE_TYPE }) ); 

  DEBUG_WRITE_FIRMWARE_CONFIGURATION("system.num_digital_inputs", "3");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.digital_input.0.pin", "3");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.digital_input.1.pin", "14");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.digital_input.2.pin", "18");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.digital_input.2.name", "ZMIN");

  DEBUG_COMMAND_ARRAY("Get Input Switch Name 0", ORDER_DEVICE_NAME, ARRAY({ PM_DEVICE_TYPE_SWITCH_INPUT, 0 }) );
  DEBUG_COMMAND_ARRAY("Get Input Switch Name 1", ORDER_DEVICE_NAME, ARRAY({ PM_DEVICE_TYPE_SWITCH_INPUT, 1 }) );
  DEBUG_COMMAND_ARRAY("Get Input Switch Name 2", ORDER_DEVICE_NAME, ARRAY({ PM_DEVICE_TYPE_SWITCH_INPUT, 2 }) );

  DEBUG_READ_FIRMWARE_CONFIGURATION("devices.digital_input.0.name");
  DEBUG_READ_FIRMWARE_CONFIGURATION("devices.digital_input.1.name");

  DEBUG_WRITE_FIRMWARE_CONFIGURATION("system.num_temp_sensors", "1");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.temp_sensor.0.pin", "13");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.temp_sensor.0.type", "7");

  DEBUG_WRITE_FIRMWARE_CONFIGURATION("system.num_digital_outputs", "3");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.digital_output.0.pin", "5");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.digital_output.1.pin", "6");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.digital_output.2.pin", "11");
  
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("system.num_pwm_outputs", "1");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.pwm_output.0.pin", "4");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.pwm_output.0.use_soft_pwm", "0");
  
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("system.num_buzzers", "1");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.buzzer.0.pin", "33");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.buzzer.0.use_soft_pwm", "1");
  
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("system.num_temp_sensors", "1");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.temp_sensor.0.pin", "13");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.temp_sensor.0.type", "7");
//  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.temp_sensor.1.pin", "15");
//  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.temp_sensor.1.type", "7");
//  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.temp_sensor.2.pin", "9");
//  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.temp_sensor.2.type", "1");

  DEBUG_WRITE_FIRMWARE_CONFIGURATION("system.num_steppers", "4");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.stepper.0.enable_pin", "38");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.stepper.0.direction_pin", "55");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.stepper.0.direction_invert", "1");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.stepper.0.step_pin", "54");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.stepper.0.enable_invert", "1");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.stepper.1.enable_pin", "56");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.stepper.1.direction_pin", "61");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.stepper.1.direction_invert", "0");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.stepper.1.step_pin", "60");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.stepper.1.enable_invert", "1");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.stepper.2.enable_pin", "62");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.stepper.2.direction_pin", "48");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.stepper.2.direction_invert", "1");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.stepper.2.step_pin", "46");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.stepper.2.enable_invert", "1");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.stepper.3.enable_pin", "24");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.stepper.3.direction_pin", "28");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.stepper.3.step_pin", "26");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.stepper.3.enable_invert", "1");

  DEBUG_WRITE_FIRMWARE_CONFIGURATION("system.num_heaters", "1");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.heater.0.pin", "10");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.heater.0.max_temp", "2500");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.heater.0.use_bang_bang", "1");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.heater.0.power_on_level", "255");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.heater.0.bang_bang_hysteresis", "1");
  DEBUG_WRITE_FIRMWARE_CONFIGURATION("devices.heater.0.temp_sensor", "0");
  
  DEBUG_READ_FIRMWARE_CONFIGURATION("debug.stack_memory");
  DEBUG_READ_FIRMWARE_CONFIGURATION("debug.stack_low_water_mark");
  DEBUG_READ_FIRMWARE_CONFIGURATION("stats.queue_memory");

  DEBUG_COMMAND_ARRAY("Request Num Input Switches", ORDER_REQUEST_INFORMATION, ARRAY({ PARAM_REQUEST_INFO_NUM_SWITCH_INPUTS }) );
  DEBUG_COMMAND_ARRAY("Request Input Switch 0", ORDER_GET_INPUT_SWITCH_STATE, ARRAY({ PM_DEVICE_TYPE_SWITCH_INPUT, 0 }) );
  DEBUG_COMMAND_ARRAY("Request Input Switch 1", ORDER_GET_INPUT_SWITCH_STATE, ARRAY({ PM_DEVICE_TYPE_SWITCH_INPUT, 1 }) );
  DEBUG_COMMAND_ARRAY("Request Input Switch 2", ORDER_GET_INPUT_SWITCH_STATE, ARRAY({ PM_DEVICE_TYPE_SWITCH_INPUT, 2 }) );
  
  DEBUG_COMMAND_STR("Request Num Output Switches", ORDER_REQUEST_INFORMATION, "\x11" );
  DEBUG_COMMAND_ARRAY("Set Output Switch 0 (P5)", ORDER_SET_OUTPUT_SWITCH_STATE, ARRAY({ PM_DEVICE_TYPE_SWITCH_OUTPUT, 0, 0 }) );
//  DEBUG_COMMAND_ARRAY("Set Output Switch 1 (P6)", ORDER_SET_OUTPUT_SWITCH_STATE, ARRAY({ PM_DEVICE_TYPE_SWITCH_OUTPUT, 1, 1 }) );

  DEBUG_COMMAND_ARRAY("Request Num Pwm Outputs", ORDER_REQUEST_INFORMATION, ARRAY({ PARAM_REQUEST_INFO_NUM_PWM_OUTPUTS }) ); 
//  DEBUG_COMMAND_ARRAY("Set Pwm Output 0", ORDER_SET_PWM_OUTPUT_STATE, ARRAY({ PM_DEVICE_TYPE_PWM_OUTPUT, 0, 64, 0 }) );
   
  DEBUG_COMMAND_ARRAY("Request Num Buzzers", ORDER_REQUEST_INFORMATION, ARRAY({ PARAM_REQUEST_INFO_NUM_BUZZERS }) );
//  DEBUG_COMMAND_ARRAY("Set Buzzer Output 0", ORDER_SET_PWM_OUTPUT_STATE, ARRAY({ PM_DEVICE_TYPE_BUZZER, 0, 128, 0 }) );
  
  DEBUG_COMMAND_ARRAY("Request Num Temp Sensors", ORDER_REQUEST_INFORMATION, ARRAY({ PARAM_REQUEST_INFO_NUM_TEMP_SENSORS }) );

  DEBUG_COMMAND_ARRAY("Configure Endstops 0", ORDER_CONFIGURE_ENDSTOPS, ARRAY({ 0, 0, 0, 0  }) );
  DEBUG_COMMAND_ARRAY("Configure Endstops 1", ORDER_CONFIGURE_ENDSTOPS, ARRAY({ 1, 1, 0, 0, 2, 0, 0 }) );

  DEBUG_COMMAND_ARRAY("Configure Axis Rates", ORDER_CONFIGURE_AXIS_MOVEMENT_RATES, ARRAY({ (28000/255), (28000%255), \
      (28000/255), (28000%255), (11200/255), (11200%255), (56700/255), (56700%255) }) );

  DEBUG_COMMAND_ARRAY("Activate Stepper Control", ORDER_ACTIVATE_STEPPER_CONTROL, ARRAY({ 1 }) );

  DEBUG_COMMAND_ARRAY("Set Stepper Enable (Empty)", ORDER_ENABLE_DISABLE_STEPPERS, ARRAY({  }) );

//  DEBUG_COMMAND_ARRAY("Set Stepper Enable", ORDER_ENABLE_DISABLE_STEPPERS, ARRAY({ 0, 1, 1, 0 }) );
  
  DEBUG_COMMAND_STR("Flush Command Queue", ORDER_CLEAR_COMMAND_QUEUE, "" );
  
  DEBUG_COMMAND_STR("Read Queue Length", ORDER_READ_FIRMWARE_CONFIG_VALUE, "stats.queue_memory" );
  DEBUG_COMMAND_STR("Read Stack Length", ORDER_READ_FIRMWARE_CONFIG_VALUE, "debug.stack_memory" );
  
//  while (!temp_meas_ready)  
//    ;
  Device_TemperatureSensor::UpdateTemperatureSensors();
//  while (!temp_meas_ready)  
//    ;
  Device_TemperatureSensor::UpdateTemperatureSensors();

  DEBUG_COMMAND_ARRAY("Read Temp Sensor 0", ORDER_REQUEST_TEMPERATURE_READING, ARRAY({ PM_DEVICE_TYPE_TEMP_SENSOR, 0 }) );
  //DEBUG_COMMAND_ARRAY("Read Temp Sensor 1", ORDER_REQUEST_TEMPERATURE_READING, ARRAY({ PM_DEVICE_TYPE_TEMP_SENSOR, 1 }) );
  //DEBUG_COMMAND_ARRAY("Read Temp Sensor 2", ORDER_REQUEST_TEMPERATURE_READING, ARRAY({ PM_DEVICE_TYPE_TEMP_SENSOR, 2 }) );

#endif
}

void setup()
{
  reset_cause = MCUSR;   // only works if bootloader doesn' t set MCUSR to 0
  MCUSR=0;

#if USE_WATCHDOG_FOR_RESET
  wdt_disable();   // turn off watchdog 
#endif
  
  __malloc_margin = MIN_STACK_SIZE;
  
  writeStackLowWaterMarkPattern();

  // initialize PaceMaker serial port to first value
  PSERIAL.begin(pgm_read_dword(&autodetect_baudrates[0]));
  if (sizeof(autodetect_baudrates)/sizeof(autodetect_baudrates[0])  )
    autodetect_baudrates_index = 0xFF; // no need to autodetect

#if DEBUG_ENABLED  
  DSerial.begin();
#endif  

  NVConfigStore::Initialize();
  movement_ISR_init();
  temperature_ISR_init();

  applyDebugConfiguration();
}

FORCE_INLINE static bool get_command()
{
  char serial_char;
  
  if (recv_buf_len > 0 && millis() - first_rcvd_time > MAX_FRAME_COMPLETION_DELAY_MS)
  {
    // timeout waiting for frame completion
    if (recv_buf_len >= PM_HEADER_SIZE)
    {    
      generate_response_transport_error_start(PARAM_FRAME_RECEIPT_ERROR_TYPE_BAD_FRAME, 
          recv_buf[PM_CONTROL_BYTE_OFFSET]);
      generate_response_send();
      recv_errors += 1;
    }
    recv_buf_len = 0;
  }

  while(PSERIAL.available() > 0) 
  {
    serial_char = PSERIAL.read();

    if (recv_buf_len == 0)
    {
      // ignore non-sync characters at the start
      if (serial_char == SYNC_BYTE_ORDER_VALUE)
      {
        first_rcvd_time = millis();
        recv_buf[0] = SYNC_BYTE_ORDER_VALUE;
        recv_buf_len = 1;
      }
      continue;
    }

    // still need more bytes?
    else if (recv_buf_len < PM_HEADER_SIZE
        || recv_buf_len - PM_HEADER_SIZE < recv_buf[PM_LENGTH_BYTE_OFFSET])
    {
      if (recv_buf_len == sizeof(recv_buf)) // should only occur due to reduced buffer size
      {
        generate_response_transport_error_start(PARAM_FRAME_RECEIPT_ERROR_TYPE_BAD_FRAME, 
            recv_buf[PM_CONTROL_BYTE_OFFSET]);
        generate_response_send();
        recv_buf_len = 0;
        return false;
      }
      recv_buf[recv_buf_len++] = serial_char;
      continue;
    }

    // we have enough bytes - check the crc
    else if (serial_char != crc8(&recv_buf[PM_ORDER_CODE_OFFSET],recv_buf_len-1))
    {
#if !DEBUG_DONT_CHECK_CRC8_VALUE 
      generate_response_transport_error_start(PARAM_FRAME_RECEIPT_ERROR_TYPE_BAD_CHECK_CODE, 
          recv_buf[PM_CONTROL_BYTE_OFFSET]);
      generate_response_send();
      recv_buf_len = 0;
      recv_errors += 1;
      continue;
#endif      
    }
    
    // we have a packet  
    recv_buf[recv_buf_len] = '\0'; // null-terminate buffer
    recv_count += 1;
    return true;
  }
  return false;
}

void loop()
{
  // quickly scan through baudrates until we receive a valid packet within timeout period
#if 0 // the baud rate change still isn't working for some reason.
  if (autodetect_baudrates_index != 0xFF)
  {
    if (millis() - first_rcvd_time > MAX_FRAME_COMPLETION_DELAY_MS * 1.5) // make sure its not a multiple of 100ms
    {
      PSERIAL.end();
      PSERIAL.flush();
      autodetect_baudrates_index += 1;
      if (autodetect_baudrates_index >= NUM_ARRAY_ELEMENTS(autodetect_baudrates))
        autodetect_baudrates_index = 0;
      PSERIAL.begin(pgm_read_dword(&autodetect_baudrates[autodetect_baudrates_index]));
      recv_buf_len = 0;
      first_rcvd_time = millis();
    }
  }
#endif
  
  if (get_command())
  {
    autodetect_baudrates_index = 0xFF;
    order_code = recv_buf[PM_ORDER_CODE_OFFSET];
    control_byte = recv_buf[PM_CONTROL_BYTE_OFFSET];
    parameter_length = recv_buf[PM_LENGTH_BYTE_OFFSET];

#if TRACE_ORDER
    DEBUGPGM("\nOrder(");  
    DEBUG_F(order_code, HEX);  
    DEBUGPGM(", len=");  
    DEBUG_F(parameter_length, DEC);  
    DEBUGPGM(", cb=");  
    DEBUG_F(control_byte, HEX);  
    DEBUGPGM("):");  
    for (uint8_t i = 0; i < parameter_length; i++)
    {
      DEBUG(' '); 
      DEBUG_F(recv_buf[i], HEX);  
    }
    DEBUG_EOL();
#endif    
    
    // does this match the sequence number of the last reply 
    if ((control_byte & CONTROL_BYTE_SEQUENCE_NUMBER_MASK) == 
            (reply_control_byte & CONTROL_BYTE_SEQUENCE_NUMBER_MASK) 
        && (control_byte & CONTROL_BYTE_ORDER_HOST_RESET_BIT) == 0
        && reply_control_byte != 0xFF)
    {
      if (!reply_started)
      { 
        // resend last response
        reply_started = true;
        generate_response_send();
      }
      else
      {
        // this is an unexpected error case (matching sequence number but nothing to send)
        generate_response_transport_error_start(PARAM_FRAME_RECEIPT_ERROR_TYPE_UNABLE_TO_ACCEPT, 
            control_byte);
        generate_response_msg_addPGM(PMSG(MSG_ERR_NO_RESPONSE_TO_SEND));
        generate_response_send();
      }
    }
    else
    {
      reply_sent = false;
       
      process_command();
      
      if (!reply_sent)
      {
        send_app_error_response(PARAM_APP_ERROR_TYPE_FIRMWARE_ERROR,
             PMSG(MSG_ERR_NO_RESPONSE_GENERATED));
      }
    }
    recv_buf_len = 0;
  }

  // Idle loop activities
  if (temp_meas_ready)
  {
    Device_TemperatureSensor::UpdateTemperatureSensors();
    Device_Heater::UpdateHeaters();
  }
}

void emergency_stop()
{
  uint8_t i;
  is_stopped = true; // this should be set by caller but just to be sure
                     // this also causes the movement_ISR() to stop and flush queue
  stopped_is_acknowledged = false;
  // caller should have setup what the cause is - but in case they haven't/
  if (stopped_cause == PARAM_STOPPED_CAUSE_RESET || stopped_cause == 0xFF)
  {
    stopped_cause = PARAM_STOPPED_CAUSE_FIRMWARE_ERROR;
    stopped_type = PARAM_STOPPED_TYPE_ONE_TIME_OR_CLEARED;
  }
  for (i=0; i<Device_Stepper::GetNumDevices(); i++)
  {
    Device_Stepper::WriteEnableState(i, false);
  }
  for (i=0; i<Device_Heater::GetNumDevices(); i++)
  {
    if (Device_Heater::IsInUse(i))
    {
      Device_Heater::SetTargetTemperature(i, 0);
      digitalWrite(Device_Heater::GetHeaterPin(i), LOW); // forces device to turn off immediately even with soft PWM.
    }
  }    
  for (i=0; i<Device_OutputSwitch::GetNumDevices(); i++)
  {
    Device_OutputSwitch::WriteState(i, OUTPUT_SWITCH_STATE_DISABLED);
  }    
  for (i=0; i<Device_PwmOutput::GetNumDevices(); i++)
  {
    if (Device_PwmOutput::IsInUse(i))
    {
      Device_PwmOutput::WriteState(i, 0);
      digitalWrite(Device_PwmOutput::GetPin(i), LOW); // forces device to turn off immediately even with soft PWM.
    }
  }    
}

void die()
{
  cli(); // Stop interrupts
  // Unfortunately we have no reliable way to do a hardware reset!
  //
  // Normally you'd be able to reset by enabling the watchdog but the
  // standard Arduino bootloader is retarded and doesn't disable the 
  // watchdog when it starts and indeed reduces the timeout to 15ms 
  // which causes the AVR to go into a reset loop.
  //
  // Some newer bootloaders do something smarter (in which case you can 
  // enable the USE_WATCHDOG_TO_RESET flag) but we can't set this as
  // default.
#if USE_WATCHDOG_FOR_RESET
  wdt_enable(WDTO_1S);
  while (true) { } // wait for manual reset (or watchdog if enabled)
#else
  // just do a software only reset
  void(* resetFunc) (void) = 0; //declare reset function @ address 0
  resetFunc();  //call reset
#endif  
}

