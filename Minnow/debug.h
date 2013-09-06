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

#ifndef DEBUG_H
#define DEBUG_H

#include "Minnow.h"
#include "protocol.h"

//
// TODO More work is needed on handling of debug and errors!!! Not finished. 
//

#define QUEUE_DEBUG 1
#define QUEUE_TEST 0


//
// Switches to make development testing easier
//
#define DEBUG_DONT_CHECK_CRC8_VALUE   1
#define DEBUG_NOT_STOPPED_INITIALLY   1

#define TRACE_RESPONSE 1

///////////////////////////////
// Debug tracing config
///////////////////////////////

//
// How should debug messages be sent out?
//
#define USE_PACEMAKER_FRAMES_FOR_DEBUG   1    // sends debug strings as unsolicited Pacemaker frames 
//#define USE_SERIAL_PORT_FOR_DEBUG        1    // sends debug strings out another serial UART
//#define USE_EVENTS_FOR_DEBUG             1    // sends debug strings as delayed events  

//
// Method specific config
//

#if USE_PACEMAKER_FRAMES_FOR_DEBUG
  #define DEBUG_BUFFER_SIZE             128
#endif  

#if USE_SERIAL_PORT_FOR_DEBUG
  // Otherwise we use a separate serial port
  #define DEBUG_SERIAL_PORT Serial1
  #define DEBUG_SERIAL_PORT_BAUDRATE 115200
#endif  

#define DEBUG_ENABLED (USE_PACEMAKER_FRAMES_FOR_DEBUG || USE_SERIAL_PORT_FOR_DEBUG || USE_EVENTS_FOR_DEBUG)

// 
// Interfaces and Implementation
//

// TODO: treat these differently once event framework is implemented
#define ERROR(x) DSerial.print(true, x)
#define ERROR_F(x,y) DSerial.print(true, x,y)
#define ERRORPGM(x) ERRORPGM_P(PSTR(x))
#define ERRORPGM_P(x) DSerial.printPGM(true, x)
#define ERROR_EOL() DSerial.println(true)
#define ERRORLN(x) DSerial.println(true, x)
#define ERRORLNPGM(x) ERRORLNPGM_P(PSTR(x))
#define ERRORLNPGM_P(x) do{DSerial.printPGM(true, x);DSerial.println(true);}while(0)
#define ERRORPGM_ECHOPAIR(name,value) DSerial.printPGM_pair(true, PSTR(name),(value))


#if !DEBUG_ENABLED

#define DEBUG(x) 
#define DEBUG_EOL() 
#define DEBUG_F(x,y) 
#define DEBUGPGM(x) 
#define DEBUGPGM_P(x) 
#define DEBUGLN(x) 
#define DEBUGLNPGM(x) 
#define DEBUGLNPGM_P(x) 
#define DEBUGPGM_ECHOPAIR(name,value) 

#else

class DebugSerial;
extern DebugSerial DSerial;
  
//
// Debug macros
//
#define DEBUG(x) DSerial.print(false, x)
#define DEBUG_F(x,y) DSerial.print(false, x, y)
#define DEBUGPGM(x) DEBUGPGM_P(PSTR(x))
#define DEBUGPGM_P(x) DSerial.printPGM(false, x)
#define DEBUG_EOL() DSerial.println(false)
#define DEBUGLN(x) DSerial.println(false, x)
#define DEBUGLNPGM(x) DEBUGLNPGM_P(PSTR(x))
#define DEBUGLNPGM_P(x) do{DSerial.printPGM(false, x);DSerial.println(false);}while(0)
#define DEBUGPGM_ECHOPAIR(name,value) DSerial.printPGM_pair(false, PSTR(name),(value))

//
// Debug implementation class
//
class DebugSerial
{
  public:
    DebugSerial();

#ifndef DEC    
#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2
#define BYTE 0
#endif
    
    // Low level functions
    void begin();
    void flush(void);
    
    FORCE_INLINE void write(bool error, uint8_t c)
    {
#if USE_PACEMAKER_FRAMES_FOR_DEBUG
      debug_buf[debug_buf_len++] = c;
      if (debug_buf_len == sizeof(debug_buf) || c == '\n')
        flush();
#elif USE_SERIAL_PORT_FOR_DEBUG
      DEBUG_SERIAL_PORT.write(c);
#elif USE_EVENTS_FOR_DEBUG
      #error Waiting for events framework implementation
#endif  
    }   

    //
    // Convenience functions
    //

    void print(bool error, long n, int base);
    
    FORCE_INLINE void write(bool error, const char *str)
    {
      while (*str)
        write(error, *str++);
    }
    
