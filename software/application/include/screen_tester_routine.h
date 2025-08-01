// Screen tester machine execution routines
#ifndef SCREEN_TESTER_ROUTINE_H
#define SCREEN_TESTER_ROUTINE_H

// Prevent multiple inclusion
// Define X and Y zeroing positions (step counts)
#define X_ZERO_STEPS 3890    // Calibrated X-axis zero: "Zero steps count: 3950, 830"
#define Y_ZERO_STEPS 840     // Calibrated Y-axis zero

void routine_1();       // Reserved or unused

void routine_2();       // Input test routine

void routine_3();       // Test the run_xy() stepper function

void Console_Task(void *pvParameters);  // FreeRTOS console task

#endif
