#include "stepper_library.h"
#include "smu.h"

#define TIMEOUT_TRAFFICO_CANBUS_MS	(2000) // 2 secondi

void _PRINT(int x, const char *m)
{
    _DBD32(x); // Print the integer x in 32-bit decimal format
    _DBG(m);   // Print the string m
}

//sezione azionamento motore stepper
#define ARR_COUNT( a )				( sizeof( a ) / sizeof( a[0] ) )		//per numero elementi di array
#define MAX_NUM_OF_BLOCKS			( ( ARR_COUNT( speed_array )-1 ) * 2 + 1 )		//in pratica accelerazione per salita e per discesa

// Assuming that:

// Timer source is CCLK/4 -> 25 MHz
// Timers prescale is 16 -> 1.562500 MHz
// Stepper motor is configured for half-step mode (400 pulses per round)
// The pulley has a diameter of 11 mm -> circumference 34.6 mm

// With a Timer Match of 7812 -> the timer is 200 Hz 	-> 100 Hz of pulses to the motor -> 0.25 rpm -> 8,65 mm/s
// With a prescaler of 140 -> the timer is 11.161 kHz 	-> 5.580 kHz of pulses to the motor -> 14.0 rpm -> 483 mm/s

#define SPEED_MIN					( speed_array[0] )
#define NUMBER_OF_STEPS_PER_BLOCK	256

#define	SPEED_LEVELS		8
#define K_STEPS_PER_SPEED	500000

//calibrato a 30 V, aumentare tensione per aumentare velocità
//problema sembra essere sulla velocità minima

#define SPEED_MIN_uS_anycubic		10000	//Valore calibrato: 20000	// 250 Hz, importante abbassare la velocità minima, questi sono i valori calibrati, tendenti al basso
#define SPEED_MAX_uS_anycubic		250		//:valore lento	//Valore calibrato: 250   // ?
#define SPEED_MIN_uS_laser			20000		//20000 // :valore lento	//Valore calibrato: 20000 // 250 Hz, importante abbassare la velocità minima, questi sono i valori calibrati, tendenti al basso
#define SPEED_MAX_uS_laser			480		//550			//:valore lento	//Valore calibrato: 250   // ?

int SPEED_MIN_uS = 0;						//20000 // :valore lento	//Valore calibrato: 20000 // 250 Hz, importante abbassare la velocità minima, questi sono i valori calibrati, tendenti al basso
int SPEED_MAX_uS = 0;						// :valore lento	//Valore calibrato: 250   // ?

int speed_array[SPEED_LEVELS] =		//di fatto viene creato dopo, a runtime
{
	// 125000, 26957, 15107, 10494, 8040, 6515, 5477, 4724, 4153, 3706, 3345, 3048, 2800, 2589, 2408, 2250,
	// 7812, 1684, 944, 655, 502, 407, 342, 295, 259, 231, 209, 190, 175, 161, 150, 140,
};

// Numero di steps in cui mantenere la velocitï¿½ N
int steps_array[SPEED_LEVELS - 1] =
{

};

// Minimo numero di step per poter raggiungere la velocitï¿½ N
int steps_integral[SPEED_LEVELS - 1] =		//inizializzato da create-arrays
{

};

void create_arrays()		//crea i tre array
{

	SPEED_MIN_uS = SPEED_MIN_uS_laser;
	SPEED_MAX_uS = SPEED_MAX_uS_laser;

	speed_array[0] = SPEED_MIN_uS;		//definito 20000 in base alla frequenza del clock
	speed_array[SPEED_LEVELS - 1] = SPEED_MAX_uS;

	float last = ( 1 / ( float )speed_array[0] );
	float delta = ( ( 1 / ( float )speed_array[SPEED_LEVELS - 1] ) - ( 1 / ( float )speed_array[0] ) ) / ( SPEED_LEVELS - 1 );

	_DBG( "ARRAYS:\n" );

	// Speed array
	_DBG("Speed: ");

	for( int fd = 1; fd < SPEED_LEVELS - 1; fd++ )
	{
		last += delta;
		speed_array[fd] = 1/last + 0.5;
		_PRINT( speed_array[fd], ", " );
	}
	_DBG( "\n" );

	// Steps array
	_DBG("Steps: ");
	for( int fd = 0; fd < SPEED_LEVELS - 1; fd++ )
	{
		steps_array[fd] = K_STEPS_PER_SPEED / ( float )speed_array[fd] + 0.5;
		_PRINT( steps_array[fd], ", " );
	}
	_DBG( "\n" );

	// Steps integral
	_DBG("Integral: ");
	int sum = 0;
	for( int fd = 0; fd < SPEED_LEVELS - 1; fd++ )
	{
		sum += steps_array[fd];
		steps_integral[fd] = sum*2;
		_PRINT( steps_integral[fd], ", " );
	}
	_DBG( "\n" );
} //di fatto lavora con 3 array

