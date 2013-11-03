/*
  Minnow Pacemaker client firmware.
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
  
*/

#include "debug.h"

#if DEBUG_ENABLED

#include "crc8.h"

// Constructors ////////////////////////////////////////////////////////////////

DebugSerial::DebugSerial()
{
#if USE_PACEMAKER_FRAMES_FOR_DEBUG
  debug_header[PM_SYNC_BYTE_OFFSET] = SYNC_BYTE_RESPONSE_VALUE;
  debug_header[PM_ORDER_BYTE_OFFSET] = UNSOLICITED_FRAME_DEBUG_MESSAGE;
  debug_buf_len = 0;
  debug_sequence_number = 0;
#elif USE_SERIAL_PORT_FOR_DEBUG
  DEBUG_SERIAL_PORT.begin(DEBUG_SERIAL_PORT_BAUDRATE);
#endif  
}

// Public Methods //////////////////////////////////////////////////////////////

void
DebugSerial::begin()
{
#if USE_SERIAL_PORT_FOR_DEBUG
  DEBUG_SERIAL_PORT.begin(DEBUG_SERIAL_PORT_BAUDRATE);
#endif  
}

void DebugSerial::flush()
{
#if USE_PACEMAKER_FRAMES_FOR_DEBUG
  if (debug_buf_len > 0)
  {
    debug_header[PM_LENGTH_BYTE_OFFSET] = debug_buf_len + 2;
    debug_header[PM_CONTROL_BYTE_OFFSET] = CONTROL_BYTE_RESPONSE_DEBUG_BIT | (debug_sequence_number++ & CONTROL_BYTE_SEQUENCE_NUMBER_MASK);
    PSERIAL.write(debug_header, sizeof(debug_header));
    PSERIAL.write(debug_buf, debug_buf_len);
    PSERIAL.write(crc8(&debug_header[1], PM_HEADER_SIZE + debug_buf_len - 1));
    debug_buf_len = 0;
  }
#elif USE_SERIAL_PORT_FOR_DEBUG
  DEBUG_SERIAL_PORT.flush();
#elif USE_EVENTS_FOR_DEBUG
  #error Waiting for events framework implementation
#endif  
}

/// imports from print.h


void DebugSerial::print(bool error, long n, int base)
{
  if (base == 0) {
    write(error, n);
  } else if (base == 10) {
    if (n < 0) {
      print(error, '-');
      n = -n;
    }
    printNumber(error, n, 10);
  } else {
    printNumber(error, n, base);
  }
}


// Private Methods /////////////////////////////////////////////////////////////

void DebugSerial::printNumber(bool error, unsigned long n, uint8_t base)
{
  unsigned char buf[8 * sizeof(long)]; // Assumes 8-bit chars. 
  unsigned long i = 0;

  if (n == 0) {
    print(error, '0');
    return;
  } 

  while (n > 0) {
    buf[i++] = n % base;
    n /= base;
  }

  for (; i > 0; i--)
    print(error, (char) (buf[i - 1] < 10 ?
      '0' + buf[i - 1] :
      'A' + buf[i - 1] - 10));
}

void DebugSerial::printFloat(bool error, double number, uint8_t digits) 
{ 
  // Handle negative numbers
  if (number < 0.0)
  {
     print(error, '-');
     number = -number;
  }

  // Round correctly so that print(1.999, 2) prints as "2.00"
  double rounding = 0.5;
  for (uint8_t i=0; i<digits; ++i)
    rounding /= 10.0;
  
  number += rounding;

  // Extract the integer part of the number and print it
  unsigned long int_part = (unsigned long)number;
  double remainder = number - (double)int_part;
  print(error, int_part);

  // Print the decimal point, but only if there are digits beyond
  if (digits > 0)
    print(error, "."); 

  // Extract digits from the remainder one at a time
  while (digits-- > 0)
  {
    remainder *= 10.0;
    int toPrint = int(remainder);
    print(error, toPrint);
    remainder -= toPrint; 
  } 
}

// Preinstantiate Objects //////////////////////////////////////////////////////

DebugSerial DSerial;

#endif //DEBUG_ENABLED




