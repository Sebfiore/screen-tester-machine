#include "routine_helpers_library.h"
#include "stepper_speed_timer.h"

// Functions that utilize the stepper library and assist the main routine

// Solenoid control: 1 = ON, 0 = OFF
void solenoide_run(int on) {
	if (on)
		GPIO_SetValue(SOLENOIDE_PORT, SOLENOIDE_PIN);
	else
		GPIO_ClearValue(SOLENOIDE_PORT, SOLENOIDE_PIN);
}

// Delay implementation
#define CYCLES 7550  // Clock cycles per millisecond

void custom_delay(int milliseconds) {
	for (volatile int i = 0; i < milliseconds * CYCLES; i++);
}

// INPUTS

// Array to store the state of 8 digital inputs (0 = switch open)
int in_status[8] = {0};

// Configure MUX selector pins (A0, A1, A2) for a given channel (0–7)
void set_mux_channel(int channel) {
	if (channel & 0x01)
		GPIO_SetValue(3, CH_SEL_A0);
	else
		GPIO_ClearValue(3, CH_SEL_A0);

	if (channel & 0x02)
		GPIO_SetValue(0, CH_SEL_A1);
	else
		GPIO_ClearValue(0, CH_SEL_A1);

	if (channel & 0x04)
		GPIO_SetValue(1, CH_SEL_A2);
	else
		GPIO_ClearValue(1, CH_SEL_A2);
}

// Read the digital input from MUX
int read_mux_input() {
	uint32_t port_value = GPIO_ReadValue(0);
	return !!(port_value & DI_PIN);  // Double negation to ensure 0 or 1
}

// Read a single MUX input (0 = ON, 1 = OFF due to inverted logic)
int read_single_input(int channel) {
	GPIO_SetValue(0, CH_SEL_ENABLE);  // Enable MUX
	set_mux_channel(channel);         // Select input channel

	for (volatile int del = 0; del < 1000; del++);  // Allow signal to stabilize

	int result = (read_mux_input() == 1) ? 0 : 1;  // Inverted logic
	GPIO_ClearValue(0, CH_SEL_ENABLE);  // Disable MUX

	return result;
}

// Read all MUX inputs and update global input status array
void read_all_inputs() {
	GPIO_SetValue(0, CH_SEL_ENABLE);  // Enable MUX

	for (int i = 0; i < 8; i++) {
		set_mux_channel(i);
		for (volatile int j = 0; j < 100; j++);  // Stabilization delay

		in_status[i] = (read_mux_input() == 1) ? 0 : 1;
	}

	GPIO_ClearValue(0, CH_SEL_ENABLE);  // Disable MUX
}

// CALIBRATION

// Move both axes toward home using constant speed until switches are hit
void constant_speed_calibration() {
	_DBG("Calibration\n");

	int xax_is_home = 0;
	int yax_is_home = 0;

	// Set directions: X ←, Y ↑
	GPIO_ClearValue(X_PORT, X_DIR_PIN);
	GPIO_SetValue(Y_PORT, Y_DIR_PIN);

	while (!xax_is_home || !yax_is_home) {
		GPIO_ClearValue(X_PORT, X_PULSE_PIN);
		GPIO_ClearValue(Y_PORT, Y_PULSE_PIN);

		for (volatile int del = 0; del < CALIBRATION_SPEED; del++);

		if (read_single_input(XAX_SWITCH) && !xax_is_home)
			GPIO_SetValue(1, 1 << 29);  // X PULSE
		else if (!read_single_input(XAX_SWITCH))
			xax_is_home = 1;

		if (read_single_input(YAX_SWITCH) && !yax_is_home)
			GPIO_SetValue(1, 1 << 25);  // Y PULSE
		else if (!read_single_input(YAX_SWITCH))
			yax_is_home = 1;

		for (volatile int del = 0; del < CALIBRATION_SPEED; del++);
	}

	_DBG("Calibration finished\n");
}

// Move both axes to the defined zero position
void reach_zero(int x_zero, int y_zero) {
	_DBG("Going to zero\n");

	run_stepper_timer(x_zero, y_zero, 1000, 1000, 5000, 5000);

	_DBG("Zero reached\n");
}

// PATH HANDLING

int actual_position[2] = {0, 0};  // Current (x, y) in mm

// Move from current position to target (in mm)
void reach_point_mm(int xax, int yax) {
	int val[2] = {0};

	_DBG("Actual position: ");
	_DBD32(actual_position[0]); _DBG(", ");
	_DBD32(actual_position[1]); _DBG("\n");

	_DBG("Target point: ");
	_DBD32(xax); _DBG(", ");
	_DBD32(yax); _DBG("\n");

	if (xax == actual_position[0] && yax == actual_position[1]) {
		_DBG("Already at target position\n");
		return;
	}

	val[0] = ((xax - actual_position[0]) * X_STEPS_PER_mm) / DIVISORE_STEPS;
	val[1] = ((yax - actual_position[1]) * Y_STEPS_PER_mm) / DIVISORE_STEPS;

	_DBG("Calling run_stepper_timer with: ");
	_DBD32(val[0]); _DBG(", ");
	_DBD32(val[1]); _DBG("\n");

	run_stepper_timer(val[0], val[1], X_CALIBRATED_MIN_SPEED, Y_CALIBRATED_MIN_SPEED, X_CALIBRATED_MAX_SPEED, Y_CALIBRATED_MAX_SPEED);

	_DBG("End of movement\n");

	actual_position[0] = xax;
	actual_position[1] = yax;
}

// Zero calibration: accepts relative moves over UART to define new zero
void calibrate_zero() {
	_DBG("Zero calibration started:\n");

	int zero_steps[2] = {0};
	char pos[128];
	uint32_t cmd_pos = 0;

	while (1) {
		if (cmd_pos >= sizeof(pos) - 1) {
			cmd_pos = 0;
			_DBG("Command too long, buffer reset\n");
			continue;
		}

		char c = _DG;
		if (c == 0) continue;

		pos[cmd_pos] = c;

		// If command ends with newline
		if ((c == 0x0d) || (c == 0x0a)) {
			pos[cmd_pos] = 0;

			if (strstr(pos, "end") == pos) {
				_DBG("Calibration completed\n");
				return;
			}

			int vals[2] = {0};
			char *sstr = pos;

			for (int vv = 0; vv < 2; vv++) {
				vals[vv] = atoi_8(sstr);
				search_chars_moving_and_skipping(&sstr, &cmd_pos, ' ', ' ');
			}

			for (int i = 0; i < 2; i++)
				zero_steps[i] += vals[i];

			_DBG("Zero steps count: ");
			_DBD32(zero_steps[0]); _DBG(", ");
			_DBD32(zero_steps[1]); _DBG("\n");

			run_stepper_timer(vals[0], vals[1], 1000, 1000, 5000, 5000);

			cmd_pos = 0;
			continue;
		}

		cmd_pos++;
	}
}

// Execute a list of points with solenoid pulses at each
void execute_path(int path[NUMBER_OF_POINTS][2]) {
	_DBG("Executing path\n\n");

	for (int i = 0; i < NUMBER_OF_POINTS; i++) {
		_DBG("Point to reach: ");
		_PRINT(path[i][0], ", ");
		_PRINT(path[i][1], "\n");

		reach_point_mm(path[i][0], path[i][1]);

		// Solenoid pulse
		custom_delay(100);
		if (i >= 0) {
			solenoide_run(1);
			custom_delay(SOLENOIDE_DELAY);
			solenoide_run(0);
		}
		custom_delay(100);
	}

	// Return to zero at the end
	reach_point_mm(0, 0);
}
