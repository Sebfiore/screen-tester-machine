# Screen Tester Machine  
*University Project Summary*

![System overview: Electrical subsystem with TB6600 drivers and LPC1768 control board.](electrical/images/IMG_3673.jpeg)

## Overview

The Screen Tester Machine is an interdisciplinary mechatronics university project that automates validation of capacitive touch displays by tightly integrating mechanics, electronics, and embedded software into a coherent control system. Precise multi-axis motion (via stepper motors and TB6600 drivers), compliant touch interfaces, and discrete actuation (solenoid taps) are coordinated in real time by firmware on an LPC1768 controller, with sensing (multiplexed endstops) providing the necessary feedback for safe, repeatable operation. This closed-loop system embodies core mechatronic challenges—synchronizing motion and force, adapting to mechanical tolerances making it both a testbed and demonstration of practical automation, control, and system-level design.


## Table of Contents

- [Features](#features)  
- [Subsystems](#subsystems)  
  - [Mechanical](#mechanical)  
  - [Electrical](#electrical)  
  - [Software / Firmware](#software--firmware)  
- [Architecture Summary](#architecture-summary)  
- [Contributors](#contributors)  
- [License](#license)  

## Features

- Precise dual-axis actuation via stepper motors  
- Modular touch heads (conceptual and 3D-printed realized versions)  
- Compliant mechanisms for controlled force transfer  
- Centralized control using an LPC1768-based custom board  
- Stepper driving through TB6600 modules  
- Multiplexed endstop sensing  
- Solenoid-based discrete tap actuation with protection  

## Subsystems

### Mechanical

Detailed design and evolution of the mechanical interface, including:  
- Complete system assembly  
- Two versions of the touch head (conceptual sketch and 3D-printed real version)  
- Two versions of compliant mechanisms for force modulation and alignment tolerance  

See [`mechanical/README.md`](mechanical/README.md) for full description, design rationale, and validation strategy.

### Electrical

Handles actuation signaling, sensing, and power distribution:  
- Dual TB6600 stepper driver control  
- Solenoid drive with flyback protection  
- Software-selectable endstop inputs via multiplexer  
- Power domains (24V, 12V, 5V) coordinated by the LPC1768 controller  

See [`electrical/README.md`](electrical/README.md) for subsystem architecture and component details.

### Software / Firmware

The embedded software is written in **C** and runs on a custom LPC1768 (ARM Cortex-M3) controller. It provides real-time orchestration of motion, touch actuation, sensing, and test sequencing for touchscreen validation.

#### Core Responsibilities
- **Motion Control:** Precise dual-axis stepper driving with trapezoidal velocity profiles (accel/constant/decel), coordinated gestures, and endstop-aware corrections. Timer interrupts and priority handling ensure sub-millisecond pulse accuracy.  
- **Test Sequence Engine:** Executes scripted routines (moves, taps, waits, conditional logic) with timing control, state persistence, and safe recovery on faults.  
- **Sensing & Safety:** Multiplexed endstop sampling, limit enforcement and command validation to prevent mechanical or electrical misuse.  

#### Implementation Highlights
- Written in embedded C with a modular architecture (motion control, parser, test engine, hardware abstraction).  
- Uses hardware timers, circular buffers, and interrupt-driven design to balance timing precision with communication reliability.  

#### Outcomes
- Achieved repeatable positioning (±0.1mm after calibration) and low command latency (<10ms typical).  
- Designed for extensibility: new test patterns, conditional behaviors, and additional sensors can be integrated without rewriting core logic.  
- Educationally demonstrates real-time embedded programming, state-machine design, and system integration in a constrained environment.


## Architecture Summary

The system is arranged in layers:

1. **Mechanical layer** provides accurate positioning and compliant touch interface.  
2. **Electrical/control layer** (LPC1768) interprets test sequences, drives actuators (steppers and solenoid), and reads limit conditions.  
3. **Touch interaction layer** (touch heads + compliant mechanism) mediates contact with the device under test to emulate realistic touch events.  

Coordination among layers enables automated, repeatable test routines for capacitive displays.

## License

This project is licensed under the MIT License. See the [LICENSE](./LICENSE) file for details.
