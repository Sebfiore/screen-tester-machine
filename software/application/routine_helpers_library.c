#include "routine_helpers_library.h"
#include "stepper_speed_timer.h"

//funzioni che utilizzano la stepper_library e che servono per la routine

//solenoide
void solenoide_run( int on ) {
    //1 = on, 0 = off
	if( on )
	{
		GPIO_SetValue( SOLENOIDE_PORT, SOLENOIDE_PIN );
	}

	else
	{
		GPIO_ClearValue( SOLENOIDE_PORT, SOLENOIDE_PIN );
	}
}

//delay
#define CYCLES 7550 //per ms

void custom_delay( int milliseconds )		//delay
{
	for( volatile int i = 0; i < milliseconds * CYCLES; i++ );
}

//inputs
// Array per memorizzare lo stato degli 8 ingressi
int in_status[8] = {0}; // 0 => switch aperto

// Funzione per impostare i pin di selezione del MUX
void set_mux_channel(int channel) {
// Imposta i pin di selezione A0, A1, A2 in base al canale (0-7)
	if (channel & 0x01) {
		GPIO_SetValue(3, CH_SEL_A0); // A0 = 1
	} else {
		GPIO_ClearValue(3, CH_SEL_A0); // A0 = 0
	}

	if (channel & 0x02) {
		GPIO_SetValue(0, CH_SEL_A1); // A1 = 1
	} else {
		GPIO_ClearValue(0, CH_SEL_A1); // A1 = 0
	}

	if (channel & 0x04) {
		GPIO_SetValue(1, CH_SEL_A2); // A2 = 1
	} else {
		GPIO_ClearValue(1, CH_SEL_A2); // A2 = 0
	}
}

// Funzione per leggere lo stato del pin DI (ingresso) tramite MUX
int read_mux_input()
{
	//uint32_t port_value = GPIO_ReadValue(0);	// Legge tutti i pin della porta 0
	//return (port_value & DI_PIN) ? 1 : 0; 		// Restituisce 1 se il pin DI alto, 0 se basso

	uint32_t port_value = GPIO_ReadValue(0);
	return !!(port_value & DI_PIN);  // Il doppio ! forza il risultato a 0 o 1
}

// Function to read a single MUX input channel, returns the state of that channel (0=ON, 1=OFF), logica inversa
int read_single_input( int channel ) { //da 0 a 7

    GPIO_SetValue(0, CH_SEL_ENABLE);           // Enable MUX

    set_mux_channel(channel);                  // Set the channel selection pins
    //_DBG("Channel selected: ");
    //_PRINT(channel, ",\n");

    for( volatile int del = 0; del < 1000; del++ );

    int result;
    if(read_mux_input() == 1) {
        result = 0;    // ON
    } else {
        result = 1;    // OFF
    }

    GPIO_ClearValue(0, CH_SEL_ENABLE);         // Disable MUX

    return result;
}

// Funzione per leggere tutti gli ingressi del MUX, aggiorna l'array globale in_status[]
void read_all_inputs() {
	GPIO_SetValue(0, CH_SEL_ENABLE);			// Abilita il MUX

    for (int i = 0; i < 8; i++) {      				// Ciclo per leggere ciascuno degli 8 canali del MUX
        set_mux_channel(i);

        for (volatile int j = 0; j < 100; j++);	// Attendere per stabilizzazione del segnale sul pin DI, calibrare !

        if(  read_mux_input() == 1 )
        {
            in_status[i] = 0;	//ON
        }
        else
        {
            in_status[i] = 1;	//OFF
        }
    }

    GPIO_ClearValue(0, CH_SEL_ENABLE);				// Disabilita il MUX
}

//Calibration
void constant_speed_calibration()		//funzione per calibrare
	{
	_DBG("Calibration\n");
	int xax_is_home = 0;
    int yax_is_home = 0;

	// Set directions
	GPIO_ClearValue( X_PORT, X_DIR_PIN );
	GPIO_SetValue( Y_PORT, Y_DIR_PIN );

	while( !xax_is_home || !yax_is_home  )
	{
		GPIO_ClearValue( X_PORT, X_PULSE_PIN );
		GPIO_ClearValue( Y_PORT, Y_PULSE_PIN );

		for( volatile int del = 0; del < CALIBRATION_SPEED; del++ );

		if( read_single_input(XAX_SWITCH) && !xax_is_home )	//asse x, passo a logica inversa per gli swich, meno errori di floating
			{
				GPIO_SetValue( 1, 1<<29 );		//x pulse
			}
		else if( !read_single_input(XAX_SWITCH) )		//secondo controllo per evitare errori di lettura
		{ xax_is_home = 1; }

		if( read_single_input(YAX_SWITCH) && !yax_is_home )
			{
				GPIO_SetValue( 1, 1<<25 ); //y pulse
			}
		else if( !read_single_input(YAX_SWITCH) )
		{ yax_is_home = 1; }


		//_PRINT( !read_single_input(0), ", " );
		//_PRINT( !read_single_input(1), ", " );
		//_PRINT( !read_single_input(2), ",\n" );

		for( volatile int del = 0; del < CALIBRATION_SPEED ; del++ );

	}
	_DBG("Calibration finished\n");

    }

