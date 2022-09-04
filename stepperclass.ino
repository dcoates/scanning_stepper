//#include "stepper.h" // Apparently implicitly included ??
#include "limits.h" // for LONG_MAX

#define DEBUG_STEPPER 1

// StepperState implementation. Constructors just inits.
StepperState::StepperState(int pin_pulse, int pin_dir) {
	pos_current=0;
	mypos_end=0;
	pulse_on_time=LONG_MAX;
	pulse_off_time=LONG_MAX;
  
  mypin_pulse = pin_pulse;
  mypin_dir = pin_dir;
  
  pinMode(mypin_pulse,OUTPUT);
  pinMode(mypin_dir,OUTPUT);

  sweeping=0;
}

// Save the endpoint and the step interval, set the direction pin 
void StepperState::prepare_move(signed int pos_end, unsigned long move_duration) {
  mypos_end = pos_end;
  unsigned long total_steps;
	if (pos_end>pos_current)  {
		digitalWrite(mypin_dir,HIGH);
		mydir=1;
    total_steps = pos_end-pos_current;
	} else {
		digitalWrite(mypin_dir,LOW);
		mydir=-1;
    total_steps = pos_current-pos_end;
	}

  // TODO: be careful about rounding here!
  this->step_interval_us = (move_duration/total_steps);

#if DEBUG_STEPPER
  Serial.println(move_duration);
  Serial.println(total_steps);
  Serial.println(this->step_interval_us);
#endif
};

// Arm the movement by setting the first "on" time
void StepperState::start_move() {
  pulse_on_time = micros(); // arm first movement
  sweeping=1;
  debug_output(0);
};

void StepperState::stop_pulse() {
    digitalWrite(mypin_pulse,LOW);
    pulse_off_time=LONG_MAX;
}

void StepperState::stop_move() {
  stop_pulse(); // lower step pulse now
  pulse_on_time = LONG_MAX; // Disable future movements
  sweeping=0;
  debug_output(2);
};

void StepperState::debug_output(unsigned long msg) {
#if DEBUG_STEPPER
    Serial.print(msg);  // Before was trying to pass in msg string
    Serial.print(" ");
    Serial.print(mypin_pulse);
    Serial.print(" ");
    Serial.print(pos_current);
    Serial.print(" ");
    Serial.print(mypos_end);
    Serial.print(" ");
    Serial.println(step_interval_us);
#endif
}

void StepperState::do_update() {
	if (micros() >= pulse_off_time ) {
	  stop_pulse();
	}

	// Probably don't want this to execute also right after above
	// If it's too fast, that will be a problem (not enough of a duty cycle)
	// For now, assume that step_interval_us is fairly large, >> 5us or so
	if (micros() >= pulse_on_time ) {

		digitalWrite(mypin_pulse,HIGH);
    // Update the current position
    pos_current = pos_current + mydir; // mydir will be +1 or -1
		pulse_off_time = micros() + MIN_PULSE_DUR_USEC;
    debug_output(1);
      
    // Decide whether to re-arm for another movement
		if (pos_current != mypos_end) {
			pulse_on_time = micros() + step_interval_us; // Keep steppin'
		} else { // done, at destination
      stop_move();
		}
	}
};
	//long int step_size;
	//long int step_rate; 

	//long int camera_rate;
