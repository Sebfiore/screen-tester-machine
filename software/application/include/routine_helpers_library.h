#ifndef ROUTINE_HELPERS_LIBRARY_H
#define ROUTINE_HELPERS_LIBRARY_H

// Solenoid control
#define SOLENOIDE_PORT 1
#define SOLENOIDE_PIN (1 << 26)
#define SOLENOIDE_DELAY 400 // For custom_delay(), in milliseconds

// MUX input selection pins
#define CH_SEL_A0      (1 << 25) // Port 3, Pin 25
#define CH_SEL_A1      (1 << 30) // Port 0, Pin 30
#define CH_SEL_A2      (1 << 19) // Port 1, Pin 19
#define CH_SEL_ENABLE  (1 << 29) // Port 0, Pin 29 (MUX enable)
#define DI_PIN         (1 << 10) // Port 0, Pin 10 (digital input)

// Calibration
#define CALIBRATION_SPEED 600

#define XAX_SWITCH 0
#define YAX_SWITCH 1

// Integer division factor for speed control
#define DIVISORE_STEPS 128 // Must be power of two for fast shifting on microcontroller

// Path configuration
#define NUMBER_OF_POINTS 12
#define X_STEPS_PER_mm (20 * DIVISORE_STEPS) // Steps per mm for X-axis
#define Y_STEPS_PER_mm (20 * DIVISORE_STEPS) // Steps per mm for Y-axis

// Path data defined in screen_tester_routine.c
extern int path_8[NUMBER_OF_POINTS][2];

// Helper function declarations
void constant_speed_calibration();
void reach_zero(int x_zero, int y_zero);
void calibrate_zero();
void solenoide_run(int on);
void reach_point_mm(int xax, int yax);
void execute_path(int path[NUMBER_OF_POINTS][2]);

#endif
