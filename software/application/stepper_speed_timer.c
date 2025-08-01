#include "stepper_speed_timer.h"

//funzione velocità steppers
//altre funzioni utili, tipo per la calibrazione o lo zeroo che utilizzano queste funzioni, includeranno questa libreria
//obiettivo: impostare velocità come steps al secondo
//valutare mm al secondo
//riabilitare gli interrupt

#include "LPC17xx.h"

typedef struct {
    int total_steps;
    int previous_count;
    int remaining_steps;
    int current_speed;       // steps per second, current speed = minumun speed at the beginning
    int delta_speed;
    int min_speed;           //steps per second
    int max_speed;           //step per second
    int accel_steps;         //steps taken during acceleration
    volatile int phase;               //imp. il volatile, impedisce ottimizzazioni sopratutto in questa che è una variabile aggiornata da una isr! //0=accel, 1=cruise, 2=decel
    int update_speed_flag;
} ax_state;

//creo i due assi
ax_state x_ax;
ax_state y_ax;

int done_x = 0;
int done_y = 0;

int tim0_update = 0;
int tim1_update = 0;

void set_directions(int xs, int ys) {		
    if(xs > 0)
    {
        GPIO_SetValue(X_PORT, X_DIR_PIN);    // x dir

    }
    else
    {
        GPIO_ClearValue(X_PORT, X_DIR_PIN);
    }
    if(ys < 0)
        GPIO_SetValue(Y_PORT, Y_DIR_PIN);    // y dir
    else
        GPIO_ClearValue(Y_PORT, Y_DIR_PIN);
}

