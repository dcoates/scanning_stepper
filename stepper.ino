  #include "stepper.h"
#include "driver_unit.h"
#include "digitalWriteFast.h"

// In theory, the first of each of these numbers depends on the DIP switch settings (steps/rev)
// The second depends on the units (1 revolution=360 deg, X mm, etc..). BUT
// The relationship isn't so clear based on the other mechanical stages, so expressing as steps
// (and measuring empirically) is more straightforward.
#define STEPPER1_STEPS_PER_UNIT (200.0/2.0) 
#define STEPPER2_STEPS_PER_UNIT 1
// Old stepper2: (8000.0/4.0)  
#define STEPPER3_STEPS_PER_UNIT 1
// Old stepper3 DIP(8000.0/4.0)
//#define STEPPER4_STEPS_PER_UNIT (1600.0/4.0) /* Degrees. Empirical: not sure why "/4" */
#define STEPPER4_STEPS_PER_UNIT (400/8.0)

// Stepper 1: up/down scissors lifter: DIP? . Lookup table to support non-linear lift
// Stepper 2: in/out: DIP? . lookup table to allow cosine motion
// Stepper 3: rotational: DIP?  Constant steps.
// Stepper 4 (Old #1) : horizontal : 200  . Constant steps.
// limit1: Comes across the DB9 serial cable from the scissors lifter (new #1)
// limit2: Comes from the horizontal motor bundle: (old motor #1)
// limit3: Comes from the motor #2 bundle
// limit4: No limit switch for motor 4 (horizontal) yet
// Camera is the pair wired into the #3 bundle

// Zero/middle point for Stepper1 is at 1300 steps away from "0" (due to non-linear scissors)
// Zero/middle point for Stepper2?
// Zero/middle point for Stepper3 is zero

// Where to begin the sweep. Button press left moves from "0" here
#define STEPPER1_START (-32.5+9.5) 
#define STEPPER2_START -900
#define STEPPER3_START 95
#define STEPPER4_START -35

// Where to sweep until. Button press right cases sweep until value is reached
#define STEPPER1_END (32.5+9.5)
#define STEPPER2_END 0
#define STEPPER3_END -95
#define STEPPER4_END 35

// Start some number of steps in the table
#define TABLE_START1 250

// -30 to +30 : 30.36mm

#define SWEEP_TIME_SEC 3.0

#define CAMERA_SNAP_INTERVAL_HORIZONTAL (3000000/7)
#define CAMERA_SNAP_INTERVAL_VERTICAL (3000000/7)

#define BUTTON_HOLD_MS 1500
#define LIMS_DEBOUNCE_PERIOD_US 10000 // Debounce limit switch over a 2000us (2ms). It must remain stable/constant for this long
#define NUDGE_SMALL1 100
#define NUDGE_LARGE1 500
#define NUDGE_SMALL2 250
#define NUDGE_LARGE2 500
#define NUDGE_SMALL3 31
#define NUDGE_LARGE3 100
#define NUDGE_SMALL4 31
#define NUDGE_LARGE4 100

#define STEP_BACK1 500 //500
#define STEP_BACK2 850 //450
#define STEP_BACK3 NUDGE_LARGE3

#define REAL_SYSTEM 1 // On the real hardware, this should be 1. If 0, we are probably developing/testing  w/o any hardware.

// These are shared between legacy.ino and this file
// So that we can peek at the buttons
unsigned long lastDebounceTime1 = 0; 
unsigned long lastDebounceTime2 = 0; 
unsigned long lastDebounceTime3 = 0; 
int dir1Current;
int dir2Current;
int dir3Current;

// class instances for each stepper motor
StepperLUT8* stepper1;
StepperLUT16* stepper2;
StepperConstant* stepper3;
StepperConstant* stepper4;

// One motor instance will write into the trace buffer:
#define TRACE_BUF_SIZE 4
unsigned int any_sweeping=0;
unsigned int step_trace_counter=0;
unsigned int step_trace[TRACE_BUF_SIZE];

