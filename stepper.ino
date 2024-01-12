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

#define VERBOSE  0
#define VERBOSE2 1

// Stepper 1: up/down scissors lifter: DIP? . Lookup table to support non-linear lift
// Stepper 2: in/out: DIP? . lookup table to allow cosine motion
// Stepper 3: rotational: DIP?  Constant steps.
// Stepper 4 (Old #1) : horizontal : 200  . Constant steps.
// limit1: Comes across the DB9 serial cable from the scissors lifter (new #1)
// limit2: Comes from the horizontal motor bundle: (old motor #1)
// limit3: Comes from the motor #2 bundle
// limit4: No limit switch for motor 4 (horizontal) yet
// Camera is the pair wired into the #3 bundgle

// Zero/middle point for Stepper1 is at 1000 steps away from "0"/middle (due to non-linear scissors)
// Zero/middle point for Stepper2?
// Zero/middle point for Stepper3 is zero

// Where to begin the sweep. Button press left moves from "0" here
#define STEPPER1_START (-32.5+10.5) 
#define STEPPER2_START -900
#define STEPPER3_START 95
#define STEPPER4_START -30

// Where to sweep until. Button press right cases sweep until value is reached
#define STEPPER1_END (32.5+10.5)
#define STEPPER2_END (0) // Table/indexing has some off-by-one issues !
#define STEPPER3_END -95
#define STEPPER4_END 30

// Start some number of steps in the table
#define TABLE_START1 200 //250

// -30 to +30 : 30.36mm
#define SEC_TO_USEC 1000000
#define MICRO_TO_USEC 1000

#define SWEEP_TIME_SEC_H 3.0
#define SWEEP_TIME_SEC_V 3.0

#define CAMERA_SNAP_INTERVAL_HORIZONTAL (4000000/49) //3s/7 for GY data
#define CAMERA_SNAP_INTERVAL_VERTICAL (3000000/26) //diagonal scan uses snap interval from vertical scan. accurate up to 10-20 ms (?) //rounds up wrt num of frames

#define BUTTON_HOLD_MS 1500
#define LIMS_DEBOUNCE_PERIOD_US 10000 // Debounce limit switch over a 2000us (2ms). It must remain stable/constant for this long
#define NUDGE_SMALL1 100
#define NUDGE_LARGE1 500
#define NUDGE_SMALL2 250
#define NUDGE_LARGE2 500
#define NUDGE_SMALL3 31
#define NUDGE_LARGE3 127
#define NUDGE_SMALL4 31
#define NUDGE_LARGE4 100

#define STEP_BACK1 550 //500
#define STEP_BACK2 5250 // 850 //450
#define STEP_BACK3 NUDGE_LARGE3

#define HORIZ_SENTINEL 9999

#define REAL_SYSTEM 0 // On the real hardware, this should be 1. If 0, we are probably developing/testing  w/o any hardware.
// Not used too heavily. Mainly for failsafe panic buttons, and limit stuff.

// These are shared between legacy.ino and this file
// So that we can peek at the buttons
unsigned long lastDebounceTime1 = 0; 
unsigned long lastDebounceTime2 = 0; 
unsigned long lastDebounceTime3 = 0; 
int dir1Current;
int dir2Current;
int dir3Current;

// class instances for each stepper motor
//StepperLUT8* stepper1;
//StepperLUT16* stepper2;
StepperConstant* stepper1;
StepperConstant* stepper2;
StepperConstant* stepper3;
StepperConstant* stepper4;

// One motor instance will write into the trace buffer:
#define TRACE_BUF_SIZE 1
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

#define PVT_TABLE_ENTRIES 32
uint8_t pvt_count=0;
uint8_t pvt_index=0; // for current sweep
signed int pvt_table[PVT_TABLE_ENTRIES*4];
uint8_t in_pvt_sweep=0;

