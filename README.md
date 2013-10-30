Minnow - A High Performance 3D Printer and CNC Firmware for Arduino
===================================================================

Minnow is an Arduino-based Pacemaker client implementation for 3D printer, CNC and laser cutter controller boards.

The Pacemaker protocol is defined here: https://github.com/JustAnother1/Pacemaker

The Minnow firmware was originally created by Robert Fairlie-Cuninghame based largely on the Marlin printer firmware by Erik van der Zalm.

The firmware is intended for both standalone Arduino-based controller boards (using a PC or similar host platform) as well integrated Arduino + ARM controller boards. 

The Minnow firmware and Pacemaker protocol have been specifically designed to support a sustained, high rate of individual movement control segments with minimal processing overhead on the client, thereby allowing a far more sophisticated level of motion planning, co-ordinate remapping and better support for non-linear co-ordinate systems without requiring a real time operating system on the host.

The firmware has also been designed to provide a flexible host-based configuration system so that individual firmware builds are not required for different printer and controller configurations. 

The Minnow firmware also supports an arbitrary number of devices - steppers, digital input & outputs, heaters, fans, buzzers, etc (limited only by the device's capabilities). 

System requirements:
 - a 16Mhz or 20Mhz AVR Arduino (i.e., not Due)
 - at least 64KB flash memory (currently although this could be reduced using static configuration)
 - at least 2KB SRAM memory

TODO List 
- Makefile and Arduino libraries directory
- Add event handling
- Fully test movement control
- Add advanced stepper & heater configuration
- Added thermocouple support (thermistor support already available)
- Add coding guide
- Define LCD and Rotary encoder extensions
- World domination

Currently supported firmware configuration commands:
---------------------------------------------------

* System level configuration elements
  - system.board_identity
  - system.board_serialnum
  - system.hardware_name
  - system.hardware_type
  - system.hardware_rev
  - system.reset_eeprom (operation)
  
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
  - devices.heater.<device number>.use_pid
  - devices.heater.<device number>.pid_range
  - devices.heater.<device number>.kp
  - devices.heater.<device number>.ki
  - devices.heater.<device number>.kd
  - devices.heater.<device number>.dpi_do_autotune (operation)

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
  - debug.stack_memory
  
* Diagnostic/development elements
  - debug.stack_low_water_mark
  


