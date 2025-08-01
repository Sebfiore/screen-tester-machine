#include "stepper_speed_timer.h"

// This file defines stepper speed functions.
// Other utility functions (e.g., for calibration or zeroing) that use these routines will include this library.
// Goal: set speed as steps per second (consider using mm/s).
// Reactivate interrupts when needed.

#include "LPC17xx.h"

typedef struct {
    int total_steps;
    int previous_count;
    int remaining_steps;
    int current_speed;       // Steps per second, initialized to min speed
    int delta_speed;
    int min_speed;           // Steps per second
    int max_speed;           // Steps per second
    int accel_steps;         // Steps during acceleration
    volatile int phase;      // Must be volatile: used in ISR. 0=accel, 1=cruise, 2=decel, 4=done
    int update_speed_flag;
} ax_state;

ax_state x_ax;
ax_state y_ax;

int done_x = 0;
int done_y = 0;

int tim0_update = 0;
int tim1_update = 0;

void set_directions(int xs, int ys) {
    if (xs > 0)
        GPIO_SetValue(X_PORT, X_DIR_PIN);   // X direction
    else
        GPIO_ClearValue(X_PORT, X_DIR_PIN);

    if (ys < 0)
        GPIO_SetValue(Y_PORT, Y_DIR_PIN);   // Y direction
    else
        GPIO_ClearValue(Y_PORT, Y_DIR_PIN);
}

void init_timer0(uint32_t frequency_hz) {
    // Frequency of interrupts = steps per second
    uint32_t pclk = SystemCoreClock / 4;           // LPC1768 typical: 100 MHz / 4 = 25 MHz
    uint32_t match_value = pclk / frequency_hz;

    LPC_TIM0->MR0 = match_value;
    LPC_TIM0->MCR = 3;      // Interrupt and reset on MR0
    LPC_TIM0->TCR = 1;      // Enable timer
    NVIC_EnableIRQ(TIMER0_IRQn);
}

void init_timer1(uint32_t frequency_hz) {
    uint32_t pclk = SystemCoreClock / 4;
    uint32_t match_value = pclk / frequency_hz;

    LPC_TIM1->MR0 = match_value;
    LPC_TIM1->MCR = 3;
    LPC_TIM1->TCR = 1;
    NVIC_EnableIRQ(TIMER1_IRQn);
}

void run_stepper_timer(int xs, int ys, int x_speed_min, int y_speed_min, int x_speed_max, int y_speed_max) {
    // Start a move with acceleration, cruise, and deceleration phases.
    // Speed parameters could be constants, but using arguments allows calibration.

    set_directions(xs, ys);

    // --- X AXIS SETUP ---
    x_ax.total_steps     = abs(xs);
    x_ax.remaining_steps = abs(xs);
    x_ax.current_speed   = x_speed_min;
    x_ax.min_speed       = x_speed_min;
    x_ax.max_speed       = x_speed_max;
    x_ax.delta_speed     = X_DELTA_SPEED;
    x_ax.previous_count  = 0;
    x_ax.phase           = 0;
    x_ax.accel_steps     = 0;
    x_ax.update_speed_flag = 0;

    // --- Y AXIS SETUP ---
    y_ax.total_steps     = abs(ys);
    y_ax.remaining_steps = abs(ys);
    y_ax.current_speed   = y_speed_min;
    y_ax.min_speed       = y_speed_min;
    y_ax.max_speed       = y_speed_max;
    y_ax.delta_speed     = Y_DELTA_SPEED;
    y_ax.previous_count  = 0;
    y_ax.phase           = 0;
    y_ax.accel_steps     = 0;
    y_ax.update_speed_flag = 0;

    // --- X acceleration calculations ---
    int x_acceleration_steps = (x_ax.max_speed - x_ax.min_speed) / X_DELTA_SPEED;

    // Case: Not enough distance to reach max speed
    if (x_ax.total_steps <= (x_acceleration_steps * 2)) {
        x_ax.accel_steps = x_ax.total_steps / 2;
        x_ax.max_speed = x_ax.min_speed + (X_DELTA_SPEED * x_ax.accel_steps);
    } else {
        x_ax.accel_steps = x_acceleration_steps;
    }

    // --- Y acceleration calculations ---
    int y_acceleration_steps = (y_ax.max_speed - y_ax.min_speed) / Y_DELTA_SPEED;

    if (y_ax.total_steps <= (y_acceleration_steps * 2)) {
        y_ax.accel_steps = y_ax.total_steps / 2;
        y_ax.max_speed = y_ax.min_speed + (Y_DELTA_SPEED * y_ax.accel_steps);
    } else {
        y_ax.accel_steps = y_acceleration_steps;
    }

    tim0_update = x_ax.current_speed;
    tim1_update = y_ax.current_speed;

    // Set steps per second (Hz) for both timers
    init_timer0(x_ax.current_speed);
    init_timer1(y_ax.current_speed);

    // Wait until both axes complete their motion
    while (x_ax.phase != 4 || y_ax.phase != 4) {
        // Do nothing; volatile flags will be updated by the ISRs
    }
}