    FORCE_INLINE void write(bool error, const uint8_t *buffer, size_t size)
    {
      while (size--)
        write(error, *buffer++);
    }

    FORCE_INLINE void print(bool error, const String &s)
    {
      for (int i = 0; i < (int)s.length(); i++) {
        write(error, s[i]);
      }
    }
    
    FORCE_INLINE void print(bool error, const char *str)
    {
      write(error, str);
    }
    
    FORCE_INLINE void print(bool error, char c, int base = BYTE)
    {
      print(error, (long) c, base);
    }

    FORCE_INLINE void print(bool error, unsigned char b, int base = BYTE)
    {
      print(error, (unsigned long) b, base);
    }

    FORCE_INLINE void print(bool error, int n, int base = DEC)
    {
      print(error, (long) n, base);
    }

    FORCE_INLINE void print(bool error, unsigned int n, int base = DEC)
    {
      print(error, (unsigned long) n, base);
    }

    FORCE_INLINE void print(bool error, unsigned long n, int base = DEC)
    {
      if (base == BYTE) write(error, n);
      else printNumber(error, n, base);
    }

    FORCE_INLINE void print(bool error, double n, int digits = 2)
    {
      printFloat(error, n, digits);
    }

    FORCE_INLINE void println(bool error)
    {
      print(error, '\n');  
    }

    FORCE_INLINE void println(bool error, const String &s)
    {
      print(error, s);
      println(error);
    }

    FORCE_INLINE void println(bool error, const char c[])
    {
      print(error, c);
      println(error);
    }

    FORCE_INLINE void println(bool error, char c, int base = BYTE)
    {
      print(error, c, base);
      println(error);
    }

    FORCE_INLINE void println(bool error, unsigned char b, int base = BYTE)
    {
      print(error, b, base);
      println(error);
    }

    FORCE_INLINE void println(bool error, int n, int base = DEC)
    {
      print(error, n, base);
      println(error);
    }

    FORCE_INLINE void println(bool error, unsigned int n, int base = DEC)
    {
      print(error, n, base);
      println(error);
    }

    FORCE_INLINE void println(bool error, long n, int base = DEC)
    {
      print(n, base);
      println(error);
    }

    FORCE_INLINE void println(bool error, unsigned long n, int base = DEC)
    {
      print(n, base);
      println(error);
    }

    FORCE_INLINE void println(bool error, double n, int digits = 2)
    {
      print(n, digits);
      println(error);
    }
    
    //
    // PROGMEM methods
    //
    
    FORCE_INLINE void printPGM(bool error, const char *str)
    {
      char ch=pgm_read_byte(str);
      while(ch)
      {
        write(error, ch);
        ch=pgm_read_byte(++str);
      }
    }
    
    FORCE_INLINE void printPGM_pair(bool error, const char *s_P, float v)
    {
      printPGM(error, s_P);
      printFloat(error, v, 2);
    }
    
    FORCE_INLINE void printPGM_pair(bool error, const char *s_P, double v)
    {
      printPGM(error, s_P);
      printFloat(error, v, 2);
    }
    
    FORCE_INLINE void printPGM_pair(bool error, const char *s_P, unsigned long v)
    {
      printPGM(error, s_P);
      printNumber(error, v, DEC);
    }
      
private:

    void printNumber(bool error, unsigned long, uint8_t);
    void printFloat(bool error, double, uint8_t);

  #if USE_PACEMAKER_FRAMES_FOR_DEBUG
    uint8_t debug_header[PM_HEADER_SIZE];
    uint8_t debug_buf[DEBUG_BUFFER_SIZE];
    uint8_t debug_buf_len;
    uint8_t debug_sequence_number;
    void send_debug_frame();
  #endif   
    
};

#endif //USE_PACEMAKER_FRAMES_FOR_DEBUG || USE_SERIAL_PORT_FOR_DEBUG || USE_EVENTS_FOR_DEBUG

//
// Macros to apply configuration at boot time (useful for testing without host-based configuration) 
//

#define APPLY_DEBUG_CONFIGURATION 1
#if APPLY_DEBUG_CONFIGURATION

// Macro used in DEBUG_STATIC_FIRMWARE_CONFIGURATION_LIST to write a firmware configuration value.
#define DEBUG_WRITE_FIRMWARE_CONFIGURATION(name,value) \
do { \
  static PROGMEM const char namePstr[] = name; \
  static PROGMEM const char valuePstr[] = value; \
  DEBUGPGM("WriteConfig: "); \
  DEBUGPGM_P(namePstr); \
  DEBUG("="); \
  DEBUGLNPGM_P(valuePstr); \
  strcpy_P((char *)parameter_value, namePstr); \
  strcpy_P((char *)&parameter_value[sizeof(namePstr)], valuePstr); \
  handle_firmware_configuration_request((char *)parameter_value, (char *)&parameter_value[sizeof(namePstr)]); \
} while (0) \

