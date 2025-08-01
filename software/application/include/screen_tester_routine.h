//screen tester machine running rutine
#ifndef SCREEN_TESTER_ROUTINE_H
//per non includere più volte, se chiamata più volte
#define SCREEN_TESTER_ROUTINE_H

//zero
#define X_ZERO_STEPS 3890     //nuova calibrazione  "Zero steps count: 3950, 830,""
#define Y_ZERO_STEPS 840       

void routine_1();

void routine_2();       //per test inputs

void routine_3();       //test funzione run_xy

void Console_Task( void *pvParameters );

#endif
