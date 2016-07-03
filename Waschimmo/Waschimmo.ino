
/* 
 *  Plattenwaschmaschine 
 *  Waschimmo Automatic
 *  
 *  Waschimmo ist lizenziert unter einer Creative Commons Namensnennung - Nicht-kommerziell - Weitergabe unter gleichen Bedingungen 4.0 International Lizenz.
 *  http://www.waschimmo.de/
 *  
 */
#include <Wire.h> 
#include <EEPROM.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>
#include <MenuSystem.h>

/*
 * EEPROM Memory Map
 * Programmierte Settings werden im EEPROM gespeichert. Folgende Adressen werden verwendet:
 * #0000 Waschzeit (1-6)
 * #0001 RPM (33|45|78)
 * #0002 Plattengroße (1-180)
 */

// Menu variables
MenuSystem ms;
Menu m("Waschimmo");
MenuItem m1("Start");
MenuItem m2("Schlauch trockn.");
MenuItem m3("Parken");
Menu m4("Einstellungen");
MenuItem m4_m1("Waschzeit");
MenuItem m4_m2("Drehzahl");
MenuItem m4_m3("Plattengr\357sse");

// init LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

Servo armServo;
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

int washingTime = EEPROM.read(0);    // minutes
int rpm = EEPROM.read(1);
int innerPos = 96;     // inner arm positon (right before the record label)
int outerPos = EEPROM.read(2);      // outer arm position (record edge)

int brushPos=0;

char lastChar;

void setup() {
  armServo.attach(armServoPin);
  armServo.write(0);

  liftServo.attach(armLiftPin);
  liftServo.write(0);

  brushServo.attach(brushPin);
  brushServo.write(0);


  // setup relais
  digitalWrite(motorPin, HIGH);
  digitalWrite(pumpPin, HIGH);
  pinMode(motorPin, OUTPUT);
  pinMode(pumpPin, OUTPUT);


  pinMode(inputPlus, INPUT_PULLUP);
  pinMode(inputMinus, INPUT_PULLUP);
  pinMode(inputEnter, INPUT_PULLUP);
  pinMode(inputBack, INPUT_PULLUP);

  lcd.begin();
  // Print a message to the LCD.
  lcd.print("Waschimmo");
  lcd.setCursor(0,1);
  lcd.print("V 1.02");
  delay(5000);

  // set up menu
  m.add_item(&m1, &washMain);    // Start
  m.add_item(&m2, &dryPipe);     // Schlauch trocknen
  m.add_item(&m3, &park);        // Park brush
  m.add_menu(&m4);               // Setup
    m4.add_item(&m4_m1, &setTime);        // Wschzeit
    m4.add_item(&m4_m2, &setRpm);         // RPM
    m4.add_item(&m4_m3, &setRecordSize);  // Plattengröße

  ms.set_root_menu(&m);
  displayMenu();
}

void loop() {
  menuHandler();
  delay(100);
}

void displayMenu() {
  lcd.clear();
  lcd.setCursor(0,0);
  // Display the menu
  Menu const* cp_menu = ms.get_current_menu();

  //lcd.print("Current menu name: ");
  lcd.print(cp_menu->get_name());
  
  lcd.setCursor(0,1);
  
  lcd.print(cp_menu->get_selected()->get_name());
}

void menuHandler() {
  char inChar;
  if((inChar = buttonRead())>0) { 
    switch (inChar) {
    case '-': // Previous item
      ms.prev();
      displayMenu();
      break;
    case '+': // Next item
      ms.next();
      displayMenu();
      break;
    case 'b': // Back pressed
      ms.back();
      displayMenu();
      break;
    case 'e': // Enter pressed
      ms.select();
      displayMenu();
      break;
    default:
      break;
    }
  }
}


char buttonRead() {
  char inChar;
  inChar=0;
  if (digitalRead(inputPlus) == LOW) {
     inChar = '+';
  } else if (digitalRead(inputMinus) == LOW) {
     inChar = '-';
  } else if (digitalRead(inputEnter) == LOW) {
     inChar = 'e';
  } else if (digitalRead(inputBack) == LOW) {
     inChar = 'b';
  }
  delay(75);
  return(inChar);
}

