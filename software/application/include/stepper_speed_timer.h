#ifndef STEPPER_SPEED_TIMER_H
#define STEPPER_SPEED_TIMER_H

/* MCU headers */
#include "LPC17xx.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_clkpwr.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_libcfg.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_emac.h"
#include "lpc17xx_rit.h"
#include "lpc17xx_ssp.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_wdt.h"
#include "debug_frmwrk.h"

/* FreeRTOS headers */
#include "FreeRTOS.h"
#include "croutine.h"
#include "queue.h"
#include "task.h"

/* Application headers */
#include "printf.h"
#include "ad.h"
#include "eeprom_internal.h"
#include "io.h"
//#include "modbus_task.h"
#include "can_task.h"
#include "svnrev.h"

/* Screen tester includes */
#include "stepper_library.h"
#include "stepper_speed_lib.h"
#include "screen_tester_routine.h"

/* Stepper motor control pin definitions */
#define X_PORT      1
#define Y_PORT      1
#define X_DIR_PIN   (1<<28)
#define Y_DIR_PIN   (1<<22)
#define X_PULSE_PIN (1<<29)
#define Y_PULSE_PIN (1<<25)

/* Motor configuration */
#define STEPS_PER_ROTATION 800  // Microstepping resolution

/* Acceleration settings */
#define X_DELTA_SPEED 16        // Max acceleration step for X (higher = faster, but risk of step loss)
#define Y_DELTA_SPEED 9         // Lower acceleration for Y due to higher inertia

/* Calibrated speed limits */
#define X_CALIBRATED_MIN_SPEED 2000
#define X_CALIBRATED_MAX_SPEED 26000
#define Y_CALIBRATED_MIN_SPEED 1000
#define Y_CALIBRATED_MAX_SPEED 17000

/* Timer-based stepper movement function */
void run_stepper_timer(int xs, int ys, int x_speed_min /* in steps per second */, int y_speed_min, int x_speed_max, int y_speed_max);

#endif
