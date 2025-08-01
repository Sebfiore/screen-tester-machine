#ifndef ROUTINE_HELPERS_LIBRARY_h
#define ROUTINE_HELPERS_LIBRARY_H

//solenoide
#define SOLENOIDE_PORT 1
#define SOLENOIDE_PIN 1<<26
#define SOLENOIDE_DELAY 400 //PER CUSTOM DELAY, SONO MS

//inputs
// Definizione dei pin per la selezione dei canali del MUX
#define CH_SEL_A0      (1 << 25) // Port 3, Pin 25
#define CH_SEL_A1      (1 << 30) // Port 0, Pin 30
#define CH_SEL_A2      (1 << 19) // Port 1, Pin 19
#define CH_SEL_ENABLE  (1 << 29) // Port 0, Pin 29 (enable MUX)
#define DI_PIN         (1 << 10) // Port 0, Pin 10 (data input)

//calibration
#define CALIBRATION_SPEED 600       //1000

#define XAX_SWITCH 0
#define YAX_SWITCH 1

//divisione tra interi per velocità
#define	DIVISORE_STEPS					128						//baste che sia un numero binario, veloce, per micro è solo uno shift

//path
#define NUMBER_OF_POINTS 12
#define X_STEPS_PER_mm   20	* DIVISORE_STEPS    /*verifico che non sia un problema di arrotondamento*///#define X_STEPS_PER_mm   20.2	* DIVISORE_STEPS        //fa 198 mm con 4000 steps => 20.2
#define Y_STEPS_PER_mm   20	* DIVISORE_STEPS    //#define Y_STEPS_PER_mm   20.2	* DIVISORE_STEPS        //assi uguali

extern int path_8[NUMBER_OF_POINTS][2]; // definition of points in screen_tester_routine.c

void constant_speed_calibration();

void reach_zero( int x_zero, int y_zero );

void calibrate_zero();

void solenoide_run( int on );

void reach_point_mm( int xax, int yax );

void execute_path( int path[NUMBER_OF_POINTS][2] );

#endif
