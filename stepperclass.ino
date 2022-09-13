//#include "stepper.h" // Apparently implicitly included ??
#include "digitalWriteFast.h"
#include "limits.h" // for LONG_MAX

#define DEBUG_STEPPER 1

// StepperState implementation. Constructors just inits.
StepperState::StepperState(int num_motor, int pin_pulse, int pin_dir) {
	pos_current=0;
	mypos_end=0;
	pulse_on_time=LONG_MAX;
	pulse_off_time=LONG_MAX;
  
  mypin_pulse = pin_pulse;
  mypin_dir = pin_dir;
  
  pinMode(mypin_pulse,OUTPUT);
  pinMode(mypin_dir,OUTPUT);

  this->num_motor = num_motor;
  sweeping=0;
}

// Save the endpoint and the step interval, set the direction pin 
void StepperState::prepare_move(signed long pos_end, unsigned long move_duration) {
  mypos_end = pos_end;
  unsigned long total_steps;
	if (pos_end>pos_current)  {
		digitalWrite(mypin_dir,HIGH);
		mydir=1;
    total_steps = pos_end-pos_current;
	} else if (pos_end<pos_current) {
		digitalWrite(mypin_dir,LOW);
		mydir=-1;
    total_steps = pos_current-pos_end;
	} else {
    mydir=1; // ? TODO
    total_steps = 0;
	}

  step_trace_counter=0; // DBG

  // TODO: be careful about rounding here!
  if (total_steps>0)
    this->step_interval_us = (float(move_duration)/total_steps); // Force to be float
  else
    this->step_interval_us = 100; // Not sure what to do for 0.

#if DEBUG_STEPPER
  Serial.print("PREP ");
  Serial.print(num_motor);
  Serial.print(" ");
  Serial.print(move_duration);
  Serial.print(" ");
  Serial.print(total_steps);
  Serial.print(" ");
  Serial.print(this->step_interval_us);
  Serial.println(" ");
#endif
};

// Arm the movement by setting the first "on" time
void StepperState::start_move() {
  if (mypos_end != pos_current) {
    
    digitalWrite(mypin_pulse,HIGH);
    high_time = MIN_PULSE_DUR_USEC; // Will set to low after this much time (minimum)
    pulse_on_time = micros(); // arm first movement: NOW!
    
    interval_next = int(step_interval_us); // start out with theoretical interval
    error=0.0;
    
    sweeping=1;
    sweep_start_time=pulse_on_time; // now
    debug_output(0);
  }
};

void StepperState::stop_pulse() {
    digitalWrite(mypin_pulse,LOW);
}

void StepperState::stop_move(unsigned int lower_pulse) {
  if (lower_pulse)
    stop_pulse(); // lower step pulse now

  //pulse_on_time = LONG_MAX; // Disable future movements
  sweeping=0;
  sweep_end_time=micros();
  debug_output(2);

  Serial.print(mypin_pulse);
  Serial.println("done");
};

#if DEBUG_STEPPER
void StepperState::debug_output(unsigned long msg) {
    Serial.print(msg);  // Before was trying to pass in msg string
    Serial.print(" ");
    Serial.print(mypin_pulse);
    Serial.print(" ");
    Serial.print(pos_current);
    Serial.print(" ");
    Serial.print(mypos_end);
    Serial.print(" ");
    Serial.println(step_interval_us);
}
#else
  void StepperState::debug_output(unsigned long msg) {}
#endif

void StepperState::do_update() {

  if (!sweeping)
    return;
    
  unsigned long now = micros();
  unsigned long elapsed = (now-pulse_on_time);
  
	if ( elapsed > high_time ) {
	  stop_pulse();
    high_time=-1; // Set this to be large so that won't keep pulling low, until next "on" brings high_time to Xus

/*
    if (elapsed>100) {
      bad_now = now;
      bad_elapsed = elapsed;
      bad_potime = pulse_on_time;

    // step_trace[step_trace_counter%TRACE_BUF_SIZE] = elapsed;
    // step_trace_counter++;
    } */
	}

	// Probably don't want this to execute also right after above
	// If it's too fast, that will be a problem (not enough of a duty cycle)
	// For now, assume that step_interval_us is fairly large, >> 5us or so

	else if (elapsed > interval_next ) {

    error += elapsed - step_interval_us;

    if (( step_interval_us - error ) < 0 )
      interval_next = 0;
    else
      interval_next = int( step_interval_us - error );

    if (num_motor == 1) {
      if (step_trace_counter<TRACE_BUF_SIZE) {
        step_trace[step_trace_counter%TRACE_BUF_SIZE] = elapsed;
      }
      step_trace_counter++;
    }

    pulse_on_time = now;
		digitalWrite(mypin_pulse,HIGH);
    high_time = MIN_PULSE_DUR_USEC;   // So the set low will happen once
    
    // Update the current position
	  pos_current = pos_current + mydir; // mydir will be +1 or -1
	  //pulse_off_time = micros() + MIN_PULSE_DUR_USEC;
   
    // With each step is too noisy
    if ( !(step_trace_counter % 4000) )
      debug_output(1);
      
    // Decide whether to re-arm for another movement
		if (pos_current == mypos_end) {
      // done, at destination
      stop_move(1);
		}
	}
};

Stepper1::Stepper1(int num_motor, int pin_pulse, int pin_dir) : StepperState( num_motor, pin_pulse, pin_dir ) {}

inline void Stepper1::writePulseLow(boolean value) {
  digitalWriteFast(DRIVER1_PULSE,LOW);
};
inline void Stepper1::writePulseHigh(boolean value) {
  digitalWriteFast(DRIVER1_PULSE,HIGH);
};

Stepper2::Stepper2(int num_motor, int pin_pulse, int pin_dir) : StepperState( num_motor, pin_pulse, pin_dir ) {}

inline void Stepper2::writePulseLow(boolean value) {
  digitalWriteFast(DRIVER2_PULSE,LOW);
};
inline void Stepper2::writePulseHigh(boolean value) {
  digitalWriteFast(DRIVER2_PULSE,HIGH);
};