// All 3 instances overwrite this global (TODO fix):
unsigned long sweep_start_time;
unsigned long sweep_end_time;

// All 3 write into this, to indicate extra-long intervals,
// Suggestive of a timing error. Okay as-is for error checking.
unsigned long bad_now;
unsigned long bad_elapsed;
unsigned long bad_potime;


#define POS_BUF_SIZE 32 * 5
unsigned int pos_curr=0;
signed long pos_buffer[POS_BUF_SIZE]; // Store tiime + pos for each motor
unsigned long sweep_snap_time=0;
unsigned long sweep_snap_interval;

uint8_t in_sweep=0;
void setup() {

    Serial.begin(115200);
    Serial.println("ready");
    
    legacy_setup(); // Call legacy setup code. Sets pin directions, mainly.

    stepper1 = new StepperLUT8(1, DRIVER1_PULSE, DRIVER1_DIR, limit1, (signed long)(STEPPER1_START*STEPPER1_STEPS_PER_UNIT));
    stepper2 = new StepperLUT16(2, DRIVER2_PULSE, DRIVER2_DIR, limit2, (signed long)(STEPPER2_START*STEPPER2_STEPS_PER_UNIT)); 
    stepper3 = new StepperConstant(3, DRIVER3_PULSE,DRIVER3_DIR, limit3, (signed long)(STEPPER3_START*STEPPER3_STEPS_PER_UNIT));
    stepper4 = new StepperConstant(4, DRIVER4_PULSE,DRIVER4_DIR, limit3, (signed long)(STEPPER4_START*STEPPER4_STEPS_PER_UNIT));

    in_sweep=0;
}

void print_pos()
{
  
  Serial.print("pos1:");
  Serial.println(stepper1->pos_current);
  delay(50);
  Serial.print("pos2:");
  Serial.println(stepper2->pos_current);
  delay(50);
  Serial.print("pos3:");
  Serial.println(stepper3->pos_current);
  delay(50);
  Serial.print("pos4:");
  Serial.println(stepper4->pos_current);
}

void debug_blast() {
  int n;
  Serial.println(step_trace_counter);
  float sum=0;
  for (n=0; n<TRACE_BUF_SIZE; n++) {
    sum += step_trace[n];
    Serial.print(n);
    Serial.print(": ");            
    Serial.print(step_trace[n]);
    if ( (n%8)==0) 
      Serial.println(" ");
    else
      Serial.println(" ");
  }
  Serial.print(" AVG: " );
  Serial.println(sum/TRACE_BUF_SIZE);
  Serial.print("Count 1T: " );
  Serial.println(stepper1->table_counter);
  Serial.print("Count 2: " );
  Serial.println(stepper2->steps_completed);
  Serial.print("Count 3: " );
  Serial.println(stepper3->steps_completed);
  Serial.print("Sweep: ");
  Serial.println(sweep_start_time);
  Serial.println(sweep_end_time);
  Serial.print("Sweep elapsed: ");
  Serial.println(sweep_end_time-sweep_start_time);
  Serial.print("BAD now: ");
  Serial.println(bad_now);
  Serial.println(bad_elapsed);
  Serial.println(bad_potime);

  Serial.print("Pos: ");
  Serial.println(pos_curr);
  for (n=0; n<POS_BUF_SIZE; n++) {
    Serial.print(pos_buffer[n]);
    Serial.print(' ');
    if ((n%5)==4)
      Serial.println();
  }

  Serial.println("NOW: ");
  print_pos();
}

void handle_motion(StepperState* which_motor, signed long amount) {
  // Shouldn't look like a sweep. We want interrupts running, etc.
  in_sweep=0;

  if (amount<0) 
    which_motor->smooth_start(0,-amount);
  else // positive
    which_motor->smooth_start(1,amount);

};

