
#ifndef STEPPER_LIBRARY_H
#define STEPPER_LIBRARY_H

#ifdef  STEPPER_CONTROL_H
#define ISEXTCONTROL_TASK
#else
#define	ISEXTCONTROL_TASK extern
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

// Scheduler includes
#include "FreeRTOS.h"
#include "task.h"
#include "croutine.h"
#include "queue.h"
#include "systick.h"
#include "io.h"
#include "tachimetrica.h"
#include "eeprom_internal.h"

//#include "smu.h"
#define TIMEOUT_TRAFFICO_CANBUS_MS	(2000) // 2 secondi

ISEXTCONTROL_TASK void Control_Task( void * pvParameters );

void _PRINT(int x, const char *m);

volatile int is_running;

void Stepper_Task( void *pvParameters );

void calibrate_zero( void );

#endif	//STEPPER_LIBRARY_H
