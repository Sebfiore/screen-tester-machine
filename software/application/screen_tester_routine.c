#include "screen_tester_routine.h"
#include "stepper_library.h"
#include "stepper_speed_lib.h"
#include "stepper_speed_timer.h"
#include "routine_helpers_library.h"

#include "smu.h"

// Implements core routine logic using helper libraries (e.g., calibration, zeroing)
// Ideally called from main — effectively a stepper motor task

int path_8[NUMBER_OF_POINTS][2] = {     // Used in mm, for new top zeroing
                                        // Watch out for issues if starting at (0,0)
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

void routine_2()        // Temporary test routine
{
        _DBG("Routine_2\n");
}

void routine_3()        // Test for stepper speed function
{
    _DBG("Stepper speed function test\n");

}

void routine_4()        // Test stepper function using timers
{
    _DBG("Stepper speed function test - routine_4()\n");
    constant_speed_calibration();

    int steps = 4500;

    int x_min = 2000;        // Calibrated min speed for X-axis
    int x_max = 26000;       // X-axis step loss limit
    int y_min = 1000;
    int y_max = 18000;       // Max speed Y-axis

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

void routine_5()        // Execute path using timer-based stepper control
{
    _DBG("Path execution with timers - routine_5()\n");
    _DBG("Calibration\n");
    constant_speed_calibration();

    _DBG("Reaching zero\n");
    reach_zero( X_ZERO_STEPS, Y_ZERO_STEPS );

    int count_to_calibrate = 0;

    while( 1 ) {
        if( count_to_calibrate >= 5 )       // Recalibrate every 5 cycles
        {
            constant_speed_calibration();
            reach_zero( X_ZERO_STEPS, Y_ZERO_STEPS );
            count_to_calibrate = 0;
        }

        execute_path( path_8 );

        count_to_calibrate++;
    }
}

void routine_6()        // Perform zero calibration via calibration + manual input
{
    constant_speed_calibration();
    calibrate_zero();
}

void routine_7()        // Check for step loss on Y-axis
{
    _DBG("Verifying Y-axis step loss\n");
    constant_speed_calibration();

    _DBG("Reaching zero\n");
    reach_zero( X_ZERO_STEPS, Y_ZERO_STEPS );

    for( int i = 0; i < 10; i++ ) {
        reach_point_mm( 0, 200 );
        reach_point_mm( 0, 0 );
    }
}

void routine_8()        // Execute path in loop with timer control
{
    _DBG("Calibration\n");
    constant_speed_calibration();

    _DBG("Reaching zero\n");
    reach_zero( X_ZERO_STEPS, Y_ZERO_STEPS );

    for( int i = 0; i < 30; i++ ) {
        execute_path( path_8 );
    }
    while( 1 ){}
}

void routine_9()        // Execute path once with timer control
{
    _DBG("Calibration\n");
    constant_speed_calibration();

    _DBG("Reaching zero\n");
    reach_zero( X_ZERO_STEPS, Y_ZERO_STEPS );

    execute_path( path_8 );
}

void Console_Task( void *pvParameters ) // Default: runs routine_5
{
    (void)pvParameters;

    char cmd[128];
    uint32_t cmd_pos = 0;

    while (1)
    {
        routine_5(); // Default behavior

        if (cmd_pos >= sizeof(cmd) - 1)
        {
            cmd_pos = 0; // Reset if too long
        }

        cmd[cmd_pos] = _DG;

        if (cmd[cmd_pos] != 0)
        {
            if ((cmd[cmd_pos] == 0x0d) || (cmd[cmd_pos] == 0x0a))
            {
                cmd[cmd_pos] = 0;  // Null terminate

                // Parse command by prefix
                if (strstr(cmd, "pos") == cmd)
                {
                    calibrate_zero();
                }
                else if (strstr(cmd, "zero") == cmd)
                {
                    _DBG("Going to zero\n");
                }
                else if (strstr(cmd, "reset") == cmd)
                {
                    _DBG("Going to home\n");
                    constant_speed_calibration();
                }
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
                    char *sstr = cmd + 1;
                    uint32_t temp_cmd_pos = 1;

                    for (int vv = 0; vv < 3; vv++)
                    {
                        vals[vv] = atoi_8(sstr);
                        sstr = strchr(sstr, ' ');
                        if (sstr) sstr++;
                        else break;
                    }

                    _DBG("Datain: ");
                    _DBD32(vals[0]); _DBG(", ");
                    _DBD32(vals[1]); _DBG(", ");
                    _DBD32(vals[2]); _DBG("\n");

                    is_running = 3;
                }

                cmd_pos = 0;
            }
            else
            {
                cmd_pos++;
            }
        }
    }
}