// This calibration function blocks everything else. The state machine will not run
// while doing the calibrations.
// Amount indicates direction too.
int calibrate_new(StepperState* which_motor, signed long amount) {
  handle_motion(which_motor, amount); // start the motor, uses "Tone"

  // Repeat until failsafe button or limit hit
  while ( (digitalRead(m3go)==LOW)  && (!which_motor->limit_hit) )
  {
      which_motor->read_limit(); // debounce and read.
      delay(1);
  }
  smooth_stop();

  if (!which_motor->limit_hit) { // If user hit panic button
    Serial.println('Failsafe during calibrate');
    return 0;
  }

  handle_motion(which_motor, -amount); // Go opposite way for a bit

  delay(1250); // 1250 for Motor  1 (lifter)
  smooth_stop();
  
  // Tell it to reset itself, but at a known position.
  which_motor->reset_state();
  which_motor->pos_current = which_motor->pos_start;

  // Now this motor is just off the limit switch
  return 1;
}

void smooth_stop() {
  stepper1->smooth_stop();
  stepper2->smooth_stop();
  stepper3->smooth_stop();
  stepper4->smooth_stop();

  in_sweep=0;
};

String str_move_amount= ""; // USed like global variable in move1

void process_serial_commands() {
  
      // Check for serial commands
      if (Serial.available() > 0) {
        // read the incoming byte:
        int incomingByte = Serial.read();

        // Sweeps: (vertical, then horizontal)
        if (incomingByte=='S') {
          sweep_to_start();
        } else if (incomingByte=='F') {
          sweep_to_end();
        } else if (incomingByte=='Z') {
          sweep_to_zero();
        }
        
        else if (incomingByte=='s') {
          sweep_horizontal(
       (signed long) (STEPPER4_START*STEPPER4_STEPS_PER_UNIT),
       SWEEP_TIME_SEC*1000000.0,1);
        } else if (incomingByte=='f') {
          sweep_horizontal(
       (signed long) (STEPPER4_END*STEPPER4_STEPS_PER_UNIT),
       SWEEP_TIME_SEC*1000000.0,2);
        } else if (incomingByte=='z') {
          sweep_horizontal(
       (signed long) 0,
       SWEEP_TIME_SEC*1000000.0,3);

        // Debug/info
        } else if (incomingByte=='?') {
          debug_blast();
        } else if (incomingByte=='p') {
          print_pos();
        }

        // Nudge commands: (follow top row of ASDF keyboard. -/+, then large/small is case
        else if (incomingByte=='Q') {handle_motion(stepper1,(signed long)-NUDGE_LARGE1);}
        else if (incomingByte=='q') {handle_motion(stepper1,(signed long)-NUDGE_SMALL1);}
        else if (incomingByte=='w') {handle_motion(stepper1,(signed long)NUDGE_SMALL1);}
        else if (incomingByte=='W') {handle_motion(stepper1,(signed long)NUDGE_LARGE1);}

        else if (incomingByte=='E') {handle_motion(stepper2,(signed long)-NUDGE_LARGE2);}
        else if (incomingByte=='e') {handle_motion(stepper2,(signed long)-NUDGE_SMALL2);}
        else if (incomingByte=='r') {handle_motion(stepper2,(signed long)NUDGE_SMALL2);}
        else if (incomingByte=='R') {handle_motion(stepper2,(signed long)NUDGE_LARGE2);}

        else if (incomingByte=='T') {handle_motion(stepper3,(signed long)-NUDGE_LARGE3);}
        else if (incomingByte=='t') {handle_motion(stepper3,(signed long)-NUDGE_SMALL3);}
        else if (incomingByte=='y') {handle_motion(stepper3,(signed long)NUDGE_SMALL3);}
        else if (incomingByte=='Y') {handle_motion(stepper3,(signed long)NUDGE_LARGE3);}

        else if (incomingByte=='U') {handle_motion(stepper4,(signed long)-NUDGE_LARGE4);}
        else if (incomingByte=='u') {handle_motion(stepper4,(signed long)-NUDGE_SMALL4);}
        else if (incomingByte=='i') {handle_motion(stepper4,(signed long)NUDGE_SMALL4);}
        else if (incomingByte=='I') {handle_motion(stepper4,(signed long)NUDGE_LARGE4);}

        else if (incomingByte=='a') {calibrate_new(stepper1,(signed long)-STEP_BACK1);}
        else if (incomingByte=='b') {calibrate_new(stepper2,(signed long)-STEP_BACK2);}
        else if (incomingByte=='c') {calibrate_new(stepper3,(signed long) STEP_BACK3);}
        //else if (incomingByte=='4') {calibrate_new(stepper4,(signed long)-NUDGE_LARGE);}

        else if (incomingByte=='x') {smooth_stop();}

		// accumulate string
		else if (incomingByte=='-') { str_move_amount += (char)incomingByte; }
		else if (incomingByte>='0' && incomingByte<='9') { str_move_amount += (char)incomingByte; }

		// Use the accumulated number:
        else if (incomingByte=='A') {move1(stepper1);}
        else if (incomingByte=='B') {move1(stepper2);}
        else if (incomingByte=='C') {move1(stepper3);}
        else if (incomingByte=='D') {move1(stepper4);}
    }
}

