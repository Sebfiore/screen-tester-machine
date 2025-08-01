# Screen Tester Machine - Software Documentation

A student project developing automated testing software for capacitive touchscreen validation using embedded systems programming.

## 🎓 Project Overview

This project explores the software challenges involved in creating an automated touchscreen testing system. The software component focuses on real-time motion control, command processing, and test sequence execution using embedded C programming on the LPC1768 microcontroller platform.

## 🎯 Software Objectives

The software development addresses several key engineering challenges:
- **Real-time Control**: Implementing precise stepper motor control with timing constraints
- **Command Processing**: Designing a robust serial communication protocol
- **State Management**: Managing complex system states and error conditions
- **Modular Architecture**: Creating maintainable and extensible embedded code

## 💻 Software Architecture

### Core Components

**Motion Control Engine**
- Implements stepper motor control algorithms with acceleration/deceleration profiles
- Manages multi-axis coordination for complex gesture simulation
- Handles real-time positioning feedback and error correction
- Uses interrupt-driven timing for precise step generation

**Command Parser Module**
- Processes incoming serial commands using finite state machine approach
- Implements command validation and parameter checking
- Manages command queue for sequential execution
- Provides error reporting and status feedback

**Test Sequence Engine**
- Executes programmed test routines with precise timing control
- Supports conditional logic and loop structures
- Manages test state persistence and recovery
- Integrates logging functionality for result capture

### Implementation Insights

**Microcontroller Platform: LPC1768**
- ARM Cortex-M3 architecture provides sufficient processing power
- Multiple timer peripherals enable precise motion control timing
- UART interface handles serial communication at 115200 baud
- GPIO pins control stepper drivers and read endstop sensors

**Real-Time Considerations**
- Uses hardware timers for step pulse generation (sub-millisecond precision)
- Implements priority-based interrupt handling for motion control
- Employs circular buffers for command queuing and data logging
- Manages memory allocation for embedded environment constraints

## 🔧 Technical Implementation

### Motion Control Algorithm
The software implements trapezoidal velocity profiles:
- Acceleration phase with configurable ramp rates
- Constant velocity phase for smooth motion
- Deceleration phase with position prediction
- Real-time adjustment based on endstop feedback

## 📊 Software Features Developed

**Command Set Implementation**
- Movement commands with coordinate validation
- System control (home, reset, status)
- Configuration commands for calibration

**Safety Features**
- Limit switch monitoring and emergency stops
- Command validation to prevent mechanical damage

## 🛠️ Development Challenges Addressed

### Timing Precision
- Achieving sub-millisecond timing accuracy for touch simulation
- Balancing interrupt priorities to prevent motion jitter
- Implementing smooth acceleration curves in real-time

### Communication Reliability
- Handling serial communication errors and timeouts
- Implementing command acknowledgment system
- Managing buffer overflow conditions

### System Integration
- Coordinating multiple hardware subsystems
- Managing power sequencing and initialization
- Implementing diagnostic capabilities for debugging

## 📈 Technical Achievements

**Performance Metrics Achieved:**
- Position accuracy: ±0.1mm repeatability through software calibration
- Command response latency: <10ms average processing time
- Continuous operation capability with error recovery mechanisms
- Modular code structure enabling easy feature additions

**Learning Outcomes:**
- Embedded C programming for real-time applications
- Interrupt-driven system design and timing analysis
- State machine design for complex system control
- Hardware abstraction layer development

## 🔍 Code Structure

```
firmware/
├── src/
│   ├── main.c              # Main application loop
│   ├── motion_control.c    # Stepper motor algorithms
│   ├── command_parser.c    # Serial protocol handling
│   ├── test_engine.c       # Test sequence execution
│   └── hardware_hal.c      # Hardware abstraction
├── include/
│   ├── system_config.h     # Configuration parameters
│   ├── commands.h          # Command definitions
│   └── motion_types.h      # Data structures


## 🎯 Educational Value

This project demonstrates practical application of:
- **Embedded Systems Programming**: Low-level hardware control and real-time constraints
- **Communication Protocols**: Designing robust serial interfaces
- **Software Architecture**: Creating modular, maintainable embedded code
- **System Integration**: Coordinating software with mechanical and electrical components

The software component showcases the complexity involved in creating reliable automation systems and provides hands-on experience with industrial control programming techniques.