uint8_t in_sweep=0;
void setup() {

    Serial.begin(115200);
    Serial.println("ready");
    
    legacy_setup(); // Call legacy setup code. Sets pin directions, mainly.

    //stepper1 = new StepperLUT8(1, DRIVER1_PULSE, DRIVER1_DIR, limit1, (signed long)(STEPPER1_START*STEPPER1_STEPS_PER_UNIT));
    //stepper2 = new StepperLUT16(2, DRIVER2_PULSE, DRIVER2_DIR, limit2, (signed long)(STEPPER2_START*STEPPER2_STEPS_PER_UNIT)); 
    stepper1 = new StepperConstant(1, DRIVER1_PULSE,DRIVER1_DIR, limit1, (signed long)(STEPPER1_START*STEPPER1_STEPS_PER_UNIT));
    stepper2 = new StepperConstant(2, DRIVER2_PULSE,DRIVER2_DIR, limit2, (signed long)(STEPPER2_START*STEPPER2_STEPS_PER_UNIT));
    stepper3 = new StepperConstant(3, DRIVER3_PULSE,DRIVER3_DIR, limit3, (signed long)(STEPPER3_START*STEPPER3_STEPS_PER_UNIT));
    stepper4 = new StepperConstant(4, DRIVER4_PULSE,DRIVER4_DIR, limit3, (signed long)(STEPPER4_START*STEPPER4_STEPS_PER_UNIT));

    in_sweep=0;
    in_pvt_sweep=0;
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

  for (n=0; n<pvt_count; n++) {
    Serial.print(n);
    Serial.print(": ");            
    for (int m=0; m<4; m++) {
      Serial.print(pvt_table[n*4+m]);
      Serial.print(" ");
    }
    Serial.println();
  }
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

String str_input_buffer= ""; // USed like global variable in move1

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
        
        else if (incomingByte=='s') {horiz_sweep_start();}
        else if (incomingByte=='f') {horiz_sweep_end(); }
        else if (incomingByte=='z') {horiz_sweep_zero(); }

        else if (incomingByte=='P') {horiz_sweep_start(); sweep_to_start(); }
        else if (incomingByte=='L') {horiz_sweep_end(); sweep_to_end(); }
        else if (incomingByte=='O') {horiz_sweep_zero(); sweep_to_zero(); }

        // Debug/info
        else if (incomingByte=='?') {
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

		// accumulate string into buffer for various functions:
        else if ( (incomingByte>='0' && incomingByte<='9') || incomingByte=='-' || incomingByte=='.' || incomingByte==',')
          { str_input_buffer += (char)incomingByte; } // append to end of string

		// Use the accumulated number:
        else if (incomingByte=='A') {movex();}

      else if (incomingByte=='v') {pvt_add();}
      else if (incomingByte=='V') {pvt_execute();}
      else if (incomingByte=='*') {pvt_count=0; pvt_index=0;} //reset the count
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
        pos_buffer[pos_curr++] = (signed int)stepper1->pos_current;
        pos_buffer[pos_curr++] = (signed int)stepper2->pos_current;
        pos_buffer[pos_curr++] = (signed int)stepper3->pos_current;
        pos_buffer[pos_curr++] = (signed int)stepper4->pos_current;
        //pos_buffer[pos_curr++] = (signed int)stepper1->table_counter;
        if (pos_curr >= POS_BUF_SIZE)
          pos_curr = 0; // Paranoid buffer size checking
          
        sweep_snap_time = now-error; // try to get the next one to happen a little earlier

        if (pos_curr != 0) {
          pvt_next(); // Do that if need be 
        }

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

void pvt_add() {
  String param=strtok(str_input_buffer.c_str(),",");
  signed int pos1=(signed long)param.toInt();
  param=strtok(NULL,",");
  signed int pos2=(signed long)param.toInt();
  param=strtok(NULL,",");
  signed int pos3=(signed long)param.toInt();
  param=strtok(NULL,",");
  signed int pos4=(signed long)param.toInt();
  str_input_buffer=""; // Reset for next time

  pvt_table[pvt_count*4+0] = pos1;
  pvt_table[pvt_count*4+1] = pos2;
  pvt_table[pvt_count*4+2] = pos3;
  pvt_table[pvt_count*4+3] = pos4;
  Serial.print("PVTadd ");
  Serial.print(pvt_count);
  //Serial.print(":");
  //Serial.print(pos1);
  //Serial.print(",");
  //Serial.print(pos2);
  //Serial.print(",");
  //Serial.print(pos3);
  //Serial.print(",");
  //Serial.println(pos4);

  pvt_count++;
  if (pvt_count>PVT_TABLE_ENTRIES)
    pvt_count=0;
}

void pvt_execute() {
  unsigned long now=micros();
  Serial.print(now);
  Serial.println(" PVT_EXE");

#if 0 // Not needed since we are now letting it call sweep_to directly
  sweep_snap_interval=(unsigned long)CAMERA_SNAP_INTERVAL_VERTICAL; // TODO
  sweep_start_time=millis();
  sweep_snap_time=sweep_start_time-sweep_snap_interval; // So it'll trigger immediately on entry

  step_trace_counter=0;
  pos_curr=0;

  in_sweep=1;
#endif //0

  pvt_index=0;
  in_pvt_sweep=1;
  pvt_first(); // prepare the next/first move (to the second position!)
}

void pvt_first() {
  if (!in_pvt_sweep)
    return;

  if (pvt_index==pvt_count) {
    in_pvt_sweep=0;
    in_sweep=0;
    pvt_index=0;

    stepper1->sweeping=0;
    stepper2->sweeping=0;
    stepper3->sweeping=0;
    stepper4->sweeping=0;
    return;
  }

  unsigned long dur_usec=3.0 * SEC_TO_USEC; //sweep_snap_interval;

  int pos1=pvt_table[(pvt_count-1)*4+0];
  int pos2=pvt_table[(pvt_count-1)*4+1];
  //pos2=-pos2; // move to "opposite" across center (but will reverse halfway)
  int pos3=pvt_table[(pvt_count-1)*4+2];

  int pos4=pvt_table[(pvt_count-1)*4+3];
  if (pos4<HORIZ_SENTINEL) {
    //stepper4->prepare_move( pos4*STEPPER4_STEPS_PER_UNIT,dur_usec, 1);
    //stepper4->dur_extra=0.0;
    //stepper4->start_move();

    sweep_horizontal((signed long)pos4,dur_usec,1);
  }

  stepper2->pos_start=pos2; // what to reverse to after zero (original value)
  sweep_to(
       (signed long) pos1,
       (signed long) -pos2, // (will turn around and go back to start)
       (signed long) pos3,
       dur_usec, MODE_PVT); 
}

void pvt_next() {
  if (!in_pvt_sweep)
    return;

  unsigned long dur_usec=sweep_snap_interval;

  int pos1=pvt_table[pvt_index*4+0];
  int pos2=pvt_table[pvt_index*4+1];
  int pos3=pvt_table[pvt_index*4+2];
  int pos4=pvt_table[pvt_index*4+3];

  if (pvt_index==pvt_count) {
    in_pvt_sweep=0;
    in_sweep=0;
    pvt_index=0;

    stepper1->sweeping=0;
    stepper2->sweeping=0;
    stepper3->sweeping=0;
    stepper4->sweeping=0;
    return;

    Serial.print(micros());
    Serial.print(" PVTX");
  }

  // Fake it by changing the avg speed of each motor
  long int int1new=dur_usec/abs(pos1 - stepper1->pos_current);
  stepper1->step_interval_us=int1new;

  // for stepper2, switch direction at the halfway point
  if ( (pvt_index)==int( pvt_count/2) ) {

    stepper2->prepare_move( stepper2->pos_start, 0L, MODE_PVT); // This will reverse dir also and reset mypos_end, etc. //Reversing=mode 3
    stepper2->start_move(); 

#if 1
  unsigned long now=micros();
  Serial.print(now);
  Serial.print(" PVTR ");
  Serial.print(pos2);
  Serial.print(" ");
  Serial.println(stepper2->pos_current);
#endif 
  }
  // Then update to correct speed for next interval
  long int pos_delta = abs(pos2) - abs(stepper2->pos_current);
  if (pos_delta==0)  {
    stepper2->step_interval_us=dur_usec*10; // don't move at all during this interval (will be updatedd next time)
    unsigned long now=micros();
    Serial.print(now);
    Serial.println(" PVT0");
  } else {
    long int int2new=dur_usec/abs(pos_delta) ;
    stepper2->step_interval_us=int2new;
  }

  long int int3new=dur_usec/abs(pos3 - stepper3->pos_current);
  stepper3->step_interval_us=int3new;

  long int int4new=dur_usec/abs(pos4 - stepper4->pos_current);
  stepper4->step_interval_us=int4new;

#if VERBOSE
  unsigned long now=micros();
  Serial.print(now);
  Serial.print(" PVTN ");
  Serial.print(pvt_index);
  Serial.print("/");
  Serial.print(pvt_count);
  Serial.print(":");
  Serial.print(stepper1->pos_current);
  Serial.print(",");
  Serial.print(int1new);
  Serial.print(",");
  Serial.print(pos1);
  Serial.print(",");
  Serial.print(pos2);
  Serial.print(",");
  Serial.print(pos3);
  Serial.print(",");
  Serial.println(pos4);
#endif

  pvt_index++;
}


void movex(void) {
  String param=strtok(str_input_buffer.c_str(),",");
  signed long pos1=(signed long)param.toInt();
  param=strtok(NULL,",");
  signed long pos2=(signed long)param.toInt();
  param=strtok(NULL,",");
  signed long pos3=(signed long)param.toInt();
  param=strtok(NULL,",");
  signed long pos4=(signed long)param.toInt();
  str_input_buffer=""; // Reset for next time

  stepper1->prepare_move( pos1,2.0*1000000.0, 1); //2 second
  stepper1->dur_extra=0.0; // move at normal_speed
  //stepper1->table_counter=0; 
  stepper1->start_move();

  stepper2->prepare_move( pos2,2.0*1000000.0, 1); //2 second
  stepper2->dur_extra=0.0; // move at normal_speed
  //stepper2->table_counter=0; 
  stepper2->start_move();
  stepper2->pos_start = pos2;  // Need to save this for when we turn around

  stepper3->prepare_move( pos3,2.0*1000000.0, 1); //2 second
  stepper3->dur_extra=0.0; // move at normal_speed
  stepper3->start_move();

  if (pos4 <HORIZ_SENTINEL) {
    stepper4->prepare_move( pos4,2.0*1000000.0, 1); //2 second
    stepper4->dur_extra=0.0; // move at normal_speed
    stepper4->start_move();
  }

#if VERBOSE2
  unsigned long now=micros();
  Serial.print(now);
  Serial.print(" mvX");
  //Serial.print(which_motor->num_motor);
  Serial.print(":");
  Serial.print(pos1);
  Serial.print(",");
  Serial.print(pos2);
  Serial.print(",");
  Serial.print(pos3);
  Serial.print(",");
  Serial.println(pos4);
#endif

  in_sweep=1;
}

void sweepx(StepperState* which_motor) {
  String param=strtok(str_input_buffer.c_str(),",");
  signed long pos_start=(signed long)param.toInt();
  param=strtok(NULL,",");
  signed long pos_end=(signed long)param.toInt();
  param=strtok(NULL,",");
  signed long duration_usec=(signed long)param.toInt();
  param=strtok(NULL,",");
  double dur_extra=param.toFloat();
  param=strtok(NULL,",");
  signed long table_offset=(signed long)param.toInt();
  param=strtok(NULL,",");
  signed long pos2=(signed long)param.toInt();
  param=strtok(NULL,",");
  double dur_extra2=param.toFloat();
  param=strtok(NULL,",");
  signed long pos3=(signed long)param.toInt();
  
  param=strtok(NULL,",");
  signed long horiz_start=param.toInt();
  param=strtok(NULL,",");
  signed long horiz_end=param.toInt();
  
  Serial.print("sweepX@");
  Serial.print(which_motor->num_motor);
  Serial.print(":");
  Serial.print(pos_start);
  Serial.print(",");
  Serial.print(pos_end);
  Serial.print(",");
  Serial.print(duration_usec);
  Serial.print(",");
  Serial.print(dur_extra,4);
  Serial.print(",");
  Serial.print(table_offset);
  Serial.print(",");
  Serial.print(pos2);
  Serial.print(",");
  Serial.print(dur_extra2,4);
  Serial.print(",");
  Serial.print(stepper3->pos_current);
  Serial.print(",");
  Serial.print(pos3);

  Serial.print(",");
  Serial.print(horiz_start);
  Serial.print(",");
  Serial.println(horiz_end);

  stepper1->reset_state();
  stepper1->table_counter=table_offset; // Start partway through the lookup table
  stepper1->pos_current = pos_start;
  stepper1->dur_extra = dur_extra;

  stepper2->reset_state();
  stepper2->pos_start = pos2;  // Need to save this for when we turn around
  stepper2->pos_current = pos2;
  stepper2->table_counter=-(STEPPER2_START-pos2);  // Index into the table. This one is one-to-one with step position
  stepper2->dur_extra = dur_extra2;

  // Stepper 4 horizontal
  if (horiz_end < HORIZ_SENTINEL) {
    stepper4->prepare_move( horiz_end,2.0*1000000.0, 1); //2 second
    stepper4->dur_extra=0.0; // move at normal_speed
    stepper4->start_move();
  }      

  // Stepper 3 is handled in here:
  sweep_to(
       (signed long) pos_end,
       (signed long) STEPPER2_END, // (will turn around and go back to start)
       (signed long) -pos3,
      duration_usec, 2);
}

void sweep_to(signed long pos1, signed long pos2, signed long pos3, unsigned long duration, int mode) {
  sweep_continue(pos1, pos2, pos3, duration, mode);

  sweep_start_time=millis();

  step_trace_counter=0;
  pos_curr=0;
  sweep_snap_interval=(unsigned long)CAMERA_SNAP_INTERVAL_VERTICAL;
  sweep_snap_time=sweep_start_time-sweep_snap_interval; // So it'll trigger immediately on entry
  in_sweep=1; // so main loop knows we are sweeping
}

void sweep_continue(signed long pos1, signed long pos2, signed long pos3, unsigned long duration, int mode) {
  unsigned long now=micros();
  Serial.print(now);
  Serial.print(" Sweep ");
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
}

void sweep_horizontal(signed long pos, unsigned long duration, int mode) {
  unsigned long now=micros();
  Serial.print(now);
  Serial.print(" SweepH ");
  Serial.print(pos);
  Serial.print(" ");
  Serial.println(duration);

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
      SWEEP_TIME_SEC_V*1000000.0, 1); 
}

void sweep_to_zero() {
  stepper1->table_counter=0; // reset to start of LUT. Not totally accurate, but avoids going past table.
  stepper2->table_counter=0;
  stepper3->table_counter=0;
  
   sweep_to(
       (signed long) 0,
       (signed long) 0,
       (signed long) 0,
      SWEEP_TIME_SEC_V*1000000.0, 0); 
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
      SWEEP_TIME_SEC_V*1000000.0, 2);
}

void horiz_sweep_start() {
  sweep_horizontal(
       (signed long) (STEPPER4_START*STEPPER4_STEPS_PER_UNIT),
       SWEEP_TIME_SEC_H*1000000.0,1);
}

void horiz_sweep_end() {
  sweep_horizontal(
       (signed long) (STEPPER4_END*STEPPER4_STEPS_PER_UNIT),
       SWEEP_TIME_SEC_H*1000000.0,2);
}

void horiz_sweep_zero() {
  sweep_horizontal(
       (signed long) 0,
       SWEEP_TIME_SEC_H*1000000.0,3);
}