#define DEBUG_READ_FIRMWARE_CONFIGURATION(name) \
do { \
  static PROGMEM const char namePstr[] = name; \
  DEBUGPGM("ReadConfig: "); \
  DEBUGPGM_P(namePstr); \
  strcpy_P((char *)parameter_value, namePstr); \
  handle_firmware_configuration_request((char *)parameter_value, 0); \
} while (0) \

// Macro used in DEBUG_STATIC_COMMAND_LIST to send a Pacemaker command using an array initializer
// for the parameter data.
// e.g., DEBUG_COMMAND_ARRAY("Request Num Input Switches", ORDER_REQUEST_INFORMATION, ARRAY({ PARAM_REQUEST_INFO_NUM_SWITCH_INPUTS }) )
#define DEBUG_COMMAND_ARRAY(name, order, parameter_array) \
do { \
  static PROGMEM const uint8_t parameterPstr[] = parameter_array; \
  DEBUGPGM("\nExecuting "); \
  DEBUGPGM(STRINGIFY(order)); \
  DEBUGPGM(" :"); \
  DEBUGPGM(name); \
  DEBUGPGM("(len="); \
  DEBUG((int)sizeof(parameterPstr)); \
  DEBUGLNPGM(")"); \
  order_code = order; \
  parameter_length = sizeof(parameterPstr); \
  memcpy_P(parameter_value, parameterPstr, parameter_length); \
  process_command(); \
} while (0) \

// Macro used in DEBUG_STATIC_COMMAND_LIST to send a Pacemaker command using a string specifier
// for the parameter data.
// e.g., DEBUG_COMMAND_ARRAY("Request Num Input Switches", ORDER_REQUEST_INFORMATION, "\x10" )
#define DEBUG_COMMAND_STR(name, order, parameter_str) \
do { \
  static PROGMEM const char parameterPstr[] = parameter_str; \
  DEBUGPGM("\nExecuting: "); \
  DEBUGPGM(name); \
  DEBUGPGM("(len="); \
  DEBUG((int)sizeof(parameterPstr) - 1); \
  DEBUGLNPGM(")"); \
  order_code = order; \
  parameter_length = sizeof(parameterPstr) - 1; \
  memcpy_P(parameter_value, parameterPstr, parameter_length); \
  process_command(); \
} while (0)

//
// Static Configuration to set at start up
//

#define DEBUG_STATIC_FIRMWARE_CONFIGURATION_LIST \
   DEBUG_WRITE_FIRMWARE_CONFIGURATION("system.num_digital_inputs", "6"); \
   DEBUG_WRITE_FIRMWARE_CONFIGURATION("system.num_digital_outputs", "2"); \
   DEBUG_WRITE_FIRMWARE_CONFIGURATION("system.num_pwm_outputs", "2"); \
   DEBUG_WRITE_FIRMWARE_CONFIGURATION("system.num_buzzers", "1"); \
   DEBUG_WRITE_FIRMWARE_CONFIGURATION("system.num_temp_sensors", "4"); \
   DEBUG_READ_FIRMWARE_CONFIGURATION("debug.stack_memory"); \
   DEBUG_READ_FIRMWARE_CONFIGURATION("debug.stack_low_water_mark"); \
   DEBUG_READ_FIRMWARE_CONFIGURATION("stats.queue_memory"); 
   
#define DEBUG_STATIC_COMMAND_LIST \
   DEBUG_COMMAND_ARRAY("Request Num Input Switches", ORDER_REQUEST_INFORMATION, ARRAY({ PARAM_REQUEST_INFO_NUM_SWITCH_INPUTS }) ); \
   DEBUG_COMMAND_STR("Request Num Output Switches", ORDER_REQUEST_INFORMATION, "\x11" ); \
   DEBUG_COMMAND_ARRAY("Request Num Buzzers", ORDER_REQUEST_INFORMATION, ARRAY({ PARAM_REQUEST_INFO_NUM_BUZZERS }) ); \
   DEBUG_COMMAND_STR("Flish Command Queue", ORDER_CLEAR_COMMAND_QUEUE, "" ); \
   DEBUG_COMMAND_STR("Read Quue Length", ORDER_READ_FIRMWARE_CONFIG_VALUE, "stats.queue_memory" ); \
   
#endif //if APPLY_DEBUG_CONFIGURATION  


#endif //DEBUG_H
