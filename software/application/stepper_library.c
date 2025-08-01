// First version of the stepper library
// Initially used speed arrays to avoid divisions at runtime
// The new version uses timers instead

#include "stepper_library.h"
#include "smu.h"

#define TIMEOUT_TRAFFICO_CANBUS_MS (2000) // 2 seconds

void _PRINT(int x, const char *m)
{
    _DBD32(x); // Print integer x (32-bit decimal)
    _DBG(m);   // Print message string
}

// Stepper motor drive section
#define ARR_COUNT(a) (sizeof(a) / sizeof(a[0]))               // Macro to count array elements
#define MAX_NUM_OF_BLOCKS ((ARR_COUNT(speed_array)-1)*2 + 1) // Acceleration and deceleration blocks

// Timer assumptions:
// CCLK/4 -> 25 MHz, prescale -> 1.5625 MHz
// Half-step mode: 400 pulses/rotation
// Pulley diameter: 11 mm -> 34.6 mm circumference
// Timer Match 7812 -> 200 Hz -> 100 Hz pulses -> 0.25 RPM -> ~8.65 mm/s
// Prescaler 140 -> ~11.2 kHz -> ~5.58 kHz pulses -> 14.0 RPM -> ~483 mm/s

#define SPEED_MIN (speed_array[0])
#define NUMBER_OF_STEPS_PER_BLOCK 256
#define SPEED_LEVELS 8
#define K_STEPS_PER_SPEED 500000

// Calibrated at 30V. Increase voltage to increase speed.
#define SPEED_MIN_uS_anycubic 10000
#define SPEED_MAX_uS_anycubic 250
#define SPEED_MIN_uS_laser 20000
#define SPEED_MAX_uS_laser 480

int SPEED_MIN_uS = 0;
int SPEED_MAX_uS = 0;

int speed_array[SPEED_LEVELS] = { };
int steps_array[SPEED_LEVELS - 1] = { };
int steps_integral[SPEED_LEVELS - 1] = { };

void create_arrays()
{
    SPEED_MIN_uS = SPEED_MIN_uS_laser;
    SPEED_MAX_uS = SPEED_MAX_uS_laser;

    speed_array[0] = SPEED_MIN_uS;
    speed_array[SPEED_LEVELS - 1] = SPEED_MAX_uS;

    float last = (1 / (float)speed_array[0]);
    float delta = ((1 / (float)speed_array[SPEED_LEVELS - 1]) - last) / (SPEED_LEVELS - 1);

    _DBG("ARRAYS:\n");
    _DBG("Speed: ");

    for (int fd = 1; fd < SPEED_LEVELS - 1; fd++) {
        last += delta;
        speed_array[fd] = 1 / last + 0.5;
        _PRINT(speed_array[fd], ", ");
    }
    _DBG("\n");

    _DBG("Steps: ");
    for (int fd = 0; fd < SPEED_LEVELS - 1; fd++) {
        steps_array[fd] = K_STEPS_PER_SPEED / (float)speed_array[fd] + 0.5;
        _PRINT(steps_array[fd], ", ");
    }
    _DBG("\n");

    _DBG("Integral: ");
    int sum = 0;
    for (int fd = 0; fd < SPEED_LEVELS - 1; fd++) {
        sum += steps_array[fd];
        steps_integral[fd] = sum * 2;
        _PRINT(steps_integral[fd], ", ");
    }
    _DBG("\n");
}

// Structs for axis movement

typedef struct {
    uint8_t axis;       // 0=x, 1=y, 2=z
    uint8_t mode;       // 0=accel, 1=cruise, 2=decel
    uint8_t next_speed_level;
    uint8_t stop;
    uint32_t timer_count;
} st_pacekeeper;

st_pacekeeper pace[MAX_NUM_OF_BLOCKS * 3];
volatile int pace_count;
volatile int pace_index;
static LPC_TIM_TypeDef *const get_timers[] = { LPC_TIM0, LPC_TIM1, LPC_TIM2 };
volatile int irq_count_A, irq_count_B;

typedef struct {
    uint32_t tot_steps;
    uint32_t divisor;
    uint32_t remainder;
    uint32_t remaining_steps;
    uint32_t divisor_count;
    int32_t remainder_count;
} st_axis;

st_axis xax, yax, zax;

typedef struct {
    uint32_t accel;
    uint32_t cruise;
    uint32_t decel;
    uint32_t cruise_steps_count;
    uint32_t steps_count;
} st_accel;

st_accel accel;

void prepare_axis(st_axis *ax, uint32_t count, uint32_t max)
{
    ax->tot_steps = count;
    ax->divisor = max / count;
    ax->remainder = max % count;
    ax->remaining_steps = count;
    ax->divisor_count = (ax->divisor + 1) / 2;
    ax->remainder_count = -(int32_t)count / 2;
}

