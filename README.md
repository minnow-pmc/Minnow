Minnow
======

An Arduino-based Pacemaker client implementation for 3D printer, CNC and laser cutter controller boards.

The Pacemaker protocol is defined here: https://github.com/JustAnother1/Pacemaker

The Minnow firmware was originally created by Robert Fairlie-Cuninghame based on the Marlin printer firmware by Erik van der Zalm.

There is still much to do. 

TODO List 
- Makefile and Arduino libraries directory
- Add Heater PID support
- Add advanced stepper configuration
- Fully test movement control
- Add event handling
- Add coding guide

Currently supported firmware configuration commands:

* System level configuration elements
  - system.board_identity
  - system.board_serialnum
  - system.hardware_name
  - system.hardware_type
  - system.hardware_rev
  
* Device configuration elements
  - devices.digital_input.<device number>.name
  - devices.digital_input.<device number>.pin
  - devices.digital_output.<device number>.name
  - devices.digital_output.<device number>.pin
  - devices.pwm_output.<device number>.name
  - devices.pwm_output.<device number>.pin
  - devices.pwm_output.<device number>.use_soft_pwm
  - devices.buzzer.<device number>.name
  - devices.buzzer.<device number>.pin
  - devices.buzzer.<device number>.use_soft_pwm
  - devices.temp_sensor.<device number>.name
  - devices.temp_sensor.<device number>.pin
  - devices.temp_sensor.<device number>.type
  - devices.heater.<device number>.name
  - devices.heater.<device number>.pin
  - devices.heater.<device number>.temp_sensor
  - devices.heater.<device number>.max_temp
  - devices.heater.<device number>.power_on_level
  - devices.heater.<device number>.use_soft_pwm
  - devices.heater.<device number>.use_bang_bang
  - devices.heater.<device number>.bang_bang_hysteresis
  - devices.stepper.<device number>.name
  - devices.stepper.<device number>.enable_pin
  - devices.stepper.<device number>.enable_invert (0 = active low, 1 = active high)
  - devices.stepper.<device number>.direction_pin
  - devices.stepper.<device number>.direction_invert 
  - devices.stepper.<device number>.step_pin
  - devices.stepper.<device number>.step_invert 
  
* Statistics elements
  - stats.rx_count
  - stats.rx_errors
  - stats.queue_memory
  
* Diagnostic/development elements
  - debug.stack_memory
  - debug.stack_low_water_mark
  