//zero
void reach_zero( int x_zero, int y_zero )		//aggiornato, raggiunge parte superiore del display, utilizzato in routine_5
{
	_DBG("Going to zero\n");

    //run_stepper_timer( x_zero, y_zero, X_CALIBRATED_MIN_SPEED, Y_CALIBRATED_MIN_SPEED, X_CALIBRATED_MAX_SPEED, Y_CALIBRATED_MAX_SPEED );
    run_stepper_timer( x_zero, y_zero, 1000, 1000, 5000, 5000 );

    _DBG("Zero reached\n");
}

//path
int actual_position[2] = { 0, 0 };

void reach_point_mm( int xax, int yax )
{
    int val[2] = {0};		//inizializzo il vettore delle posizioni

	_DBG("actual position: ");
	_DBD32( actual_position[0] ); _DBG( ", " );
	_DBD32( actual_position[1] ); _DBG( "\n" );

    _DBG( "Going to point: " );
	_DBD32( xax ); _DBG( ", " );
	_DBD32( yax ); _DBG( "\n" );

	if (xax == actual_position[0] && yax == actual_position[1]) {
		_DBG("Already at target position\n");
		return;
	}

    val[0] = ((xax - actual_position[0]) * X_STEPS_PER_mm ) / DIVISORE_STEPS;
    val[1] = ((yax - actual_position[1]) * Y_STEPS_PER_mm ) / DIVISORE_STEPS;

	_DBG( "run_stepper_timer( " );
	_DBD32( val[0] ); _DBG( ", " );
	_DBD32( val[1] ); _DBG( " )\n" );

    run_stepper_timer( val[0], val[1], X_CALIBRATED_MIN_SPEED, Y_CALIBRATED_MIN_SPEED, X_CALIBRATED_MAX_SPEED, Y_CALIBRATED_MAX_SPEED );
	
	_DBG("end\n");

    actual_position[0] = xax;
    actual_position[1] = yax;
}


void calibrate_zero()	//zero calibration, from home, to calibrate the zero position
{
    _DBG("Zero calibration started:\n");
    int zero_steps[2] = {0};
    char pos[128];
    uint32_t cmd_pos = 0;

    while(1)
    {		
        if(cmd_pos >= sizeof(pos) - 1)
        {
            cmd_pos = 0;
            _DBG("Command too long, buffer reset\n");
            continue;
        }

        // Read character
        char c = _DG;
        if(c == 0) continue;

        pos[cmd_pos] = c;

        // Process on newline
        if((c == 0x0d) || (c == 0x0a))
        {
            pos[cmd_pos] = 0;  // Null terminate

            if(strstr(pos, "end") == pos)
            {
                _DBG("Calibration completed\n");
                return;
            }

            // Parse and execute movement
            int vals[2] = {0};
            char *sstr = pos;

            for(int vv = 0; vv < 2; vv++)
            {
                vals[vv] = atoi_8(sstr);
                search_chars_moving_and_skipping(&sstr, &cmd_pos, ' ', ' ');
            }

            // Update total steps and run motors
            for(int i = 0; i < 2; i++)
            {
                zero_steps[i] += vals[i];
            }

            _DBG("Zero steps count: ");
            _DBD32(zero_steps[0]); _DBG(", ");
            _DBD32(zero_steps[1]); _DBG(", ");

			run_stepper_timer( vals[0], vals[1], 1000, 1000, 5000, 5000 );

			cmd_pos = 0;
            continue;
        }

        cmd_pos++;
    }
}

void execute_path( int path[NUMBER_OF_POINTS][2] )		//utilizzata, per zero su parte superiore dello schermo, aggiungo un offset, con go_to_mm, con path_8
{
	_DBG("executing path\n");_DBG("\n");

    for( int i = 0; i < NUMBER_OF_POINTS; i++ )
    {
		_DBG("point to reach: ");
		_PRINT(path[i][0], ", ");
		_PRINT(path[i][1], ", ");
        
        reach_point_mm( path[i][0], path[i][1] );

		//solenoide senza croce, solo un punto
		custom_delay(100);
		if( i >= 0 )
		{
			solenoide_run( 1 );
			custom_delay( SOLENOIDE_DELAY );
			solenoide_run( 0 );
		}
		custom_delay(100);
	}

	//alla fine torna a zero
	reach_point_mm(0, 0);
}

