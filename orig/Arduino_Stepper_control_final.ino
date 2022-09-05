
 #include "pins_arduino.h"
/////variables/////
int val1 = 0;               // variable to store motorPot1 analog value
int val2 = 0;               // variable to store motorPot2 analog value
int val3 = 0;               // variable to store motorPot3 analog value
bool tog1=0;
bool tog2=0;
bool tog3=0;
int start1;
int start2;
int start3;
const int DEBOUNCE_DELAY = 50;   //initially set delay to 50mS
/////direction push button debounce function variables/////
int dir1Stable = LOW;               
int dir1Flicker = LOW;
int dir1Current;
unsigned long lastDebounceTime1 = 0;   
int dir2Stable = LOW;
int dir2Flicker = LOW;
int dir2Current;
unsigned long lastDebounceTime2 = 0;      
int dir3Stable = LOW;
int dir3Flicker = LOW;
int dir3Current;
unsigned long lastDebounceTime3 = 0;
/////Limit switch debounce function variables/////
int lim1Current;
int lim1Stable = LOW;
int lim1Flicker = LOW;
int lim1Flag = LOW;
unsigned long lastDebounceTime4 = 0;
int lim2Current;
int lim2Stable = LOW;
int lim2Flicker = LOW;
int lim2Flag = LOW;
unsigned long lastDebounceTime5 = 0;
int lim3Current;
int lim3Stable = LOW;
int lim3Flicker = LOW;
int lim3Flag = LOW;
unsigned long lastDebounceTime6 = 0;

int m1running = LOW;    //Motor on flag
int m2running = LOW;    //Motor on flag
int m3running = LOW;    //Motor on flag
 /////analog inputs/////
int motorPot1 = 0;     // Analog input, select the input pin for the potentiometer
int motorPot2 = 1;     // Analog input, select the input pin for the 2nd potentiometer
int motorPot3 = 2;     // Analog input, select the input pin for the 3rd potentiometer
/////******digital I/O******/////
int pulseOut1 = 2;     //d2 is motor1 OUTPUT pulses
int dirOut1 = 3;       //d3 is motor1 direction OUTPUT 
int pulseOut2 = 4;     //d4 is motor2 OUTPUT pulses
int dirOut2 = 5;       //d5 is motor2 direction OUTPUT
int m1go = 6;          //button INPUT to turn on motor1 
int m2go = 7;          //button INPUT to turn on motor2
int m1dir = 8;         //switch INPUT for motor1 direction 
int m2dir = 9;         //switch INPUT for motor2 direction
int pulseOut3 = 10;    //d10 is motor3 OUTPUT pulses
int dirOut3 = 11;      //d11 is motor3 direction OUTPUT
int m3go = 12;         //button INPUT to turn on motor3
int m3dir = 13;        //switch INPUT for motor3 direction 
int limit1 = 17;       //d17 is limit switch1 INPUT
int limit2 = 18;       //d18 is limit switch1 INPUT
int limit3 = 19;       //d19 is limit switch1 INPUT

void setup(){
  pinMode(pulseOut1, OUTPUT);   // Set pin#2 as output - motor1 pulses
  pinMode(dirOut1, OUTPUT);     // Set pin#3 as output - direction bit  
  pinMode(pulseOut2, OUTPUT);   // Set pin#4 as output - motor2 pulses
  pinMode(dirOut2, OUTPUT);     // Set pin#5 as output - direction bit
  pinMode(m1go, INPUT);         // Set pin#6 as an INPUT - motor1 start button
  pinMode(m2go, INPUT);         // Set pin#7 as an INPUT - motor2 start button
  pinMode(m1dir, INPUT);        // Set pin#8 as an INPUT - direction push button
  pinMode(m2dir, INPUT);        // set pin#9 as an INPUT - direction push button
  pinMode(pulseOut3, OUTPUT);   // Set pin#10 as output - motor3 pulses
  pinMode(dirOut3, OUTPUT);     // Set pin#11 as output - direction bit
  pinMode(m3go, INPUT);         // Set pin#12 as an INPUT - motor3 start button
  pinMode(m3dir, INPUT);        // Set pin#13 as an INPUT - direction push button  
  pinMode(limit1, INPUT);       // Set pin# 17 as INPUT - imit switch input, axis1
  pinMode(limit2, INPUT);       // Set pin# 18 as INPUT - limit switch input, axis2
  pinMode(limit2, INPUT);       // Set pin# 19 as INPUT - limit switch input, axis3 
}

