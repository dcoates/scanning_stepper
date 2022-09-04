#include "stepper.h"
#include "driver_unit.h"

// The first of each of these numbers depends on the DIP switch settings (steps/rev)
// The second depends on the units (1 revolution=360 deg, X mm, etc..)
#define STEPPER1_STEPS_PER_UNIT (4000.0/360.0) /* Degrees */
#define STEPPER2_STEPS_PER_UNIT (4000.0/360.0) /* MM  TODO Check */
#define STEPPER3_STEPS_PER_UNIT (4000.0/360.0) /* MM  TODO Check */
#define STEPPER4_STEPS_PER_UNIT (4000.0/360.0) /* MM */

#define STEPPER1_START -5.0
#define STEPPER2_START -1.0
#define STEPPER3_START
#define STEPPER4_START

#define STEPPER1_END 5.0
#define STEPPER2_END 1.0
#define STEPPER3_END
#define STEPPER4_END

#define SWEEP_TIME_SEC 5.0

//// ----------------------------

// These are shared between legacy.ino and this file
unsigned long lastDebounceTime1 = 0; 
unsigned long lastDebounceTime2 = 0; 
unsigned long lastDebounceTime3 = 0; 

StepperState* stepper1;
StepperState* stepper2;
StepperState* stepper3;
StepperState* stepper4;

unsigned int any_sweeping=0;
  
void setup() {
//#if DEBUG_STEPPER
    Serial.begin(9600); 
//#endif

    legacy_setup(); // Call legacy code. Sets pin directions, mainly.

    stepper1 = new StepperState(DRIVER1_PULSE,DRIVER1_DIR);
    stepper2 = new StepperState(DRIVER2_PULSE,DRIVER2_DIR); 
    //stepper3 = new StepperState(DRIVER3_PULSE,DRIVER3_DIR); 
    //stepper4 = new StepperState(DRIVER4_PULSE,DRIVER4_DIR); 

    //stepper1->prepare_move( int(STEPPER1_START*STEPPER1_STEPS_PER_UNIT),SWEEP_TIME_SEC*1000000.0);
    //stepper2->prepare_move( int(STEPPER2_START*STEPPER2_STEPS_PER_UNIT),SWEEP_TIME_SEC*1000000.0);
    //stepper3->prepare_move( int(STEPPER3_START*STEPPER3_STEPS_PER_UNIT),SWEEP_TIME_SEC*1000000.0);
    //stepper4->prepare_move( int(STEPPER4_START*STEPPER4_STEPS_PER_UNIT),SWEEP_TIME_SEC*1000000.0);

    //stepper1->start_move();
    //stepper2->start_move();
    //stepper3->start_move();
    //stepper4->start_move();
}

void loop() {
      stepper1->do_update();
      stepper2->do_update();
      //stepper3->do_update();
      //stepper4->do_update();

      any_sweeping = (stepper1->sweeping || stepper2->sweeping ); // & stepper3->sweeping & stepper4->sweeping;
      //Serial.print(any_sweeping);

      legacy_loop(); // main loop from old front panel

      if (lastDebounceTime1>3000) { //Hold for 3 seconds 
        Serial.println("HOLD1");
        //stepper1->prepare_move( int(STEPPER1_START*STEPPER1_STEPS_PER_UNIT),SWEEP_TIME_SEC*1000000.0);
        //stepper2->prepare_move( int(STEPPER2_START*STEPPER2_STEPS_PER_UNIT),SWEEP_TIME_SEC*1000000.0);
        //stepper1->start_move();
        //stepper2->start_move();
      };
      if (lastDebounceTime2>3000) { //Hold for 3 seconds 
        Serial.println("HOLD2");
        //stepper1->prepare_move( int(0.0*STEPPER1_STEPS_PER_UNIT),SWEEP_TIME_SEC*1000000.0);
        //stepper2->prepare_move( int(0.0*STEPPER2_STEPS_PER_UNIT),SWEEP_TIME_SEC*1000000.0);
        //stepper1->start_move();
        //stepper2->start_move();
      };
      if (lastDebounceTime3>3000) { //Hold for 3 seconds 
        Serial.println("HOLD3");
        //stepper1->prepare_move( int(STEPPER1_STOP*STEPPER1_STEPS_PER_UNIT),SWEEP_TIME_SEC*1000000.0);
        //stepper2->prepare_move( int(STEPPER2_STOP*STEPPER2_STEPS_PER_UNIT),SWEEP_TIME_SEC*1000000.0);
        //stepper1->start_move();
        //stepper2->start_move();
      };


}
