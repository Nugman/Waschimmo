#include <Servo.h>

Servo armServo;  // create servo object to control a servo
Servo liftServo;
Servo brushServo;
const int motorPin=2;         // red
const int pumpPin=3;          // green
const int armLiftPin=4;       // yellow
const int armServoPin=5;      // 
const int brushPin=6;         // blue
const int inputPlus=A0;
const int inputMinus=A1;
const int inputEnter=A2;
const int inputBack=A3;
void setup() {
  // put your setup code here, to run once:
  // setup relais
  digitalWrite(motorPin, HIGH);
  digitalWrite(pumpPin, HIGH);
  pinMode(motorPin, OUTPUT);
  pinMode(pumpPin, OUTPUT);
  pinMode(inputPlus, INPUT_PULLUP);
  pinMode(inputMinus, INPUT_PULLUP);
  pinMode(inputEnter, INPUT_PULLUP);
  pinMode(inputBack, INPUT_PULLUP);
  armServo.attach(armServoPin);
  armServo.write(0);
  liftServo.attach(armLiftPin);
  liftServo.write(0);
  brushServo.attach(brushPin);
  brushServo.write(0);
}
void loop()
{
  int pos = 0;
  for(pos = 0; pos < 40; pos += 1)  
  {
    brushServo.write(pos);
    delay(20);
  }
  delay(2000);
  for(pos = 40; pos>=0; pos-=1)     // goes from 180 degrees to 0 degrees
  {                               
    brushServo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(20);                       // waits 15ms for the servo to reach the position
  }
  delay(5000);
}
