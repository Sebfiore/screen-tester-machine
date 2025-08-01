#ifndef STEPPER_LIBRARY_H
#define STEPPER_LIBRARY_H

// External linkage macro for task declaration
#ifdef STEPPER_CONTROL_H
#define ISEXTCONTROL_TASK
#else
#define ISEXTCONTROL_TASK extern
#endif

// MCU includes
#include "lpc17xx_gpio.h"
#include "lpc17xx_libcfg.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_uart.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_wdt.h"
#include "debug_frmwrk.h"

// FreeRTOS includes
#include "FreeRTOS.h"
#include "task.h"
#include "croutine.h"
#include "queue.h"
#include "systick.h"

// Project-specific includes
#include "io.h"
#include "tachimetrica.h"
#include "eeprom_internal.h"

// Timeout for CAN bus traffic loss detection (in milliseconds)
#define TIMEOUT_TRAFFICO_CANBUS_MS (2000) // 2 seconds

// Task declarations
ISEXTCONTROL_TASK void Control_Task(void *pvParameters);

// Debug print helper
void _PRINT(int x, const char *m);

// Shared control flag
volatile int is_running;

// Stepper motor control task
void Stepper_Task(void *pvParameters);

// Interactive zero-point calibration
void calibrate_zero(void);

#endif // STEPPER_LIBRARY_H
