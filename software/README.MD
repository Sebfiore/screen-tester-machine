# Software - Screen Tester Machine

This folder contains the embedded software developed for the control of a custom screen tester machine. The software runs on a custom-designed board based on the NXP LPC1768 microcontroller.

## Overview

The software controls two stepper motors through internal timers, executes path-following routines, and manages calibration procedures. Additionally, it reads multiple digital inputs through a multiplexer.

## Key Features

- **Trapezoidal Acceleration Profile**  
  Implementation of a motion control system using a trapezoidal acceleration profile. Internal timers of the LPC1768 are used to generate precise step signals for the stepper drivers.

- **Dual Stepper Motor Control**  
  Independent control of two stepper motors for coordinated motion in 2D space.

- **Path Following**  
  The system can execute predefined paths by interpreting sequences of coordinates. Movement between points is smooth and synchronized.

- **Calibration Routine**  
  A fully automated calibration routine aligns the system to known mechanical limits or reference points using limit switches.

- **Digital Input Reading via Multiplexer**  
  Multiple digital inputs (e.g. for sensor readings or buttons) are read through a multiplexer to expand the number of input channels available.

## Microcontroller

- **Model**: LPC1768 (ARM Cortex-M3)
- **Clock**: Running at 100 MHz
- **Timers**: Used for pulse generation with accurate control of step frequency

## Code Structure

- `application/`  
  Main application logic, including motor control, calibration, and path execution.

- `include/`  
  Header files for application modules.

## Notes

- The motion planning logic uses a **combination of integer and floating-point arithmetic**. Performance-critical sections prioritize integers, but floating-point is used where unavoidable.
- In particular, the **calculation of timer periods from acceleration** involves **divisions**, which are computationally expensive on the LPC1768 and may introduce latency.
- This division is necessary to compute the delay between step pulses based on current velocity and acceleration, essential for maintaining a trapezoidal motion profile.
- Input readings from the multiplexer are sampled periodically and debounced in software.
- System timing is managed via interrupt routines triggered by timer overflows, ensuring precise step generation and task scheduling.

## Limitations & Optimization Ideas

- The use of floating-point division to calculate acceleration-based delays could be optimized.
- Potential future improvements include:
  - **Using fixed-point arithmetic** to replace floating-point operations
  - **Lookup tables** for inverse acceleration values
  - **Precomputing acceleration profiles** and storing them in flash or RAM
  - Implementing interrupt-priority schemes to further reduce latency during motor movement

## Future Improvements

 - Add communication protocol (e.g. UART or CAN) for external interaction
- Extend path planning with spline interpolation
- Integrate external safety inputs and software failsafes
---

