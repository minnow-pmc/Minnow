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
#define DEBUG_DONT_CHECK_CRC8_VALUE   1
#define DEBUG_NOT_STOPPED_INITIALLY   1

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
  #define DEBUG_BUFFER_SIZE             40
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
#define ERROR(x) DSerial.print(x)
#define ERROR_EOL() DSerial.write('\n')
#define ERROR_F(x,y) DSerial.print(x,y)
#define ERRORPGM(x) DSerial.printPGM(PSTR(x))
#define ERROREOL() DSerial.write('\n')
#define ERRORLN(x) do{DSerial.print(x);DSerial.write('\n');} while (0)
#define ERRORPGMLN(x) do{DSerial.printPGM(PSTR(x));DSerial.write('\n');}while(0)
#define ERRORPGM_ECHOPAIR(name,value) DSerial.printPGM_pair(PSTR(name),(value))


#if !DEBUG_ENABLED

#define DEBUG(x) 
#define DEBUG_EOL() 
#define DEBUG_F(x,y) 
#define DEBUGPGM(x) 
#define DEBUGEOL() 
#define DEBUGLN(x) 
#define DEBUGPGMLN(x) 
#define DEBUGPGM_ECHOPAIR(name,value) 

#else

class DebugSerial;
extern DebugSerial DSerial;
  
//
// Debug macros
//
#define DEBUG(x) DSerial.print(x)
#define DEBUG_EOL() DSerial.write('\n')
#define DEBUG_F(x,y) DSerial.print(x,y)
#define DEBUGPGM(x) DSerial.printPGM(PSTR(x))
#define DEBUGEOL() DSerial.write('\n')
#define DEBUGLN(x) do{DSerial.print(x);DSerial.write('\n');} while (0)
#define DEBUGPGMLN(x) do{DSerial.printPGM(PSTR(x));DSerial.write('\n');}while(0)
#define DEBUGPGM_ECHOPAIR(name,value) DSerial.printPGM_pair(PSTR(name),(value))

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
    
    FORCE_INLINE void write(uint8_t c)
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
  
    FORCE_INLINE void write(const char *str)
    {
      while (*str)
        write(*str++);
    }
    
    FORCE_INLINE void write(const uint8_t *buffer, size_t size)
    {
      while (size--)
        write(*buffer++);
    }

    FORCE_INLINE void print(const String &s)
    {
      for (int i = 0; i < (int)s.length(); i++) {
        write(s[i]);
      }
    }
    
    FORCE_INLINE void print(const char *str)
    {
      write(str);
    }
    
    void print(char, int = BYTE);
    void print(unsigned char, int = BYTE);
    void print(int, int = DEC);
    void print(unsigned int, int = DEC);
    void print(long, int = DEC);
    void print(unsigned long, int = DEC);
    void print(double, int = 2);

    void println(const String &s);
    void println(const char[]);
    void println(char, int = BYTE);
    void println(unsigned char, int = BYTE);
    void println(int, int = DEC);
    void println(unsigned int, int = DEC);
    void println(long, int = DEC);
    void println(unsigned long, int = DEC);
    void println(double, int = 2);
    void println(void);

    //
    // PROGMEM methods
    //
    
    FORCE_INLINE void printPGM(const char *str)
    {
      char ch=pgm_read_byte(str);
      while(ch)
      {
        write(ch);
        ch=pgm_read_byte(++str);
      }
    }
    
    FORCE_INLINE void printPGM_pair(const char *s_P, float v)
    {
      printPGM(s_P);
      printFloat(v, 2);
    }
    
    FORCE_INLINE void printPGM_pair(const char *s_P, double v)
    {
      printPGM(s_P);
      printFloat(v, 2);
    }
    
    FORCE_INLINE void printPGM_pair(const char *s_P, unsigned long v)
    {
      printPGM(s_P);
      printNumber(v, DEC);
    }
    
private:

    void printNumber(unsigned long, uint8_t);
    void printFloat(double, uint8_t);

  #if USE_PACEMAKER_FRAMES_FOR_DEBUG
    uint8_t debug_header[PM_HEADER_SIZE];
    uint8_t debug_buf[DEBUG_BUFFER_SIZE];
    uint8_t debug_buf_len;
    uint8_t debug_sequence_number;
    void send_debug_frame();
  #endif   
    
};

#endif //USE_PACEMAKER_FRAMES_FOR_DEBUG || USE_SERIAL_PORT_FOR_DEBUG || USE_EVENTS_FOR_DEBUG

#endif //DEBUG_H