typedef struct
{
	// Axis are 0 x, 1 y, 2 z
	uint8_t axis;
	// Mode is 0 accel, 1 cruise, 2 decel
	uint8_t mode;
	// Speed level
	uint8_t next_speed_level;
	// Is the last block?
	uint8_t stop;
	// Step count expresses as 25 MHz ticks
	uint32_t timer_count;

} st_pacekeeper;

// ( N-1 acceleration + N-1 deceleration + 1 cruise ) for 3 axis
st_pacekeeper pace[ MAX_NUM_OF_BLOCKS * 3 ];
volatile int pace_count;
volatile int pace_index;
static LPC_TIM_TypeDef *const get_timers[] = { LPC_TIM0, LPC_TIM1, LPC_TIM2 };
volatile int irq_count_A, irq_count_B;

typedef struct		//imp. per gli assi
{
	uint32_t tot_steps;
	uint32_t divisor;
	uint32_t remainder;

	uint32_t remaining_steps;
	uint32_t divisor_count;
	int32_t remainder_count;

} st_axis;		//utilizzata da prepare_axis

st_axis xax, yax, zax;

typedef struct
{
	uint32_t accel;
	uint32_t cruise;
	uint32_t decel;

	uint32_t cruise_steps_count;
	uint32_t steps_count;

} st_accel;

st_accel accel;

void prepare_axis( st_axis *ax, uint32_t count, uint32_t max )		//chiamata in run: prepare_axis( &xax, x, count_vec[max_ax] );
{
	ax->tot_steps = count;		//count di fatto è il numeor di step
	ax->divisor = max / count;		//max è il numero di step dell'asse con più steps
	ax->remainder = max % count;

	ax->remaining_steps = count;		//poi diminuisce
	ax->divisor_count = ( ax->divisor + 1 ) / 2;
	ax->remainder_count = - ( int32_t )count / 2;		//con segno

	//_PRINT( ax->remainder_count, " ax->remainder_count\n" );

	/*_PRINT( max, " max, " );

	_PRINT( ax->tot_steps, " tot, " );
	_PRINT( ax->divisor, " div, " );
	_PRINT( ax->remainder, " rem, " );

	_PRINT( ax->remaining_steps, " rs, " );
	_PRINT( ax->divisor_count, " dc, " );
	_PRINT( ax->remainder_count, " rc\n" );*/
}

void run( int32_t x, int32_t y, int32_t z )		//run principale, utilizzata
{
	x = -x;
	y = -y;

	int x_dir = 0;
	int y_dir = 0;
	int z_dir = 0;

	if( x < 0 )
		{
			x = -x;
			x_dir = 1;
		}
	if( y < 0 )
		{
			y = -y;
			y_dir = 1;
		}
	if( z < 0 )
		{
			z = -z;
			z_dir = 1;
		}

	x *= 2;
	y *= 2;
	z *= 2;

	// Set the directions
	if( x_dir == 0 ) GPIO_SetValue( 1, 1<<28 );		//sistema le direzioni, poi lavora in modo assoluto
	else GPIO_ClearValue( 1, 1<<28 );

	if( y_dir != 0 ) GPIO_SetValue( 1, 1<<22 );
	else GPIO_ClearValue( 1, 1<<22 );

	if( z_dir != 0 ) GPIO_SetValue( 4, 1<<28 );
	else GPIO_ClearValue( 4, 1<<28 );

	// Prepare steps counters
	uint32_t count_vec[] = { x, y, z, };		//vettore con gli steps da raggiungere
	int max_ax;		//di tutti, prende quello maggiore, determina il tempo minimo

	if( x > y )  //asse con più step da realizzare 0, 1, 2, in valore assoluto
		{
			if( x > z ) max_ax = 0;
			else max_ax = 2;
		}
	else if( y > z ) max_ax = 1;
	else max_ax = 2;

	prepare_axis( &xax, x, count_vec[max_ax] );
	prepare_axis( &yax, y, count_vec[max_ax] );
	prepare_axis( &zax, z, count_vec[max_ax] );		//crea le strutture, tutte sulla base degli steps massimi	

	// Calculate acceleration
	if( count_vec[max_ax] <= ( uint32_t )steps_integral[0] )
		{
			accel.accel = 0;
			accel.cruise = 1;
			accel.decel = 0;

			accel.cruise_steps_count = count_vec[max_ax];
		}
	else
		{
			accel.accel = 0;
			for( int yy = ARR_COUNT( steps_integral ) - 1; yy >= 0; yy-- )
			{
				if( count_vec[max_ax] > ( uint32_t )steps_integral[yy] )
				{
					accel.accel = yy + 1;
					break;
				}
			}

			accel.cruise = 1;
			accel.decel = accel.accel;

			// The array index should be safe because of the if/else condition
			accel.cruise_steps_count = count_vec[max_ax] - steps_integral[accel.accel - 1];
		}
		accel.steps_count = 0;
}

