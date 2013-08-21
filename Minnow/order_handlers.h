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
// This file handles each of the Pacemaker Orders
// 

#ifndef ORDER_HANDLERS_H
#define ORDER_HANDLERS_H

// Basic Pacemaker Orders
void handle_resume_order();
void handle_request_information_order();
void handle_device_name_order();
void handle_request_temperature_reading_order();
void handle_get_heater_configuration_order();
void handle_configure_heater_order();
void handle_set_heater_target_temperature_order(bool validate_only);
void handle_get_input_switch_state_order();
void handle_set_output_switch_state_order(bool validate_only);
void handle_set_pwm_output_state_order(bool validate_only);
void handle_write_firmware_configuration_value_order();

#endif