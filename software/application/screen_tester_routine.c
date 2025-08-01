#include "screen_tester_routine.h"
#include "stepper_library.h"
#include "stepper_speed_lib.h"
#include "stepper_speed_timer.h"
#include "routine_helpers_library.h"

#include "smu.h"

//implementare ciò che fa effettivamente, utilizando le librerie, anche elementi come raggiungimento zero o calibrazione
//idealmente una funzione chiamata dal main. Serve un header?
//quindi, di fatto, la stepper task

int path_8[NUMBER_OF_POINTS][2] = {		//utilizzata, in mm, per nuovo zero su parte superiore
                                        //verificate problemi se 0, 0 iniziale
    { 0, 0 },
    { -30, 0 },
    { -60, 0 },
    { -65, 22 },
    { 6, 22 },
    { -30, 82 },
    { -30, 102 },
    { -66, 162 },
    { 6, 162 },
    { 0, 181 },
    { -30, 181 },
    { -60, 181 },
};

void routine_2()		//per test, momentaneo
{
		_DBG("Routine_2\n");
}

void routine_3()        //test nuova funzione velocità stepper
{
    _DBG("Stepper speed function test\n");

}

void routine_4()        //test funzione stepper con timers
{
    _DBG("Stepper speed function test - routine_4()\n");
    constant_speed_calibration();


    int steps = 4500;

    int x_min = 2000;		//2000 valore calibrato asse x
    int x_max = 26000;		//28000 limite per perdita steps asse x
    int y_min = 1000;
    int y_max = 18000;		//20000;
    
    while( 1 )
    {
        _DBG("start first direction\n");

        run_stepper_timer( steps, steps, x_min, y_min, x_max, y_max );

        _DBG("end 1\n");
        for( volatile int i = 0; i < 300; i++ );

        _DBG("Start second direction\n");

        run_stepper_timer( -steps, -steps, x_min, y_min, x_max, y_max );

        _DBG("end 2\n");
        for( volatile int i = 0; i < 300; i++ );
    
    }
}

void routine_5()        //esecuzione path con funzione timer
{
    //calibrazione
    _DBG("Path execution with timers - routine_5()\n");
    _DBG("Calibrazione/n");
    constant_speed_calibration();

    //raggiungimento posizione iniziale
    _DBG("Raggiungimento zero/n");
    //run_stepper_timer( X_ZERO_STEPS, -Y_ZERO_STEPS, X_CALIBRATED_MIN_SPEED, Y_CALIBRATED_MIN_SPEED, X_CALIBRATED_MAX_SPEED, Y_CALIBRATED_MAX_SPEED );
    reach_zero( X_ZERO_STEPS, Y_ZERO_STEPS );

    //ìnizio routine di test
    int count_to_calibrate = 0;

    while( 1 ) {
        if( count_to_calibrate >= 5 )       //calibrazione ogni 5 cicli
        {
            constant_speed_calibration();
            reach_zero( X_ZERO_STEPS, Y_ZERO_STEPS );
            count_to_calibrate = 0;
        }

        execute_path( path_8 );

        count_to_calibrate++;
    }
}

void routine_6()        //per calibrazione zero
{
    constant_speed_calibration();
    calibrate_zero();
}

void routine_7()
{
    //calibrate_zero();
    _DBG("Verifica perdita steps asse Y/n");
    constant_speed_calibration();

    //raggiungimento posizione iniziale
    _DBG("Raggiungimento zero/n");
    //run_stepper_timer( X_ZERO_STEPS, Y_ZERO_STEPS, X_CALIBRATED_MIN_SPEED, Y_CALIBRATED_MIN_SPEED, X_CALIBRATED_MAX_SPEED, Y_CALIBRATED_MAX_SPEED );
    reach_zero( X_ZERO_STEPS, Y_ZERO_STEPS );

    //for( volatile int i = 0; i < 50000000; i++ );

    //ìnizio routine di test
    for( int i = 0; i < 10; i++ ) {
        reach_point_mm( 0, 200 );
        //for( volatile int i = 0; i < 100000; i++ );
        reach_point_mm( 0, 0 );
        //for( volatile int i = 0; i < 100000; i++ );
    }

    //for( volatile int i = 0; i < 50000000; i++ );
}

void routine_8()        //esecuzione path con funzione timer
{
    //calibrazione
    _DBG("Calibrazione/n");
    constant_speed_calibration();

    //raggiungimento posizione iniziale
    _DBG("Raggiungimento zero/n");
    //run_stepper_timer( X_ZERO_STEPS, -Y_ZERO_STEPS, X_CALIBRATED_MIN_SPEED, Y_CALIBRATED_MIN_SPEED, X_CALIBRATED_MAX_SPEED, Y_CALIBRATED_MAX_SPEED );
    reach_zero( X_ZERO_STEPS, Y_ZERO_STEPS );

    //ìnizio routine di test
    for( int i = 0; i < 30; i++ ) {
        execute_path( path_8 );
    }
    while( 1 ){}
}