// --- TIMER0 ISR (X Axis) ---
void TIMER0_IRQHandler(void) {
    if (LPC_TIM0->IR & 1) {
        LPC_TIM0->IR = 1;  // Clear interrupt

        if (x_ax.phase != 4) {
            GPIO_SetValue(X_PORT, X_PULSE_PIN);     // Pulse HIGH
            for (volatile int i = 0; i < 50; i++);   // Short delay (≥5 µs)
            GPIO_ClearValue(X_PORT, X_PULSE_PIN);    // Pulse LOW
            x_ax.remaining_steps--;

            // Acceleration
            if (x_ax.current_speed < x_ax.max_speed && x_ax.phase == 0) {
                x_ax.current_speed += X_DELTA_SPEED;
                tim0_update = x_ax.current_speed;
            } else if (x_ax.phase == 0) {
                x_ax.phase = 1;  // Switch to cruise
            }

            // Cruise → deceleration
            if (x_ax.phase == 1 && x_ax.remaining_steps <= x_ax.accel_steps) {
                x_ax.phase = 2;
            }

            // Deceleration
            if (x_ax.current_speed > x_ax.min_speed && x_ax.phase == 2) {
                x_ax.current_speed -= X_DELTA_SPEED;
                tim0_update = x_ax.current_speed;
            }

            // End of motion
            if (x_ax.remaining_steps <= 0) {
                x_ax.phase = 4;
            }
        }
    }

    if (tim0_update < 1) tim0_update = 1;
    LPC_TIM0->MR0 = (SystemCoreClock / 4) / tim0_update;
    NVIC_ClearPendingIRQ(TIMER0_IRQn);
}

// --- TIMER1 ISR (Y Axis) ---
void TIMER1_IRQHandler(void) {
    if (LPC_TIM1->IR & 1) {
        LPC_TIM1->IR = 1;  // Clear interrupt

        if (y_ax.phase != 4) {
            GPIO_SetValue(Y_PORT, Y_PULSE_PIN);     // Pulse HIGH
            for (volatile int i = 0; i < 50; i++);   // Short delay
            GPIO_ClearValue(Y_PORT, Y_PULSE_PIN);    // Pulse LOW
            y_ax.remaining_steps--;

            // Acceleration
            if (y_ax.current_speed < y_ax.max_speed && y_ax.phase == 0) {
                y_ax.current_speed += Y_DELTA_SPEED;
                tim1_update = y_ax.current_speed;
            } else if (y_ax.phase == 0) {
                y_ax.phase = 1;  // Switch to cruise
            }

            // Cruise → deceleration
            if (y_ax.phase == 1 && y_ax.remaining_steps <= y_ax.accel_steps) {
                y_ax.phase = 2;
            }

            // Deceleration
            if (y_ax.current_speed > y_ax.min_speed && y_ax.phase == 2) {
                y_ax.current_speed -= Y_DELTA_SPEED;
                tim1_update = y_ax.current_speed;
            }

            // End of motion
            if (y_ax.remaining_steps <= 0) {
                y_ax.phase = 4;
            }
        }
    }

    if (tim1_update < 1) tim1_update = 1;
    LPC_TIM1->MR0 = (SystemCoreClock / 4) / tim1_update;
    NVIC_ClearPendingIRQ(TIMER1_IRQn);
}
