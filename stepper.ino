#include "stepper.h"
#include "driver_unit.h"

// The first of each of these numbers depends on the DIP switch settings (steps/rev)
// The second depends on the units (1 revolution=360 deg, X mm, etc..)
#define STEPPER1_STEPS_PER_UNIT (3200.0/4.0) /* Degrees. Empirical: not sure why "4" */
#define STEPPER2_STEPS_PER_UNIT (6400.0/4.0) /* MM  TODO Check */
#define STEPPER3_STEPS_PER_UNIT (12800.0/360.0) /* MM  TODO Check */
#define STEPPER4_STEPS_PER_UNIT (4000.0/360.0) /* MM */

#define STEPPER1_START -30.0
#define STEPPER2_START -2.0
#define STEPPER3_START -10.0
#define STEPPER4_START

#define STEPPER1_END 30.0
#define STEPPER2_END 2.0
#define STEPPER3_END 10.0
#define STEPPER4_END

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
StepperState* stepper1;
StepperState* stepper2;
StepperState* stepper3;
StepperState* stepper4;

unsigned int any_sweeping=0;
  
void setup() {
    Serial.begin(9600); 

    legacy_setup(); // Call legacy setup code. Sets pin directions, mainly.

    stepper1 = new StepperState(1, DRIVER1_PULSE,DRIVER1_DIR);
    stepper2 = new StepperState(2, DRIVER2_PULSE,DRIVER2_DIR); 
    stepper3 = new StepperState(3, DRIVER3_PULSE,DRIVER3_DIR); 
    //stepper4 = new StepperState(4, DRIVER4_PULSE,DRIVER4_DIR); 
}

void loop() {
      stepper1->do_update();
      stepper2->do_update();
      stepper3->do_update();
      //stepper4->do_update();

      any_sweeping = (stepper1->sweeping || stepper2->sweeping || stepper3->sweeping); // & stepper3->sweeping & stepper4->sweeping;

      if (!any_sweeping) 
        legacy_loop(); // main loop from old front panel

      if (((millis() - lastDebounceTime1)>BUTTON_HOLD_MS) && dir1Current && (!any_sweeping) ) { //Hold for 3 seconds 
        sweep_to_start();
      } else if (((millis() - lastDebounceTime2)>BUTTON_HOLD_MS) && dir2Current && (!any_sweeping) ) { //Hold for 3 seconds 
        sweep_to_zero();
      } else if (((millis() - lastDebounceTime3)>BUTTON_HOLD_MS) && dir3Current && (!any_sweeping) ) { //Hold for 3 seconds 
        sweep_to_stop();
      } else {
        // NOTHING IS HELD : For now, stop all movements if no held buttons.
        //Serial.println("NOTHING"); 
			  //stepper1->stop_move(0); // 0=don't mess with the pulses, just cancel swept movement
			  //stepper2->stop_move(0);
      }
}

void sweep_to_start() {
  stepper1->prepare_move( int(STEPPER1_START*STEPPER1_STEPS_PER_UNIT),SWEEP_TIME_SEC*1000000.0);
  stepper2->prepare_move( int(STEPPER2_START*STEPPER2_STEPS_PER_UNIT),SWEEP_TIME_SEC*1000000.0);
  stepper3->prepare_move( int(STEPPER3_START*STEPPER3_STEPS_PER_UNIT),SWEEP_TIME_SEC*1000000.0);
  stepper1->start_move();
  stepper2->start_move();
  stepper3->start_move();
}

void sweep_to_zero() {
  stepper1->prepare_move( int(0.0*STEPPER1_STEPS_PER_UNIT),SWEEP_TIME_SEC*1000000.0);
  stepper2->prepare_move( int(0.0*STEPPER2_STEPS_PER_UNIT),SWEEP_TIME_SEC*1000000.0);
  stepper3->prepare_move( int(0.0*STEPPER3_STEPS_PER_UNIT),SWEEP_TIME_SEC*1000000.0);
  stepper1->start_move();
  stepper2->start_move();
  stepper3->start_move();
}

void sweep_to_stop() {
  stepper1->prepare_move( int(STEPPER1_END*STEPPER1_STEPS_PER_UNIT),SWEEP_TIME_SEC*1000000.0);
  stepper2->prepare_move( int(STEPPER2_END*STEPPER2_STEPS_PER_UNIT),SWEEP_TIME_SEC*1000000.0);
  stepper3->prepare_move( int(STEPPER3_END*STEPPER3_STEPS_PER_UNIT),SWEEP_TIME_SEC*1000000.0);
  stepper1->start_move();
  stepper2->start_move();
  stepper3->start_move();
}