void routine_9()        //esecuzione path con funzione timer
{
    //calibrazione
    _DBG("Calibrazione/n");
    constant_speed_calibration();

    //raggiungimento posizione iniziale
    _DBG("Raggiungimento zero/n");
    //run_stepper_timer( X_ZERO_STEPS, -Y_ZERO_STEPS, X_CALIBRATED_MIN_SPEED, Y_CALIBRATED_MIN_SPEED, X_CALIBRATED_MAX_SPEED, Y_CALIBRATED_MAX_SPEED );
    reach_zero( X_ZERO_STEPS, Y_ZERO_STEPS );

    //ìnizio routine di test
    execute_path( path_8 );
}


void Console_Task( void *pvParameters ) // di default chiama routine_4
{
    // _DBG("Consose_Task")
    (void)pvParameters;

    char cmd[128];
    uint32_t cmd_pos = 0;

    // Infinite loop
    while (1)
    {
        routine_5(); // di default

        if (cmd_pos >= sizeof(cmd) - 1)
        {
            // The command is too long
            cmd_pos = 0;
        }

        // Read
        cmd[cmd_pos] = _DG;

        // New char
        if (cmd[cmd_pos] != 0)
        {
            // New line, parse it
            if ((cmd[cmd_pos] == 0x0d) || (cmd[cmd_pos] == 0x0a))
            {
                // Add the end of the string
                cmd[cmd_pos] = 0;

                // Store the "zero" position
                // Note: 'cmd == strstr(cmd, "...")' is almost certainly
                // not what you want. strstr returns a pointer to the first
                // occurrence of the substring. To check if a string *starts*
                // with a substring, you typically compare the returned pointer
                // with 'cmd' (the beginning of the string). A more robust
                // approach for exact command matching is 'strcmp'.
                // If you want to check for a prefix, 'strstr(cmd, "pos") == cmd' works.
                // Assuming you want to check if the string *starts* with the command.
                if (strstr(cmd, "pos") == cmd)
                {
                    calibrate_zero();
                }
                else if (strstr(cmd, "zero") == cmd) // andare a punto iniziale dello schermo
                {
                    _DBG("Going to zero\n");
                }
                // Move to the zero position
                else if (strstr(cmd, "reset") == cmd)
                {
                    _DBG("Going to home\n");
                    constant_speed_calibration();
                }
                // Execute the path array
                else if (strstr(cmd, "path") == cmd)
                {
                    _DBG("execute path");

                }
                else if (strstr(cmd, "k") == cmd)
                {
                    // Command 'k' logic
                }
                else if (strstr(cmd, "c") == cmd)
                {
                    _DBG("routine 2\n");
                }
                else if (strstr(cmd, "z") == cmd)
                {
                    _DBG("\n");
                }
                else if (strstr(cmd, "y") == cmd)
                {
                    _DBG("");

                }
                else if (strstr(cmd, "m") == cmd)
                {
                    is_running = 0;

                    int vals[3];
                    char *sstr = cmd + 1; // Start parsing after 'm'
                    uint32_t temp_cmd_pos = 1; // Keep track of position for search_chars_moving_and_skipping

                    for (int vv = 0; vv < 3; vv++)
                    {
                        // Assuming atoi_8 is similar to atoi and handles parsing numbers.
                        // You need to ensure sstr is updated correctly after each atoi_8 call
                        // to point to the next number. The original code's `search_chars_moving_and_skipping`
                        // is problematic because `cmd_pos` is the overall buffer position,
                        // not relative to `sstr`.
                        // A safer way is to use sscanf or manual parsing.
                        // For demonstration, adapting to the original intent:
                        vals[vv] = atoi_8(sstr);
                        // This function needs to advance `sstr` to the next number's start.
                        // The original `search_chars_moving_and_skipping` uses `cmd_pos`,
                        // which is for the overall command buffer fill level, not parsing `sstr`.
                        // A more standard C way would be to find the next space or end of string.
                        // Example (conceptual, adjust based on `atoi_8` and `search_chars_moving_and_skipping` exact behavior):
                        sstr = strchr(sstr, ' '); // Find next space
                        if (sstr) sstr++; // Move past the space
                        else break; // No more spaces, no more numbers
                    }

                    _DBG("Datain: ");
                    _DBD32(vals[0]); _DBG(", ");
                    _DBD32(vals[1]); _DBG(", ");
                    _DBD32(vals[2]); _DBG("\n");

                    //run(vals[0], vals[1], vals[2]);

                    is_running = 3;
                }
                else
                {
                    // routine_5(); // If no command matches, what happens?
                }

                cmd_pos = 0; // Reset command buffer after processing
            }
            // Store the digit
            else
            {
                // Echo the character for user feedback (optional)
                //_PUTC(cmd[cmd_pos]);
                cmd_pos++;
            }
        }
    }
}