void loop() {
  //any_sweeping = 1; //(stepper1->sweeping || stepper2->sweeping || stepper3->sweeping); // & stepper3->sweeping & stepper4->sweeping;
    
  if (!in_sweep) {
      // No sweep happening: do legacy (manual) ops, serial debugging, check for hold

      interrupts();
      process_serial_commands();

#if REAL_SYSTEM
    //legacy_loop(); // main loop from old front panel for manual ops

    // Are any buttons held to initiate sweep?
    unsigned long now = millis();
    if (((now - lastDebounceTime1)>BUTTON_HOLD_MS) && dir1Current && (!any_sweeping) ) {
      sweep_to_start();
      //noInterrupts();
    } else if (((now - lastDebounceTime2)>BUTTON_HOLD_MS) && dir2Current && (!any_sweeping) ) { 
      sweep_to_zero();
      //noInterrupts();
    } else if (((now - lastDebounceTime3)>BUTTON_HOLD_MS) && dir3Current && (!any_sweeping) ) { 
      sweep_to_end();
      //noInterrupts();
    } 
#endif // REAL_SYSTEM

  } else { // In a sweep
#if REAL_SYSTEM
    // Failsafe: touch right GO button to stop. Don't even debounce: bail immediately if any button action.
    if (digitalRead(m3go)==HIGH) {
      stepper1->stop_move(1);
      stepper2->stop_move(1);
      stepper3->stop_move(1); 
      stepper4->stop_move(1); 

      stepper1->smooth_stop();
      stepper2->smooth_stop();
      stepper3->smooth_stop();
      stepper4->smooth_stop();

      interrupts(); 
      Serial.println("FAILSAFE STOP"); 
      in_sweep=0;    
    } else {
#else
    if (1) {
#endif // REAL_SYSTEM

      unsigned long now = micros();
      if ( (now - sweep_snap_time ) >= sweep_snap_interval) {
        digitalWrite(camera_pulse,HIGH); // Tell camera to take a snap

        int error = (now-sweep_snap_time) - sweep_snap_interval;
        if (pos_curr==0)
          error=0; // no error on first one
          
        pos_buffer[pos_curr++] = now;
        pos_buffer[pos_curr++] = (signed long)stepper1->pos_current;
        pos_buffer[pos_curr++] = (signed long)stepper2->pos_current;
        pos_buffer[pos_curr++] = (signed long)stepper3->pos_current;
        pos_buffer[pos_curr++] = (signed long)stepper3->pos_current;
        if (pos_curr >= POS_BUF_SIZE)
          pos_curr = 0; // Paranoid buffer size checking
          
         sweep_snap_time = now-error; // try to get the next one to happen a little earlier

        // Only exit the sweep after the last picture taken
        in_sweep = (stepper1->sweeping || stepper2->sweeping || stepper3->sweeping || stepper4->sweeping);
      } else {
        digitalWrite(camera_pulse,LOW);
      }
      
      // Move if there is a confirmed sweep happening.
      stepper1->do_update();
      stepper2->do_update();
      stepper3->do_update();
      stepper4->do_update();
      };

  };
}

void move1(StepperState* which_motor) {
  signed long amt=(signed long)str_move_amount.toInt();
  which_motor->prepare_move_relative( amt,2.0*1000000.0, 1); //2 second
  which_motor->start_move();
  Serial.print("move1 ");
  Serial.println(amt);
  str_move_amount=""; // Reset for next time
  in_sweep=1;
}


void sweep_to(signed long pos1, signed long pos2, signed long pos3, unsigned long duration, int mode) {
  Serial.print("Sweep ");
  Serial.print(pos1);
  Serial.print(",");
  Serial.print(pos2);
  Serial.print(" ");
  Serial.print(pos3);
  Serial.print(" ");
  Serial.println(duration);

  stepper1->prepare_move(  pos1,duration,mode);
  stepper2->prepare_move(  pos2,duration,mode);
  stepper3->prepare_move(  pos3,duration,mode);
  stepper1->start_move();
  stepper2->start_move();
  stepper3->start_move();
  sweep_start_time=millis();
  
  in_sweep=1; // so main loop knows we are sweeping
  step_trace_counter=0;
  pos_curr=0;
  sweep_snap_interval=(unsigned long)CAMERA_SNAP_INTERVAL_VERTICAL;
  sweep_snap_time=sweep_start_time-sweep_snap_interval; // So it'll trigger immediately on entry
}

void sweep_horizontal(signed long pos, unsigned long duration, int mode) {
  Serial.println("Sweep H");

  stepper4->prepare_move( pos, duration, mode);
  stepper4->start_move();
  sweep_start_time=millis();
  
  in_sweep=1; // so main loop knows we are sweeping
  step_trace_counter=0;
  pos_curr=0;
  sweep_snap_interval=(unsigned long)CAMERA_SNAP_INTERVAL_HORIZONTAL;
  sweep_snap_time=sweep_start_time-sweep_snap_interval; // So it'll trigger immediately on entry
}

// Sweep modes: 1 (to start), 0 (to zero) 2 (to end)
// Need these to handle the reversing that Motor 2 does
void sweep_to_start() {
  stepper1->table_counter=0; // reset to start of LUT. Not totally accurate, but avoids going past table.
  stepper2->table_counter=0;
  stepper3->table_counter=0;
  
  sweep_to(
       (signed long) (STEPPER1_START*STEPPER1_STEPS_PER_UNIT),
       (signed long) (STEPPER2_START*STEPPER2_STEPS_PER_UNIT),
       (signed long) (STEPPER3_START*STEPPER3_STEPS_PER_UNIT),
      SWEEP_TIME_SEC*1000000.0, 1); 
}

void sweep_to_zero() {
  stepper1->table_counter=0; // reset to start of LUT. Not totally accurate, but avoids going past table.
  stepper2->table_counter=0;
  stepper3->table_counter=0;
  
   sweep_to(
       (signed long) 0,
       (signed long) 0,
       (signed long) 0,
      SWEEP_TIME_SEC*1000000.0, 0); 
}

void sweep_to_end() {
  
  stepper1->reset_state();
  stepper2->reset_state();
  stepper3->reset_state();

  stepper1->table_counter=TABLE_START1; // Start partway through the lookup table
  
  stepper1->pos_current = stepper1->pos_start;
  stepper2->pos_current = stepper2->pos_start;
  stepper3->pos_current = stepper3->pos_start;

    sweep_to(
       (signed long) (STEPPER1_END*STEPPER1_STEPS_PER_UNIT),
       (signed long) (STEPPER2_END*STEPPER2_STEPS_PER_UNIT),
       (signed long) (STEPPER3_END*STEPPER3_STEPS_PER_UNIT),
      SWEEP_TIME_SEC*1000000.0, 2);
}