// Menu callback functions
void washMain(MenuItem* p_menu_item) {
  int armPos = 0;
  
  lcd.clear();
  lcd.print("Fl\365ssigkeit");
  lcd.setCursor(0,1);
  lcd.print("auftragen");

  liftBrush();
  delay(1000);

    // turn on motor
  digitalWrite(motorPin,LOW);
  delay(2000);   
    // wait for 10s
  for (int x=10; x > -1; x--){
    lcd.setCursor(11,1);
    lcd.print(x);
    delay(250);
    lcd.setCursor(13,1);
    lcd.print(".");
    delay(250);
    lcd.setCursor(14,1);
    lcd.print(".");
    delay(250);
    lcd.setCursor(15,1);
    lcd.print(".");
    delay(250);
    lcd.setCursor(10,1);
    lcd.print("      ");
  }
  delay(2000);
  lcd.clear();
  lcd.print("Waschen");

    // lower brush
  lowerBrush();
      
  for (int minutes=washingTime-1; minutes>=0; minutes--){
    for (int seconds=59; seconds>=0; seconds--){
      lcd.setCursor(0,1);
      lcd.print(minutes);
      lcd.print(":");
      (seconds < 10) ? lcd.print("0") : NULL;
      lcd.print(seconds);
      delay(1000);
    }
  }
  
    // lift brush
  liftBrush();
  delay(2000);

  lcd.clear();
  lcd.print("Trocknen");

    // raise arm
  liftArm();
  delay(2000);

    // move in arm
  for (int armPos = 0; armPos <= innerPos; armPos += 1) { // goes from 0 degrees to 180 degrees
      // in steps of 1 degree
    armServo.write(armPos);             // tell servo to go to position in variable 'armPos'
    delay(50);                          // waits 50ms for the servo to reach the position
  }

    // turn on pump and wait for the airflow
  digitalWrite(pumpPin,LOW);  
  delay(5000);

    
    // lower arm
  lowerArm();

  for (int armPos = innerPos; armPos >= outerPos; armPos -= 1) { // goes from inner to outer position
      // in steps of 1 degree
    armServo.write(armPos);              // tell servo to go to position in variable 'armPos'
    delay(60000/rpm);                    // suck 1 rotation
  }
  delay(60000/rpm);                      // stay at run-in for another rotation


    // raise arm
  liftArm();
  delay(500);
    // turn off pump
  digitalWrite(pumpPin,HIGH);  
  delay(500);

    // move out arm
  for (int armPos = outerPos; armPos >= 0; armPos -= 1) { // goes from outer to parking position 
      // in steps of 1 degree
    armServo.write(armPos);             // tell servo to go to position in variable 'armPos'
    delay(50);                          // waits 50ms for the servo to reach the position
  }
  armServo.write(0);

    // park arm
  lowerArm();

    // turn off motor
  delay(5000);
  digitalWrite(motorPin,HIGH);
  lcd.clear();
  lcd.print("Ready");
    
}

void park(MenuItem* p_menu_item){
  if (brushPos>0) {
    lowerBrush();
  }
}

void dryPipe(MenuItem* p_menu_item){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Trockne Schlauch");
    // turn on pump 
  digitalWrite(pumpPin,LOW);  
  for (int minutes=1; minutes>=0; minutes--){
    for (int seconds=59; seconds>=0; seconds--){
      lcd.setCursor(0,1);
      lcd.print(minutes);
      lcd.print(":");
      (seconds < 10) ? lcd.print("0") : NULL;
      lcd.print(seconds);
      delay(1000);
    }
  }
  digitalWrite(pumpPin,HIGH);  
}


// Setup

void setTime(MenuItem* pMenuItem) {
  char inChar;
  int newTime;
  newTime = washingTime;
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Waschzeit:");
  lcd.setCursor(0,1);
  lcd.print(newTime);
  lcd.setCursor(3,1);
  lcd.print("min.");

  while (1) {
    if((inChar = buttonRead())>0) { 
      switch (inChar) {
      case '+': 
        newTime++;
        newTime = min(newTime,8);
        lcd.setCursor(0,1);
        lcd.print(" ");
        lcd.setCursor(0,1);
        lcd.print(newTime);
        delay(150);
        break;
      case '-': 
        newTime--;
        newTime = max(newTime,0);
        lcd.setCursor(0,1);
        lcd.print(" ");
        lcd.setCursor(0,1);
        lcd.print(newTime);
        delay(150);  
        break;
      case 'b': // Back pressed
        ms.back();
        displayMenu();
        return;
        break;
      case 'e': // Enter pressed
        washingTime = newTime;
        EEPROM.update(0, washingTime);
        lcd.setCursor(0,1);
        lcd.print("Speichern");
        delay(1000);

        ms.back();
        displayMenu();
        return;

        break;
      default:
        break;
      }
    }
  }
}

