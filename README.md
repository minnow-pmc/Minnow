Minnow
======

An Arduino-based Pacemaker client implementation for 3D printer, CNC and laser cutter controller boards.

The Pacemaker protocol is defined here: https://github.com/JustAnother1/Pacemaker

The Minnow firmware was originally created by Robert Fairlie-Cuninghame based on the Marlin printer firmware by Erik van der Zalm.

There is still much to do. 

TODO List 
- Makefile and Arduino libraries directory
- Complete configuration implementation of all device types
- Add EEPROM storage of device names
- Integrate heater control parameters and functionality
- Add stepper control
- Add homing and endstop control
- Add movement control
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
  - devices.input_dig.<device number>.pin
  - devices.input_dig.<device number>.name
  - devices.output_dig.<device number>.pin
  - devices.output_dig.<device number>.name
  
* Statistics elements
  - stats.rx_count
  - stats.rx_errors
  - stats.queue_memory
  
* Diagnostic/development elements
  - debug.stack_memory
  - debug.stack_low_water_mark
  


