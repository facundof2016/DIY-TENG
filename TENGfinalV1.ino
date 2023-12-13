#include <LiquidCrystal.h>

//GPIO pins
const byte rs = 0;         //pin for LCD
const byte en = 1;         //pin for LCD
const byte d4 = 3;         //pin for LCD
const byte d5 = 4;         //pin for LCD
const byte d6 = 5;         //pin for LCD
const byte d7 = 6;         //pin for LCD
const byte dirPin = 9;     //5v output pin for motor driver direction
const byte stepPin = 10;   //5v output pin for motor driver pulse
const byte switchPin = 2;  //pin for homing limit switch

//configurable settings
const int strokeSteps = 2275;         //number of steps for full stroke, 1600 steps=72mm, 22.222 steps=1mm
const int homeDel = 500;              //delay time between each motor step during homing sequence
const int homingSteps = 1590;          //steps taken in direction away from limit switch after switch has been activated
const byte maxSpeed = 40;             //max speed expressed as the microseconds per pulse divided by 2
unsigned long updateInterval = 50;    //time between updating potentiometer reads in ms
int fullSteps = (.96 * strokeSteps);  //amount of stroke executed at full speed
int accelSteps = (strokeSteps / 50);  //amount of stroke executed while accelerating/decelerating

//required variables
int speedPot;                //integer for potentiometer read value 0 to 1023, analog read for speed pot
int delPot;                  //integer for potentiometer read value 0 to 1023, anaolog read for stroke delay pot
int stepDel;                 //delay time between each motor step during operation, lower value=faster steps
unsigned long strokeDel;     //delay time between actuator strokes
byte speedPerc;              //percent maximum speed of stepper motor, varies based on pot, used to define stepDel
int oldStepDel;              //the value of stepDel as it is displayed on LCD
int oldStrokeDel;            //the value of strokeDel as it is displayed on LCD
unsigned long lastRead = 0;  //timer set at end of potentiometer read
unsigned long endStroke;     //timer for end of stroke
bool pushPull = 1;           //changes dirPin from 0 to 1 after each stroke. Starts 1 (HIGH/true) to push carrier away from limit switch after homing

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);  //setup lcd object using variable defined pins


void setup() {

  pinMode(dirPin, OUTPUT);
  pinMode(stepPin, OUTPUT);
  pinMode(switchPin, INPUT_PULLUP);

  lcd.begin(16, 2);  //start lcd with 16 columns and 2 rows

  //indicate start of homing sequence on LCD
  lcd.setCursor(0, 0);
  lcd.print("Homing...");


  //set direction towards limit switch for homing
  digitalWrite(dirPin, LOW);

  //while limit switch is not pressed, step motor towards the limit switch for homing
  while (digitalRead(switchPin)==LOW) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(homeDel);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(homeDel);
  }

  //pause after limit switch is hit
  delay(500);

  lcd.setCursor(0, 0);
  lcd.print("Centering...");

  //set direction away from limit switch
  digitalWrite(dirPin, HIGH);

  //step motor away from limit switch, distance set by homingSteps
  for (int x = 0; x < homingSteps; x++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(homeDel);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(homeDel);
  }

  //pause before starting operation
  delay(500);

}

void loop() {

  //if the time between the last potentiometer read and LCD update is greater than the designated updateInterval, read and update LCD
  if (millis() - lastRead >= updateInterval) {
    speedPot = analogRead(A0);                        //read value from potentiometer for speed
    delPot = analogRead(A1);                          //read value from potentiometer for stroke delay
    speedPerc = (5 * map(speedPot, 0, 1023, 2, 20));  //map pot value to a percentage between 30% and 100% rounded to nearest 5%
    stepDel = (100 * maxSpeed / (speedPerc));         //sets step delay time based on speedPerc, 100%=125 microsecond delay. 50%=250 microsecond delay
    strokeDel = (50 * map(delPot, 0, 1023, 1, 40));   //sets stroke delay between 50ms and 1000ms to nearest 50

    //if strokeDel or stepDel values have changed, refresh LCD
    if (strokeDel != oldStrokeDel || stepDel != oldStepDel) {
      //writes speed and stroke delay time to lcd
      lcd.clear();
      lcd.setCursor(0, 0);        //set lcd cursor start of first line
      lcd.print("Speed (%):  ");  //print "Speed %:" on lcd
      lcd.print(speedPerc);       //print speedPerc value after "Speed %:"
      lcd.setCursor(0, 1);        //set lcd cursor start of second line
      lcd.print("Delay (ms): ");
      lcd.print(strokeDel);      //print stroke delay time in ms
      oldStrokeDel = strokeDel;  //update old values
      oldStepDel = stepDel;      //update old values
    }
    lastRead = millis();  //reset timer for lastRead
  }

  //if strokeDel time has passed, start new stroke
  if (millis() - endStroke >= strokeDel) {

    //set direction based on pushPull bool which switches after each stroke. Must start off as true/1 in order to push away from limit switch before pulling
    digitalWrite(dirPin, pushPull);

    //accelerate motor for 2% of full stroke length
    for (int x = 0; x < accelSteps; x++) {
      digitalWrite(stepPin, HIGH);
      delayMicroseconds((2 * (accelSteps - x)) + stepDel); //acceleration formula starts at 2*accelSteps and approaches 0 as x increments from 0 to accelSteps
      digitalWrite(stepPin, LOW);
      delayMicroseconds((2 * (accelSteps - x)) + stepDel);
    }
    //perform 96% of full stroke at full speed
    for (int x = 0; x < fullSteps; x++) {
      digitalWrite(stepPin, HIGH);
      delayMicroseconds(stepDel);
      digitalWrite(stepPin, LOW);
      delayMicroseconds(stepDel);
    }
    //decelerate for 2% of stroke length
    for (int x = accelSteps; x > 0; x--) {
      digitalWrite(stepPin, HIGH);
      delayMicroseconds((2 * (accelSteps - x)) + stepDel); //deceleration formula starts at 0 and approaches 2*accelSteps as x decrements from accelSteps to 0
      digitalWrite(stepPin, LOW);
      delayMicroseconds((2 * (accelSteps - x)) + stepDel);
    }

    endStroke = millis();  //set timer for end time of stroke
    pushPull = !pushPull;  //reverse direction for next loop
  }
}