void loop() {
Limit1();           //call limit switch 1 read and debounce function
Limit2();           //call limit switch 2 read and debounce function
Limit3();           //call limit switch 3 read and debounce function
direction1();       //call direction switch 1 button read and debounce function
direction2();       //call direction switch 2 button read and debounce function
direction3();       //call direction switch 3 button read and debounce function

/////////// Check to see if any axis has hit a limit switch ///////////
if (lim1Flag == HIGH && m1running == HIGH){  // if limit switch is hit and the motor is moving
  noTone(pulseOut1);                      //turn off tone fucntion = turn off motor pulse output, 
  toggle1();                          //call direction toggle function = flip the direction output level
  while (lim1Flag == HIGH){           // wait untill the motor moves off the limit shitch
    delay(1000);                      
    motor1pulses();                  //call step pulse function, allow motor to move off limit seithc                     
    Limit1();                        //call limit switch function to allow  reset while loop condition
  }  
}
if (lim2Flag == HIGH && m2running == HIGH){  //limit switch is hit and the motor is moving
  noTone(pulseOut2);                         //turn off motor pulse output
  toggle1();                                 //flip the direction output level
  while (lim2Flag == HIGH){                  // wait untill the motor moves off the limit shitch
    delay(1000);                      
    motor1pulses();                          //call step pulse function, allow motor to move off limit seithc                     
    Limit2();                                //call limit switch function to allow  reset while loop condition
  }  
}
if (lim3Flag == HIGH && m3running == HIGH){  //limit switch is hit and the motor is moving
  noTone(pulseOut3);                         //turn off motor pulse output
  toggle1();                                 //flip the direction output level
  while (lim3Flag == HIGH){                  // wait untill the motor moves off the limit shitch
    delay(1000);                      
    motor1pulses();                          //call step pulse function, allow motor to move off limit seithc                     
    Limit3();                                //call limit switch function to allow  reset while loop condition
  }  
}
else{
motor1pulses();                 //call step pulse function - motor 1
}
}
///END MAIN CODE///

//can only have the tone function running on one pin at a time, therefore only one motor 
//can operate at a time.

void motor1pulses(){
start1 = digitalRead(m1go);   //read motor push button 
start2 = digitalRead(m2go);
start3 = digitalRead(m3go);
    if (start1 == HIGH && start2 == LOW && start3 == LOW) { //only one "tone" commmand at a time
    val1 = analogRead(motorPot1);          // read the value from the speed potentiometer
    val1 = map(val1, 0, 1023, 0, 30000);   // extend the value of the pot from 1024max to 30k
    tone(pulseOut1, val1);                 //send pulses to motor1 driver
    m1running = HIGH;                      //set motor running flag
    }
    else if (start1 == LOW) {
      noTone(pulseOut1);  //stop pulses
      m1running = LOW;   //motor is not running
    }
    if (start2 == HIGH && start1 == LOW && start3 == LOW) { //only one "tone" commmand at a time
    val2 = analogRead(motorPot2);    // read the value from the sensor
    val2 = map(val2, 0, 1023, 0, 30000); 
    tone(pulseOut2, val2);
    m2running = HIGH;
    }
    else if (start2 == LOW) {
      noTone(pulseOut2);
      m2running = LOW;
    }
    if (start3 == HIGH && start2 == LOW && start1 == LOW) { //only one "tone" commmand at a time
    val3 = analogRead(motorPot3);    // read the value from the sensor
    val3 = map(val3, 0, 1023, 0, 30000); 
    tone(pulseOut3, val3);
    }
    else if (start3 == LOW) {
      noTone(pulseOut3);
    }
}
/////////motor direction toggle functions one for each direction bit//////////
void toggle1(){
      tog1 = digitalRead(dirOut1);  //read value of output bit 'dirOut1'
      digitalWrite(dirOut1, !tog1);  //flip output bit'dirOut1'
}
void toggle2(){
      tog2 = digitalRead(dirOut2);  //read value of output bit 'dirOut2'
      digitalWrite(dirOut2, !tog2);  //flip output bit'dirOut2' 
}
void toggle3(){
      tog3 = digitalRead(dirOut3);  //read value of output bit 'dirOut2'
      digitalWrite(dirOut3, !tog3);  //flip output bit'dirOut2' 
}
  /////////Limit switch debounce, read value, & set flag functions/////////
