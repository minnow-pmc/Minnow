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


//
// Switches to make development testing easier
//
#define DEBUG_DONT_CHECK_CRC8_VALUE   0
#define DEBUG_NOT_STOPPED_INITIALLY   0
#define DEBUG_DISABLE_HOST_TIMEOUT    0

#define TRACE_ORDER                   0
#define TRACE_RESPONSE                0
#define TRACE_INITIAL_PIN_STATE       0

#define QUEUE_DEBUG                   0
#define MOVEMENT_DEBUG                0
#define HEATER_DEBUG                  0

// Some unit test support

#define QUEUE_TEST                    0


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
#define ERROR_CH(x) DSerial.print(true, x, BYTE)
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
#define DEBUG_CH(x) 
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
#define DEBUG_CH(x) DSerial.print(false, x, BYTE)
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
    
    FORCE_INLINE void print(bool error, char c, int base = DEC)
    {
      if (base == BYTE)
        write(error, (char)c);
      else
        printSignedNumber(error, c, base);
    }

    FORCE_INLINE void print(bool error, unsigned char b, int base = DEC)
    {
      if (base == BYTE)
        write(error, (unsigned char)b);
      else
        printUnsignedNumber(error, b, base);
    }

    FORCE_INLINE void print(bool error, int n, int base = DEC)
    {
      printSignedNumber(error, n, base);
    }

    FORCE_INLINE void print(bool error, unsigned int n, int base = DEC)
    {
      printUnsignedNumber(error, n, base);
    }

    FORCE_INLINE void print(bool error, long n, int base = DEC)
    {
      printSignedNumber(error, n, base);
    }

    FORCE_INLINE void print(bool error, unsigned long n, int base = DEC)
    {
      printUnsignedNumber(error, n, base);
    }

    FORCE_INLINE void print(bool error, double n, int digits = 2)
    {
      printFloat(error, n, digits);
    }

    FORCE_INLINE void println(bool error)
    {
      write(error, '\n');  
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

    FORCE_INLINE void println(bool error, char c, int base = DEC)
    {
      if (base == BYTE)
        write(error, (char)c);
      else
        printSignedNumber(error, c, base);
      println(error);
    }

    FORCE_INLINE void println(bool error, unsigned char b, int base = DEC)
    {
      if (base == BYTE)
        write(error, (unsigned char)b);
      else
        printUnsignedNumber(error, b, base);
      println(error);
    }

    FORCE_INLINE void println(bool error, int n, int base = DEC)
    {
      printSignedNumber(error, n, base);
      println(error);
    }

    FORCE_INLINE void println(bool error, unsigned int n, int base = DEC)
    {
      printUnsignedNumber(error, n, base);
      println(error);
    }

    FORCE_INLINE void println(bool error, long n, int base = DEC)
    {
      printSignedNumber(error, n, base);
      println(error);
    }

    FORCE_INLINE void println(bool error, unsigned long n, int base = DEC)
    {
      printUnsignedNumber(error, n, base);
      println(error);
    }

    FORCE_INLINE void println(bool error, double n, int digits = 2)
    {
      printFloat(error, n, digits);
      println(error);
    }
    
    //
    // PROGMEM methods
    //
    
    void printPGM(bool error, const char *str)
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
      printUnsignedNumber(error, v, DEC);
    }
      
private:

    void printSignedNumber(bool error, long value, uint8_t format);
    void printUnsignedNumber(bool error, unsigned long value, uint8_t format);
    void printFloat(bool error, double value, uint8_t decimaldigits);

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

#endif //DEBUG_H