void init_timer0(uint32_t frequency_hz) {           //frequenza con cui genera interrupts, quindi steps per secondo
    uint32_t pclk = SystemCoreClock / 4;            //Tipico per LPC1768, 100 / 4 = 25 MHz
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

void run_stepper_timer(int xs, int ys, int x_speed_min, int y_speed_min, int x_speed_max, int y_speed_max) {       //speed_min, max ecc. could all be set as constant, but this way it is possible to calibrate
    
    set_directions(xs, ys);

    //two axis - X AXIS SETUP
    x_ax.total_steps = abs(xs);
    x_ax.remaining_steps = abs(xs);
    x_ax.current_speed = x_speed_min;
    x_ax.min_speed = x_speed_min;
    x_ax.max_speed = x_speed_max;
    x_ax.delta_speed = X_DELTA_SPEED;
    x_ax.previous_count = 0;
    x_ax.phase = 0;
    x_ax.accel_steps = 0;
    x_ax.update_speed_flag = 0;

    // Y AXIS SETUP
    y_ax.total_steps = abs(ys);
    y_ax.remaining_steps = abs(ys);
    y_ax.current_speed = y_speed_min;
    y_ax.min_speed = y_speed_min;
    y_ax.max_speed = y_speed_max;
    y_ax.delta_speed = Y_DELTA_SPEED;
    y_ax.previous_count = 0;
    y_ax.phase = 0;     //0 = accelerazione, 1 = cruise, 2 = decelerazione
    y_ax.accel_steps = 0;
    y_ax.update_speed_flag = 0;

    //calcoli per fase di accelerazione e decelerazione X AXIS
    int x_acceleration_steps = (x_ax.max_speed - x_ax.min_speed)/X_DELTA_SPEED;
    
    //caso accelerazione non completa per X     
    if( x_ax.total_steps <= (x_acceleration_steps * 2) )
    {
        x_ax.accel_steps = x_ax.total_steps / 2;        //importante, se non arrivo alla velocità massima
        //ma allora posso aggiornare direttamente anche la v max
        x_ax.max_speed = x_ax.min_speed + (X_DELTA_SPEED * x_ax.accel_steps);      //FIXED: correct calculation
    }
    else
    {
        x_ax.accel_steps = x_acceleration_steps;
    }

    //calcoli per fase di accelerazione e decelerazione Y AXIS
    int y_acceleration_steps = (y_ax.max_speed - y_ax.min_speed)/Y_DELTA_SPEED;
    
    //caso accelerazione non completa per Y     
    if( y_ax.total_steps <= (y_acceleration_steps * 2) )
    {
        y_ax.accel_steps = y_ax.total_steps / 2;        //importante, se non arrivo alla velocità massima
        //ma allora posso aggiornare direttamente anche la v max, poi la uso come condizione
        y_ax.max_speed = y_ax.min_speed + (Y_DELTA_SPEED * y_ax.accel_steps);      //FIXED: correct calculation
    }
    else
    {
        y_ax.accel_steps = y_acceleration_steps;
    }

    /*
    //print parametri calcolati prima di partire
    _DBG("x_ax.max_speed = ");_DBD32( x_speed_max );_DBG("   x_ax.min_speed = ");_DBD32( x_ax.min_speed );_DBG("   x_ax.total_steps = ");_DBD32( x_ax.total_steps );_DBG("\n");
    _DBG("x_acceleration_steps = ");_DBD32( x_acceleration_steps );_DBG("   x_ax.max_speed = ");_DBD32( x_ax.max_speed );_DBG("\n");
    _DBG("fase iniziale =  ");_DBD32( x_ax.phase );_DBG("\n");
    */

    tim0_update = x_ax.current_speed;       
    tim1_update = y_ax.current_speed;

    //imposta steps al secondo, è una frequenza in Hz. in pratica se voglio fare una rotazione al secondo sono 800 Hz, non dovrei avere problemi di velocità minima, basta una frequenza molto bassa
    init_timer0( x_ax.current_speed );     
    init_timer1( y_ax.current_speed );


    //_DBG("Start while in run\n");

    while( x_ax.phase != 4 || y_ax.phase != 4 )
    {
        //non ottimizzato grazie a volatile
    }
        
    //_DBG("End while in run\n");
}

// ASSE X
void TIMER0_IRQHandler(void) {     
    //chiamata isr del timer
    //NVIC_DisableIRQ(TIMER0_IRQn);

    if (LPC_TIM0->IR & 1) 
    {
        LPC_TIM0->IR = 1;  // Clear interrupt
        
        if ( x_ax.phase != 4 ) 
        {
            GPIO_SetValue(X_PORT, X_PULSE_PIN);  // Impulso HIGH
            for( volatile int i = 0; i < 50; i++ );            // Piccolo delay per durata HIGH → anche 5 µs bastano
            GPIO_ClearValue(X_PORT, X_PULSE_PIN);// Impulso LOW
            x_ax.remaining_steps--;
 
            //acceleration
            
            if( x_ax.current_speed < x_ax.max_speed && x_ax.phase == 0 /*imp., non basta velocità minore di quella massima ma serve ance essere in fase di accelerazione*/ )       //ogni step aumento la velocità fino alla massima, quindi per capire il numero di step dell'accelerazione mi basta fare (max_speed - min_speed)/ delta_speed
            {
                x_ax.current_speed = x_ax.current_speed + X_DELTA_SPEED;
                tim0_update = x_ax.current_speed;
            }
            else if( x_ax.phase == 0 )
            {
                //_DBG("Start x cruise\n"); //evitare tali print che inseriscono scatti nell'esecuzione della rotazione, molto evidenti
                x_ax.phase = 1;     //passo a cruise
            }

            //cruise
            if( x_ax.phase == 1 && x_ax.remaining_steps <= x_ax.accel_steps )  // FIXED: == instead of =
            {
                //_DBG("Start x deceleration\n");
                x_ax.phase = 2;
            }

            //deceleration
            if(  x_ax.current_speed > x_ax.min_speed && x_ax.phase == 2 )       //fase di decelerazione
            {
                x_ax.current_speed -= X_DELTA_SPEED;
                tim0_update = x_ax.current_speed;
            }

            //stato 4, ovvero asse completo
            if( x_ax.remaining_steps <= 0 ) 
            {
                x_ax.phase = 4;
                //x_end_flag = 1;
                //_DBG("Finish x_ax\n");
                //_DBG("x phase ");_DBD32(x_ax.phase);_DBG("---\n");
                //_DBG("y_phase ");_DBD32(y_ax.phase);_DBG("---\n");
                //LPC_TIM0->TCR = 0;  //Disabilita timer
            }
        }
    }

    if (tim0_update < 1) tim0_update = 1;

    LPC_TIM0->MR0 = (SystemCoreClock / 4) / tim0_update ;

    NVIC_ClearPendingIRQ(TIMER0_IRQn);  // Pulizia eventuali pending
    //NVIC_EnableIRQ(TIMER0_IRQn);
}

// ASSE Y
void TIMER1_IRQHandler(void) {
    if (LPC_TIM1->IR & 1) {
        LPC_TIM1->IR = 1;  // Clear interrupt

        if (y_ax.phase != 4) {
            GPIO_SetValue(Y_PORT, Y_PULSE_PIN);  // Impulso HIGH
            for (volatile int i = 0; i < 50; i++); // piccolo delay
            GPIO_ClearValue(Y_PORT, Y_PULSE_PIN);  // Impulso LOW
            y_ax.remaining_steps--;

            // Accelerazione
            if (y_ax.current_speed < y_ax.max_speed && y_ax.phase == 0) {
                y_ax.current_speed += Y_DELTA_SPEED;
                tim1_update = y_ax.current_speed;
            } else if (y_ax.phase == 0) {
                y_ax.phase = 1; // cruise
            }

            // Cruise → decelerazione
            if (y_ax.phase == 1 && y_ax.remaining_steps <= y_ax.accel_steps) {
                y_ax.phase = 2;
            }

            // Decelerazione
            if (y_ax.current_speed > y_ax.min_speed && y_ax.phase == 2) {
                y_ax.current_speed -= Y_DELTA_SPEED;
                tim1_update = y_ax.current_speed;
            }

            // Fine movimento
            if (y_ax.remaining_steps <= 0) {
                y_ax.phase = 4;
                //_DBG("Finish y_ax\n");
                //_DBG("x phase "); _DBD32(x_ax.phase); _DBG("\n");
                //_DBG("y phase "); _DBD32(y_ax.phase); _DBG("\n");
                // LPC_TIM1->TCR = 0;  // Disabilita timer, se vuoi
            }
        }
    }

    if (tim1_update < 1) tim1_update = 1;
    LPC_TIM1->MR0 = (SystemCoreClock / 4) / tim1_update;

    NVIC_ClearPendingIRQ(TIMER1_IRQn);
}
