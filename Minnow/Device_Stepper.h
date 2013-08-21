//
// Input Switch Device Type File
//

#ifndef DEVICE_STEPPERS_H
#define DEVICE_STEPPERS_H

// TODO - Convert to class as per other device types.

#include "config.h"

//
// Configuration Functions
//

void stepper_device_init();

bool set_stepper_enable_pin(uint8_t device_number, uint8_t pin);
bool set_stepper_enable_level(uint8_t device_number, bool enable_on_level);
bool set_stepper_direction_pin(uint8_t device_number, uint8_t pin);
bool set_stepper_direction_invert(uint8_t device_number, bool invert);
bool set_stepper_step_pin(uint8_t device_number, uint8_t pin);
bool set_stepper_step_invert(uint8_t device_number, bool invert);
bool set_stepper_vref_digipot_pin(uint8_t device_number, uint8_t pin);
bool set_stepper_vref_digipot_value(uint8_t device_number, uint8_t value);
bool set_stepper_ms1_pin(uint8_t device_number, uint8_t pin);
bool set_stepper_ms1_value(uint8_t device_number, bool value);
bool set_stepper_ms2_pin(uint8_t device_number, uint8_t value);
bool set_stepper_ms2_value(uint8_t device_number, bool value);
bool set_stepper_ms3_pin(uint8_t device_number, uint8_t value);
bool set_stepper_ms3_value(uint8_t device_number, bool value);
bool set_stepper_fault_pin(uint8_t device_number, uint8_t value);

bool validate_stepper_config(uint8_t device_number, bool invert);

//
// Fast accessor macros
//

#define GET_NUM_STEPPER_DEVICES() (num_stepper_devices)
#define IS_STEPPER_DEVICE_USED(device_number) (stepper_enable_pins[device_number] != 0xFF)

#define GET_STEPPER_ENABLE_PIN(device_number) (stepper_enable_pins[device_number])
#define GET_STEPPER_ENABLE_LEVEL(device_number) (stepper_enable_on_levels[device_number])
#define GET_STEPPER_DIRECTION_PIN(device_number) (stepper_direction_pins[device_number])
#define GET_STEPPER_DIRECTION_INVERT(device_number) (stepper_direction_invert[device_number])
#define GET_STEPPER_STEP_PIN(device_number) (stepper_step_pins[device_number])
#define GET_STEPPER_STEP_INVERT(device_number) (stepper_step_invert[device_number])
#define GET_STEPPER_VREF_DIGIPOT_PIN(device_number) (stepper_vref_digipot_pins[device_number])
#define GET_STEPPER_VREF_DIGIPOT_VALUE(device_number) (stepper_vref_digipot_values[device_number])
#define GET_STEPPER_MS1_PIN(device_number) (stepper_ms1_pins[device_number])
#define GET_STEPPER_MS1_VALUE(device_number) (stepper_ms1_values[device_number])
#define GET_STEPPER_MS2_PIN(device_number) (stepper_ms2_pins[device_number])
#define GET_STEPPER_MS2_VALUE(device_number) (stepper_ms2_values[device_number])
#define GET_STEPPER_MS3_PIN(device_number) (stepper_ms3_pins[device_number])
#define GET_STEPPER_MS3_VALUE(device_number) (stepper_ms3_values[device_number])
#define GET_STEPPER_FAULT_PIN(device_number) (stepper_fault_pins[device_number])

//
// Internal Implementation Detail 
//

extern uint8_t num_stepper_devices; // number of configured output switches

extern char stepper_friendly_names[MAX_STEPPERS][MAX_DEVICE_FRIENDLY_NAME_LENGTH+1];
extern uint8_t stepper_enable_pins[MAX_STEPPERS];
extern bool stepper_enable_on_levels[MAX_STEPPERS];
extern uint8_t stepper_direction_pins[MAX_STEPPERS];
extern bool stepper_direction_invert[MAX_STEPPERS];
extern uint8_t stepper_step_pins[MAX_STEPPERS];
extern bool stepper_step_invert[MAX_STEPPERS];
extern uint8_t stepper_vref_digipot_pins[MAX_STEPPERS];
extern uint8_t stepper_vref_digipot_values[MAX_STEPPERS];
extern uint8_t stepper_ms1_pins[MAX_STEPPERS];
extern bool stepper_ms1_values[MAX_STEPPERS];
extern uint8_t stepper_ms2_pins[MAX_STEPPERS];
extern bool stepper_ms2_values[MAX_STEPPERS];
extern uint8_t stepper_ms3_pins[MAX_STEPPERS];
extern bool stepper_ms3_values[MAX_STEPPERS];
extern uint8_t stepper_fault_pins[MAX_STEPPERS];

#endif


    