void run(int32_t x, int32_t y, int32_t z)
{
    // Reverse directions
    x = -x;
    y = -y;

    int x_dir = 0, y_dir = 0, z_dir = 0;
    if (x < 0) { x = -x; x_dir = 1; }
    if (y < 0) { y = -y; y_dir = 1; }
    if (z < 0) { z = -z; z_dir = 1; }

    x *= 2;
    y *= 2;
    z *= 2;

    // Set directions (X, Y, Z)
    if (x_dir == 0) GPIO_SetValue(1, 1<<28);
    else GPIO_ClearValue(1, 1<<28);

    if (y_dir != 0) GPIO_SetValue(1, 1<<22);
    else GPIO_ClearValue(1, 1<<22);

    if (z_dir != 0) GPIO_SetValue(4, 1<<28);
    else GPIO_ClearValue(4, 1<<28);

    uint32_t count_vec[] = { x, y, z };
    int max_ax;
    if (x > y) max_ax = (x > z) ? 0 : 2;
    else max_ax = (y > z) ? 1 : 2;

    prepare_axis(&xax, x, count_vec[max_ax]);
    prepare_axis(&yax, y, count_vec[max_ax]);
    prepare_axis(&zax, z, count_vec[max_ax]);

    if (count_vec[max_ax] <= (uint32_t)steps_integral[0]) {
        accel.accel = 0;
        accel.cruise = 1;
        accel.decel = 0;
        accel.cruise_steps_count = count_vec[max_ax];
    } else {
        accel.accel = 0;
        for (int yy = ARR_COUNT(steps_integral) - 1; yy >= 0; yy--) {
            if (count_vec[max_ax] > (uint32_t)steps_integral[yy]) {
                accel.accel = yy + 1;
                break;
            }
        }
        accel.cruise = 1;
        accel.decel = accel.accel;
        accel.cruise_steps_count = count_vec[max_ax] - steps_integral[accel.accel - 1];
    }
    accel.steps_count = 0;
}

int single_step(st_axis *ax)
{
    if (ax->divisor_count == 1) {
        ax->remainder_count += ax->remainder;
        if (ax->remainder_count > 0) {
            ax->remainder_count -= ax->tot_steps;
            ax->divisor_count = ax->divisor + 1;
        } else ax->divisor_count = ax->divisor;
        ax->remaining_steps--;
        return 1;
    } else {
        ax->divisor_count--;
    }
    return 0;
}

volatile int is_running = 0;

void Stepper_Task(void *pvParameters)
{
    (void)pvParameters;
    // [Pin configuration and logic here - skipped for brevity]
    create_arrays();
    _DBG("Array created, start while(1)\n");

    int st_1 = 0, st_2 = 0;

    while (1) {
        int speed = 0;
        while (is_running) {
            is_running = 3;

            if (xax.remaining_steps > 0) {
                if (single_step(&xax)) GPIO_SetValue(1, 1<<29);
            } else is_running--;

            if (yax.remaining_steps > 0) {
                if (single_step(&yax)) GPIO_SetValue(1, 1<<25);
            } else is_running--;

            if (zax.remaining_steps > 0) {
                if (single_step(&zax)) GPIO_SetValue(4, 1<<29);
            } else is_running--;

            // Handle acceleration, cruise, deceleration
            if (accel.accel > 0) {
                if (accel.steps_count >= (uint32_t)steps_array[speed]) {
                    speed++;
                    accel.accel--;
                    accel.steps_count = 0;
                } else accel.steps_count++;
            } else if (accel.cruise) {
                if (accel.steps_count >= accel.cruise_steps_count) {
                    accel.cruise = 0;
                    accel.steps_count = 0;
                } else accel.steps_count++;
            } else if (accel.decel > 0) {
                if (accel.steps_count >= (uint32_t)steps_array[speed - 1]) {
                    speed--;
                    accel.decel--;
                    accel.steps_count = 0;
                } else accel.steps_count++;
            }

            // Mux selection and delay
            GPIO_ClearValue(3, 1<<25);
            GPIO_ClearValue(0, 1<<30);
            GPIO_ClearValue(1, 1<<19);
            for (volatile int del = 0; del < speed_array[speed]; del++);

            if ((GPIO_ReadValue(0) & (1<<10)) == 0) {
                if (st_1) st_1 = 0;
            } else st_1 = 1;

            GPIO_ClearValue(1, 1<<29);
            GPIO_ClearValue(1, 1<<25);
            GPIO_ClearValue(4, 1<<29);

            GPIO_SetValue(3, 1<<25);
            GPIO_ClearValue(0, 1<<30);
            GPIO_ClearValue(1, 1<<19);
            for (volatile int del = 0; del < speed_array[speed]; del++);

            if ((GPIO_ReadValue(0) & (1<<10)) == 0) {
                if (st_2) st_2 = 0;
            } else st_2 = 1;
        }

        vTaskDelay(100);
    }
}
