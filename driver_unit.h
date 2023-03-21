// This header file specifies the settings for the Arduino board inside the
// unit built by Marty, with an Arduino, 3 drivers, and front-panel knobs and
// buttons..

// these are mostly copied from "Arduino_Stepper_control_final.ino"


 /////analog inputs/////
#define motorPot1 0     // Analog input, select the input pin for the potentiometer
#define motorPot2 1     // Analog input, select the input pin for the 2nd potentiometer
#define motorPot3 2     // Analog input, select the input pin for the 3rd potentiometer

/////******digital I/O******/////
// Outputs to motors:
#define DRIVER1_PULSE  14     //d2 is motor1 OUTPUT pulses
#define DRIVER1_DIR  15       //d3 is motor1 direction OUTPUT 
#define DRIVER2_PULSE 4     //d4 is motor2 OUTPUT pulses
#define DRIVER2_DIR 5       //d5 is motor2 direction OUTPUT
#define DRIVER3_PULSE 10    //d10 is motor3 OUTPUT pulses
#define DRIVER3_DIR 11      //d11 is motor3 direction OUTPUT

#define DRIVER4_PULSE  2     //d2 is motor1 OUTPUT pulses
#define DRIVER4_DIR  3       //d3 is motor1 direction OUTPUT 

// Inputs from front panel:
#define m1go 6          //button INPUT to turn on motor1 
#define m2go 7          //button INPUT to turn on motor2
#define m1dir 8         //switch INPUT for motor1 direction 
#define m2dir 9         //switch INPUT for motor2 direction
#define m3go 12         //button INPUT to turn on motor3
#define m3dir 13        //switch INPUT for motor3 direction 

#define limit1 17       //d17 is limit switch1 INPUT
#define limit2 18       //d18 is limit switch1 INPUT
#define limit3 19       //d19 is limit switch1 INPUT