int single_step( st_axis *ax )		//chiamata in stepperçtask, implementa di fatto la divisione tra interi
{
	if( ax->divisor_count == 1 )
	{
		ax->remainder_count += ax->remainder;
		if( ax->remainder_count > 0 )
		{
			ax->remainder_count -= ax->tot_steps;
			ax->divisor_count = ax->divisor + 1;
		}
		else ax->divisor_count = ax->divisor;

		ax->remaining_steps--;
		return 1;
	}
	else ax->divisor_count--;

	return 0;
}

//stepper task
volatile int is_running = 0;

void Stepper_Task( void *pvParameters )
{
	( void )pvParameters;

	/* Init
	Timer_Set();

	// motor_start( ( uint32_t (*)[2] )
	motor_start( ( uint32_t[][2] ){
		{ 21*2, 0, },
		{ 0, 0, },
		{ 0, 0, },
	});*/

	// Setup pins

	PINSEL_CFG_Type PinCfg;

	PinCfg.Funcnum = PINSEL_FUNC_0;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;

	PinCfg.Portnum = PINSEL_PORT_1;
	PinCfg.Pinnum = PINSEL_PIN_26;
	PINSEL_ConfigPin( &PinCfg );
	GPIO_SetDir( 1, 1<<26, 1 ); //Solenoide, MOS2, porta 1, pin 26
	GPIO_ClearValue( 1, 1<<26 );

	PinCfg.Portnum = PINSEL_PORT_1;
	PinCfg.Pinnum = PINSEL_PIN_28;
	PINSEL_ConfigPin( &PinCfg );
	GPIO_SetDir( 1, 1<<28, 1 ); // MOS 3	dir x
	GPIO_ClearValue( 1, 1<<28 );

	PinCfg.Portnum = PINSEL_PORT_1;
	PinCfg.Pinnum = PINSEL_PIN_29;
	PINSEL_ConfigPin( &PinCfg );
	GPIO_SetDir( 1, 1<<29, 1 ); // MOS 4	pulse x
	GPIO_ClearValue( 1, 1<<29 );

	PinCfg.Portnum = PINSEL_PORT_1;
	PinCfg.Pinnum = PINSEL_PIN_22;
	PINSEL_ConfigPin( &PinCfg );
	GPIO_SetDir( 1, 1<<22, 1 ); // MOS 5	dir y
	GPIO_ClearValue( 1, 1<<22 );

	PinCfg.Portnum = PINSEL_PORT_1;
	PinCfg.Pinnum = PINSEL_PIN_25;
	PINSEL_ConfigPin( &PinCfg );
	GPIO_SetDir( 1, 1<<25, 1 ); // MOS 6	pulse y
	GPIO_ClearValue( 1, 1<<25 );

	PinCfg.Portnum = PINSEL_PORT_4;
	PinCfg.Pinnum = PINSEL_PIN_28;
	PINSEL_ConfigPin( &PinCfg );
	GPIO_SetDir( 4, 1<<28, 1 ); // MOS 7	dir z
	GPIO_ClearValue( 4, 1<<28 );

	PinCfg.Portnum = PINSEL_PORT_4;
	PinCfg.Pinnum = PINSEL_PIN_29;
	PINSEL_ConfigPin( &PinCfg );
	GPIO_SetDir( 4, 1<<29, 1 ); // MOS 8  pulse z
	GPIO_ClearValue( 4, 1<<29 );

	// Setup CH_SEL_Ax and ENABLE

	PinCfg.Portnum = PINSEL_PORT_3;
	PinCfg.Pinnum = PINSEL_PIN_25;
	PINSEL_ConfigPin( &PinCfg );
	GPIO_SetDir( 3, 1<<25, 1 ); // CH_SEL_A0
	GPIO_ClearValue( 3, 1<<25 );

	PinCfg.Portnum = PINSEL_PORT_0;
	PinCfg.Pinnum = PINSEL_PIN_30;
	PINSEL_ConfigPin( &PinCfg );
	GPIO_SetDir( 0, 1<<30, 1 ); // CH_SEL_A1
	GPIO_ClearValue( 0, 1<<30 );

	PinCfg.Portnum = PINSEL_PORT_1;
	PinCfg.Pinnum = PINSEL_PIN_19;
	PINSEL_ConfigPin( &PinCfg );
	GPIO_SetDir( 1, 1<<19, 1 ); // CH_SEL_A2
	GPIO_ClearValue( 1, 1<<19 );

	PinCfg.Portnum = PINSEL_PORT_0;
	PinCfg.Pinnum = PINSEL_PIN_29;
	PINSEL_ConfigPin( &PinCfg );
	GPIO_SetDir( 0, 1<<29, 1 ); // CH_SEL_ENABLE
	GPIO_SetValue( 0, 1<<29 );

	// Setup ingresso DI

	PinCfg.Portnum = PINSEL_PORT_0;
	PinCfg.Pinnum = PINSEL_PIN_10;
	PINSEL_ConfigPin( &PinCfg );
	GPIO_SetDir( 0, 1<<10, 0 ); // DI porta che legge tutti gli ingressi, sono selezionati dal mux

	// Setup FUNC_SEL_Ax and ENABLE

	PinCfg.Portnum = PINSEL_PORT_1;
	PinCfg.Pinnum = PINSEL_PIN_10;
	PINSEL_ConfigPin( &PinCfg );
	GPIO_SetDir( 1, 1<<10, 1 ); // FUNC_SEL_A0
	GPIO_ClearValue( 1, 1<<10 );

	PinCfg.Portnum = PINSEL_PORT_1;
	PinCfg.Pinnum = PINSEL_PIN_8;
	PINSEL_ConfigPin( &PinCfg );
	GPIO_SetDir( 1, 1<<8, 1 ); // FUNC_SEL_A1
	GPIO_ClearValue( 1, 1<<8 );

	PinCfg.Portnum = PINSEL_PORT_1;
	PinCfg.Pinnum = PINSEL_PIN_9;
	PINSEL_ConfigPin( &PinCfg );
	GPIO_SetDir( 1, 1<<9, 1 ); // FUNC_SEL_ENABLE
	GPIO_SetValue( 1, 1<<9 );

	// Create the speed arrays
	create_arrays();		//crea tre array, chiamata una volta
	_DBG("Array created, start while(1)\n"); //dbg


	// Infinite loop

	int st_1, st_2;
	st_1 = st_2 = 0;



	while( 1 ) //agisce mentre i motori stanno andando, continuamente chiamata da rtos
	{

		int speed = 0;

		//portDISABLE_INTERRUPTS();		//mi servono per i timers in questo caso

		while( is_running )		//durante il run, aggiorna lo stato di is_running
		{
			is_running = 3;

			// Activate (t_ON)

			if( xax.remaining_steps > 0 ) //attivazione degli impulsi stepper
			{
				if( single_step( &xax ) ) GPIO_SetValue( 1, 1<<29 );
			}
			else is_running--;

			if( yax.remaining_steps > 0 )
			{
				if( single_step( &yax ) ) GPIO_SetValue( 1, 1<<25 );
			}
			else is_running--;

			if( zax.remaining_steps > 0 )
			{
				if( single_step( &zax ) ) GPIO_SetValue( 4, 1<<29 );
			}
			else is_running--;

			if( accel.accel > 0 )
			{
				if( accel.steps_count >= ( uint32_t )steps_array[speed] )
				{
					speed++;
					accel.accel--;
					accel.steps_count = 0;
				}
				else accel.steps_count++;
			}
			else if( accel.cruise )
			{
				if( accel.steps_count >= accel.cruise_steps_count )
				{
					accel.cruise = 0;
					accel.steps_count = 0;
				}
				else accel.steps_count++;
			}
			else if( accel.decel > 0 )
			{
				if( accel.steps_count >= ( uint32_t )steps_array[speed - 1] )
				{
					speed--;
					accel.decel--;
					accel.steps_count = 0;
				}
				else accel.steps_count++;
			}

			// Set the IN_1 mux
			GPIO_ClearValue( 3, 1<<25 );
			GPIO_ClearValue( 0, 1<<30 );
			GPIO_ClearValue( 1, 1<<19 );

			// Delay
			for( volatile int del = 0; del < speed_array[speed]; del++ );

			// Read IN_1
			if( ( GPIO_ReadValue( 0 ) & ( 1<<10 ) ) == 0 )
			{
				if( st_1 )
				{
					//_DBG( "Zero_1\n" );
					st_1 = 0;
				}
			}
			else st_1 = 1;

			// Deactivate (t_OFF)
			GPIO_ClearValue( 1, 1<<29 );
			GPIO_ClearValue( 1, 1<<25 );
			GPIO_ClearValue( 4, 1<<29 );

			// Set the IN_2 mux
			GPIO_SetValue( 3, 1<<25 );
			GPIO_ClearValue( 0, 1<<30 );
			GPIO_ClearValue( 1, 1<<19 );

			// Delay
			for( volatile int del = 0; del < speed_array[speed]; del++ );

			// Read IN_2
			if( ( GPIO_ReadValue( 0 ) & ( 1<<10 ) ) == 0 )
			{
				if( st_2 )
				{
					//_DBG( "Zero_2\n" );
					st_2 = 0;
				}
			}
			else st_2 = 1;
		}

		//portENABLE_INTERRUPTS();

		vTaskDelay( 100 );
	}
}










