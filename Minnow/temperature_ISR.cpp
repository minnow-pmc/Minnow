/*
  temperature.c - temperature control
  Part of Marlin
  
 Copyright (C) 2011 Camiel Gubbels / Erik van der Zalm
 
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
 This firmware is a mashup between Sprinter and grbl.
  (https://github.com/kliment/Sprinter)
  (https://github.com/simen/grbl/tree)
 
 It has preliminary support for Matthew Roberts advance algorithm 
    http://reprap.org/pipermail/reprap-dev/2011-May/003323.html

 */

#include "Minnow.h"
#include "temperature_ISR.h" 
#include "SoftPwmState.h"

#include "Device_Heater.h"
#include "Device_Buzzer.h"
#include "Device_PwmOutput.h"

#include "thermistortables.h"
 
volatile bool temp_meas_ready = false;

// The Device_TemperatureSensor static are placed in this compilation unit to 
// allow potentially better compiler optimization in the ISR
uint8_t Device_TemperatureSensor::num_temperature_sensors = 0;
uint8_t *Device_TemperatureSensor::temperature_sensor_pins;
int8_t *Device_TemperatureSensor::temperature_sensor_types;
uint16_t *Device_TemperatureSensor::temperature_sensor_isr_raw_values;
uint16_t *Device_TemperatureSensor::temperature_sensor_raw_values;
 
// Function declarations
FORCE_INLINE void updateSoftPwm(); // needs to be non-static due to friend usage elsewhere
FORCE_INLINE void updateTemperatureSensorRawValues(); // needs to be non-static due to friend usage elsewhere

void temperature_ISR_init()
{
  // Use timer0 for temperature measurement
  // Interleave temperature interrupt with millies interrupt
  OCR0B = 128;
  TIMSK0 |= (1<<OCIE0B);  
}

 // Timer 0 is shared with millis
ISR(TIMER0_COMPB_vect)
{
  updateSoftPwm();
  updateTemperatureSensorRawValues();
}

// This is executed from the ISR
FORCE_INLINE void updateSoftPwm()
{
  uint8_t i;
  uint16_t device_bitmask;
  SoftPwmState *state;
  
  for (uint8_t j=0; j<2; j++)
  {
    if (j == 0)
    {
      device_bitmask = Device_Heater::soft_pwm_device_bitmask;
      if (device_bitmask == 0)
        continue;
      state = Device_Heater::soft_pwm_state;
    }
    else
    {
      device_bitmask = Device_PwmOutput::soft_pwm_device_bitmask;
      if (device_bitmask == 0)
        continue;
      state = Device_PwmOutput::soft_pwm_state;
    }
      
    i = 0;
    do
    {
      if ((device_bitmask & 1) != 0)
      {
        volatile uint8_t *output_reg = state->output_reg[i];
        if(state->pwm_isr_count == 0)
        {
          if ((state->pwm_count[i] = state->pwm_power[i]) > 0)
            *output_reg |= state->output_bit[i];
        }
        else
        {
          if (state->pwm_count[i] <= state->pwm_isr_count)
            *output_reg &= ~state->output_bit[i];
        }
      }
      device_bitmask >>= 1;
      i+=1;
    }
    while (device_bitmask != 0);

    state->pwm_isr_count += (1 << state->soft_pwm_scale);
    state->pwm_isr_count &= 0x7f;
  }

}

FORCE_INLINE void updateTemperatureSensorRawValues()
{
  static uint8_t temp_count = 0;
  static uint8_t temp_index = 0;

  const uint8_t num_sensors = Device_TemperatureSensor::num_temperature_sensors;
  if (num_sensors <= 4)
  {
    // if there are 4 or less sensors then we prepare and read the sensors on alternate interrupt cycles
    const uint8_t sensor = temp_index / 2;
    if (sensor < num_sensors)
    {
      const uint8_t type = Device_TemperatureSensor::temperature_sensor_types[sensor];
      if (type != TEMP_SENSOR_TYPE_INVALID && type < LAST_THERMISTOR_SENSOR_TYPE)
      {
        if ((temp_index & 1) == 0)
        {
          // prepare temperature reading
          const uint8_t pin = Device_TemperatureSensor::temperature_sensor_pins[sensor];
#ifdef MUX5
          if (pin > 7)
            ADCSRB = 1<<MUX5;
          else
#endif          
            ADCSRB = 0;
          ADMUX = ((1 << REFS0) | (pin & 0x07));
          ADCSRA |= 1<<ADSC; // Start conversion
        }
        else
        {
          // retrieve measurement
          Device_TemperatureSensor::temperature_sensor_isr_raw_values[sensor] += ADC;
        }
      }
      else
      {
        // TODO handle other sensor types
      }
    }
    // we loop through sensors every 8ms
    if (++temp_index >= 8)
    {
      temp_count += 1;
      temp_index = 0;
    }
  }
  else
  {
    // if there are more than 4 sensors then we both read and prepare on each interrupt cycle (but for consecutive sensors)
    
    // first, retrieve previous measurement (time slot 0 is only used if there are >=8 sensors)
    if (num_sensors >= 8 || (temp_index > 0 && temp_index <= num_sensors))
    {
      const uint8_t sensor = (temp_index > 0) ? temp_index - 1 : num_sensors - 1;
      const uint8_t type = Device_TemperatureSensor::temperature_sensor_types[sensor];
      if (type != TEMP_SENSOR_TYPE_INVALID && type < LAST_THERMISTOR_SENSOR_TYPE)
      {
        Device_TemperatureSensor::temperature_sensor_isr_raw_values[sensor] += ADC;
      }
      else
      {
        // TODO handle other types
      }
    }
    // second, prepare next measurement
    if (temp_index < num_sensors) 
    {
      // sensor == temp_index in this case
      const uint8_t type = Device_TemperatureSensor::temperature_sensor_types[temp_index];
      if (type != TEMP_SENSOR_TYPE_INVALID && type < LAST_THERMISTOR_SENSOR_TYPE)
      {
        const uint8_t pin = Device_TemperatureSensor::temperature_sensor_pins[temp_index];
#ifdef MUX5
        if (pin > 7)
          ADCSRB = 1<<MUX5;
        else
#endif        
          ADCSRB = 0;
        ADMUX = ((1 << REFS0) | (pin & 0x07));
        ADCSRA |= 1<<ADSC; // Start conversion
      }
      else
      {
        // TODO handle other sensor types
      }
    }
    // we loop through sensors no faster than every 8 ms
    if (++temp_index >= max(num_sensors,8))
    {
      temp_count++;
      temp_index = 0;
    }
  }
  
  // We oversample 16 times, so with 8 or fewer temperature sensors we update raw temperature readings every 8ms * 16 = 128ms
    
  if(temp_count >= OVERSAMPLENR) 
  {
    //Only update the raw values if they have been read. Else we could be updating them during reading.
    if (!temp_meas_ready) 
    {
      for (uint8_t i=0; i<num_sensors; i++)
      {
        Device_TemperatureSensor::temperature_sensor_raw_values[i] = Device_TemperatureSensor::temperature_sensor_isr_raw_values[i];
        Device_TemperatureSensor::temperature_sensor_isr_raw_values[i] = 0;
      }
    }
    else
    {
      for (uint8_t i=0; i<num_sensors; i++)
      {
        Device_TemperatureSensor::temperature_sensor_isr_raw_values[i] = 0;
      }
    }
    temp_meas_ready = true;
    temp_count = 0;
  }
}
