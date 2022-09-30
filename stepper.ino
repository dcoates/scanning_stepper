#include "stepper.h"
#include "driver_unit.h"
#include "digitalWriteFast.h"

// In theory, the first of each of these numbers depends on the DIP switch settings (steps/rev)
// The second depends on the units (1 revolution=360 deg, X mm, etc..). BUT
// The relationship isn't so clear based on the other mechanical stages, so expressing as steps
// (and measuring empirically) is more straightforward.
#define STEPPER1_STEPS_PER_UNIT (200.0/2.0) /* Degrees. Empirical: not sure why "/4" */
#define STEPPER2_STEPS_PER_UNIT 1
// Old stepper2: (8000.0/4.0)  
#define STEPPER3_STEPS_PER_UNIT 1
// Old stepper3 DIP(8000.0/4.0)
//#define STEPPER4_STEPS_PER_UNIT (1600.0/4.0) /* MM */

// Zero/middle point for Stepper1 is at 1300 steps away from "0" (due to non-linear scissors)
// Zero/middle point for Stepper2?
// Zero/middle point for Stepper3 is zero

// Where to begin the sweep. Button press left moves from "0" here
#define STEPPER1_START (-35+13) //-65.0 // -65.0
#define STEPPER2_START -3200
#define STEPPER3_START -2000
//#define STEPPER4_START

// Where to sweep until. Button press right cases sweep until value is reached
#define STEPPER1_END (35 + 13)
#define STEPPER2_END 3200
#define STEPPER3_END 2000
//#define STEPPER4_END

// -30 to +30 : 30.36mm

#define SWEEP_TIME_SEC 3.0
#define BUTTON_HOLD_MS 1500
//// ----------------------------

// These are shared between legacy.ino and this file
// So that we can peek at the buttons
unsigned long lastDebounceTime1 = 0; 
unsigned long lastDebounceTime2 = 0; 
unsigned long lastDebounceTime3 = 0; 
int dir1Current;
int dir2Current;
int dir3Current;

// class instances for each stepper motor
StepperLUT* stepper1;
StepperLUT* stepper2;
StepperConstant* stepper3;

// One motor instance will write into the trace buffer:
#define TRACE_BUF_SIZE 64
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

void setup() {

    Serial.begin(9600);
    Serial.println("ready");
    
    legacy_setup(); // Call legacy setup code. Sets pin directions, mainly.

    stepper1 = new StepperLUT(1, DRIVER1_PULSE,DRIVER1_DIR);
    stepper2 = new StepperLUT(2, DRIVER2_PULSE,DRIVER2_DIR); 
    stepper3 = new StepperConstant(3, DRIVER3_PULSE,DRIVER3_DIR); 
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

}

void loop() {
  any_sweeping = (stepper1->sweeping || stepper2->sweeping || stepper3->sweeping); // & stepper3->sweeping & stepper4->sweeping;
    
  if (!any_sweeping) {
      // No sweep happening: do legacy (manual) ops, serial debugging, check for hold

      interrupts();
 
      // Check for serial commands
      if (Serial.available() > 0) {
        // read the incoming byte:
        int incomingByte = Serial.read();

        if (incomingByte=='1') {
          sweep_to_start();
        } else if (incomingByte=='2') {
          sweep_to_stop();
        } else if (incomingByte=='0') {
          sweep_to_zero();
        } else if (incomingByte=='?') {
          debug_blast();
        }
      }
  
    legacy_loop(); // main loop from old front panel for manual ops

    // Are any buttons held to sweep?
    unsigned long now = millis();
    if (((now - lastDebounceTime1)>BUTTON_HOLD_MS) && dir1Current && (!any_sweeping) ) {
      sweep_to_start();
    } else if (((now - lastDebounceTime2)>BUTTON_HOLD_MS) && dir2Current && (!any_sweeping) ) { 
      sweep_to_zero();
    } else if (((now - lastDebounceTime3)>BUTTON_HOLD_MS) && dir3Current && (!any_sweeping) ) { 
      sweep_to_stop();
    } 
  } else { // In a sweep
    // Failsafe: touch right GO button to stop. Don't even debounce: bail immediately if any button action.
    if (digitalRead(m3go)==HIGH) {
      stepper1->stop_move(1);
      stepper2->stop_move(1);
      stepper3->stop_move(1); 
      Serial.println("FAILSAFE STOP");     
    } else {
    
      noInterrupts();
  
      // Move if there is a confirmed sweep happening.
      stepper1->do_update();
      stepper2->do_update();
      stepper3->do_update();
      //stepper4->do_update();
      };
  
      interrupts();
  };
}

void sweep_to_start() {
  Serial.println("SwStart");
  step_trace_counter=0;
  stepper1->prepare_move(  (signed long) (STEPPER1_START*STEPPER1_STEPS_PER_UNIT),SWEEP_TIME_SEC*1000000.0);
  stepper2->prepare_move(  (signed long) (STEPPER2_START*STEPPER2_STEPS_PER_UNIT),SWEEP_TIME_SEC*1000000.0);
  stepper3->prepare_move(  (signed long) (STEPPER3_START*STEPPER3_STEPS_PER_UNIT),SWEEP_TIME_SEC*1000000.0);
  stepper1->start_move();
  stepper2->start_move();
  stepper3->start_move();
}

void sweep_to_zero() {
  step_trace_counter=0;
  stepper1->prepare_move(  (signed long) (0.0*STEPPER1_STEPS_PER_UNIT),SWEEP_TIME_SEC*1000000.0);
  stepper2->prepare_move(  (signed long) (0.0*STEPPER2_STEPS_PER_UNIT),SWEEP_TIME_SEC*1000000.0);
  stepper3->prepare_move(  (signed long) (0.0*STEPPER3_STEPS_PER_UNIT),SWEEP_TIME_SEC*1000000.0);
  stepper1->start_move();
  stepper2->start_move();
  stepper3->start_move();
}

void sweep_to_stop() {
  Serial.println("SwStop");
  step_trace_counter=0;
  stepper1->prepare_move(  (signed long) (STEPPER1_END*STEPPER1_STEPS_PER_UNIT),SWEEP_TIME_SEC*1000000.0);
  stepper2->prepare_move(  (signed long) (STEPPER2_END*STEPPER2_STEPS_PER_UNIT),SWEEP_TIME_SEC*1000000.0);
  stepper3->prepare_move(  (signed long) (STEPPER3_END*STEPPER3_STEPS_PER_UNIT),SWEEP_TIME_SEC*1000000.0);
  stepper1->start_move();
  stepper2->start_move();
  stepper3->start_move();
}