void setRpm(MenuItem* pMenuItem) {
  char inChar;
  int newRpmIndex;
  int newRpm[] = {33, 45, 78};
  if (rpm == 78) {
    newRpmIndex = 2;
  } else if (rpm == 45) {
    newRpmIndex = 1;
  } else {
    newRpmIndex = 0;
  }
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Drehzahl:");
  lcd.setCursor(0,1);
  lcd.print(newRpm[newRpmIndex]);
  lcd.setCursor(4,1);
  lcd.print("U/min");

  while (1) {
    if((inChar = buttonRead())>0) { 
      switch (inChar) {
      case '+': 
        newRpmIndex++;
        newRpmIndex = min(newRpmIndex,2);
        lcd.setCursor(0,1);
        lcd.print("  ");
        lcd.setCursor(0,1);
        lcd.print(newRpm[newRpmIndex]);
        delay(150);
        break;
      case '-': 
        newRpmIndex--;
        newRpmIndex = max(newRpmIndex,0);
        lcd.setCursor(0,1);
        lcd.print("  ");
        lcd.setCursor(0,1);
        lcd.print(newRpm[newRpmIndex]);
        delay(150);  
        break;
      case 'b': // Back pressed
        ms.back();
        displayMenu();
        return;
        break;
      case 'e': // Enter pressed
        rpm = newRpm[newRpmIndex];
        EEPROM.update(1, newRpm[newRpmIndex]);
        lcd.setCursor(0,1);
        lcd.print("Speichern");
        delay(1000);

        ms.back();
        displayMenu();
        return;

        break;
      default:
        break;
      }
    }
  }
}

void setRecordSize(MenuItem* p_menu_item){
  char inChar;
  int newPos;
  newPos = outerPos;
  
  lcd.clear();

    // raise arm
  liftArm();
  delay(2000);

    // move in arm
  for (int pos = 0; pos <= outerPos; pos += 1) { // goes from 0 degrees to 180 degrees
      // in steps of 1 degree
    armServo.write(pos);                // tell servo to go to position in variable 'pos'
    delay(50);                          // waits 50ms for the servo to reach the position
  }

  lcd.setCursor(0,0);
  lcd.print("+/- Position");
  lcd.setCursor(0,1);
  lcd.print(newPos);

  while (1) {
    if((inChar = buttonRead())>0) { 
      switch (inChar) {
      case '+': 
        newPos++;
        newPos = min(newPos,innerPos);
        armServo.write(newPos);
        lcd.setCursor(0,1);
        lcd.print("    ");
        lcd.setCursor(0,1);
        lcd.print(newPos);
        delay(150);
        break;
      case '-': 
        newPos--;
        newPos = max(newPos,0);
        armServo.write(newPos);
        lcd.setCursor(0,1);
        lcd.print("    ");
        lcd.setCursor(0,1);
        lcd.print(newPos);
        delay(150);  
        break;
      case 'b': // Back pressed
          // move out arm
        for (int pos = outerPos; pos >= 0; pos -= 1) { // goes from outer to parking position 
            // in steps of 1 degree
          armServo.write(pos);                // tell servo to go to position in variable 'pos'
          delay(50);                          // waits 50ms for the servo to reach the position
        }
        armServo.write(0);
      
          // park arm
        lowerArm();

        ms.back();
        displayMenu();
        return;
        break;
      case 'e': // Enter pressed
        outerPos = newPos;
        EEPROM.update(2, outerPos);
        lcd.setCursor(0,1);
        lcd.print("Speichern");
        delay(1000);

          // move out arm
        for (int pos = outerPos; pos >= 0; pos -= 1) { // goes from outer to parking position 
            // in steps of 1 degree
          armServo.write(pos);                // tell servo to go to position in variable 'pos'
          delay(50);                          // waits 50ms for the servo to reach the position
        }
        armServo.write(0);
      
          // park arm
        lowerArm();
        
        ms.back();
        displayMenu();
        return;

        break;
      default:
        break;
      }
    }
  }
}

void liftArm() {
  for(int pos = 0; pos < 40; pos += 1)
  {
    liftServo.write(pos);
    delay(25);
  }
}

void lowerArm() {
  for(int pos = 40; pos>=0; pos-=1)
  {                               
    liftServo.write(pos);
    delay(25);
  }
}  

void liftBrush() {
  for(brushPos; brushPos < 90; brushPos += 1)  
  {
    brushServo.write(brushPos);
    delay(25);
  }
}

void lowerBrush() {
  for(brushPos; brushPos>=0; brushPos-=1)
  {
    brushServo.write(brushPos);
    delay(25);
  }
}