void Limit1(){ 
lim1Current = digitalRead(limit1);
if (lim1Current != lim1Flicker){
  lastDebounceTime4 = millis();
  lim1Flicker = lim1Current;
}
if ((millis() - lastDebounceTime4) > DEBOUNCE_DELAY){
  if (lim1Stable == HIGH && lim1Current == LOW)
    lim1Flag = HIGH;
    lim1Stable = lim1Current;}
    else {
    lim1Flag = LOW;                  //return lim1Flag;
    }
}
void Limit2(){ 
lim2Current = digitalRead(limit2);
if (lim2Current != lim2Flicker){
  lastDebounceTime5 = millis();
  lim2Flicker = lim2Current;
}
if ((millis() - lastDebounceTime5) > DEBOUNCE_DELAY){
  if (lim2Stable == HIGH && lim2Current == LOW)
    lim2Flag = HIGH;
    lim2Stable = lim2Current;}
    else {
    lim2Flag = LOW;                  //return lim2Flag;
    }
}
void Limit3(){ 
lim3Current = digitalRead(limit3);
if (lim3Current != lim3Flicker){
  lastDebounceTime6 = millis();
  lim3Flicker = lim3Current;
}
if ((millis() - lastDebounceTime6) > DEBOUNCE_DELAY){
  if (lim3Stable == HIGH && lim3Current == LOW)
    lim3Flag = HIGH;
    lim3Stable = lim3Current;}
    else {
    lim3Flag = LOW;                  //return lim3Flag;
    }
}
/////////Diredtion button debounce functions - calls toggle function/////////
void direction1 (){ 
dir1Current = digitalRead(m1dir);
 if (dir1Current != dir1Flicker){
  lastDebounceTime1 = millis();
  dir1Flicker = dir1Current;
}
if ((millis() - lastDebounceTime1) > DEBOUNCE_DELAY){
  if (dir1Stable == HIGH && dir1Current == LOW)
    toggle1();              //flip output bit'dir1'
    dir1Stable = dir1Current;
 }
}
void direction2 (){
dir2Current = digitalRead(m2dir);
if (dir2Current != dir2Flicker){
  lastDebounceTime2 = millis();
  dir2Flicker = dir2Current;
}
if ((millis() - lastDebounceTime2) > DEBOUNCE_DELAY){
  if (dir2Stable == HIGH && dir2Current == LOW)
    toggle2();
    dir2Stable = dir2Current;
}
}
void direction3 (){
dir3Current = digitalRead(m3dir);
if (dir3Current != dir3Flicker){
  lastDebounceTime3 = millis();
  dir3Flicker = dir3Current;
}
if ((millis() - lastDebounceTime3) > DEBOUNCE_DELAY){
  if (dir3Stable == HIGH && dir3Current == LOW)
    toggle3();
    dir3Stable = dir3Current;
}
}
