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
// This file provides shared helper/utility functions for handling orders
// 

#ifndef ORDER_HELPERS_H
#define ORDER_HELPERS_H

#include <stdint.h>

int8_t get_num_devices(uint8_t num_devices);
bool is_pin_in_use(uint8_t pin);

uint8_t checkDigitalPin(uint8_t pin);
uint8_t checkAnalogOrDigitalPin(uint8_t pin);

#endif