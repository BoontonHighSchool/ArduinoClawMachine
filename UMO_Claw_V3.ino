/*
  +---------------------------------+-----------+---------+
  | Ultimaker Original Claw Machine | Version 3 | 11/2022 |
  +---------------------------------+-----------+---------+
  | Claw machine using stepper motors and arcade joystick.|
  | Pin Connections: Arduino MEGA Ultimaker Shield_v1.5.4 |
  +---------------+----+----+----+----+----+--------------+
  |   Steppers    | X  | Y  | Z  | E  | A  |
  +---------------+----+----+----+----+----+
  | Min Pin       | 22 | 26 | 30 |    |    |
  | Max Pin       | 24 | 28 | 32 |    |    |
  | Stepper Pin   | 25 | 31 | 37 | 43 | 49 |
  | Direction Pin | 23 | 33 | 39 | 45 | 47 |
  | Enable Pin    | 27 | 29 | 35 | 41 | 48 |
  +---------------+----+----+----+----+----+
  |   X & Y Motors use X & Y Pins          |
  |   Z Motor uses E Pins                  |
  +----------------------------------------+
  |   Claw connected to Pin 6 (Via Relay)  |
  +----------------------------------------+
  |   Joystick & Pushbutton Controls       |
  +------+-------+-------+------+------+---+--+
  | Left | Right | Front | Back | Claw | Coin |
  +------+-------+-------+------+------+------+
  |   38 |    40 |    50 |   42 |   6  |   5  |
  +------+-------+-------+------+------+------+
  |  LCD Display |  SDA  =  20  |  SCL =  21  |        
  +--------------+--------------+-------------+----+
  |1.5.4 Expansion slots:                          |
  |https://reprap.org/wiki/Ultimaker%27s_v1.5.4_PCB|
  +------------------------------------------------+
*/

#include <ezButton.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <BigFont01_I2C.h>
#include <SimpleTimer.h>

// LCD Display
LiquidCrystal_I2C lcd(0x27, 16, 2); //address, columns, rows
BigFont01_I2C     big(&lcd); // construct large font object, passing to it the name of our lcd object

// Direction Pins
const int xDirectionPin = 23;
const int yDirectionPin = 33;
const int zDirectionPin = 45;

// Stepper Pins
const int xStepPin = 25;
const int yStepPin = 31;
const int zStepPin = 43;

// Claw Motor
const int Claw = 6; // HIGH = Close & LOW = Open
const int Height = 6600; // Z Stepper Amount

// Limit Switches
const int xMin = 22; // X Limit Switch Min (Rightmost)
const int xMax = 24; // X Limit Switch Max (Leftmost)
const int yMin = 26;
const int yMax = 28;
const int Coin = 5;
const int LED_PIN = 3; // Used to test Coin & canPlay state
boolean xMinState = false; // X Limit Switch Min current state
boolean xMaxState = false; // X Limit Switch Max current state
boolean yMinState = false;
boolean yMaxState = false;

ezButton coinIn(Coin);
int canPlay = LOW; //LED pin 3 to test

// Controls
const int btnLeft = 38;
const int btnRight = 40;
const int btnFront = 50;
const int btnBack = 42;
const int btnClaw = 52;
boolean btnLeftPressed = false;
boolean btnRightPressed = false;
boolean btnFrontPressed = false;
boolean btnBackPressed = false;
boolean btnClawPressed = false;

// Timer
SimpleTimer sTimer(1000); // 1 Second Timer
int countDown = 40;  // Play time in seconds
//unsigned long lastTick;

void setup() {

  Serial.begin(9600);
  Serial.println("Starting Claw Machine");

  pinMode(xDirectionPin, OUTPUT);
  pinMode(yDirectionPin, OUTPUT);
  pinMode(zDirectionPin, OUTPUT);
  pinMode(xStepPin, OUTPUT);
  pinMode(yStepPin, OUTPUT);
  pinMode(zStepPin, OUTPUT);
  pinMode(Claw, OUTPUT);
  pinMode(btnLeft, INPUT_PULLUP);
  pinMode(btnRight, INPUT_PULLUP);
  pinMode(btnFront, INPUT_PULLUP);
  pinMode(btnBack, INPUT_PULLUP);
  pinMode(btnClaw, INPUT_PULLUP);


  pinMode(xMin, INPUT_PULLUP);
  pinMode(xMax, INPUT_PULLUP);
  pinMode(yMin, INPUT_PULLUP);
  pinMode(yMax, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();
  big.begin();
  lcd.clear();

  coinIn.setDebounceTime(50);
  digitalWrite(zDirectionPin, LOW);
  clawRise(false, Height);
  clawHome(true);
  printName();
  lcd.clear();
  printInsert();
}

void loop() {
  unsigned long currentMillis = millis();
  coinIn.loop();
  if (coinIn.isPressed()) {
    displayStart();
    Serial.print("Coin Inserted");
    canPlay = !canPlay;
    sTimer.reset();
    digitalWrite(LED_PIN, canPlay);
  }
  if (canPlay == 1) {

    if (sTimer.isReady()) {
      if (countDown > 0) {
        countDown--;
        sTimer.reset();
      }
      displayCountdown();
      if (countDown == 0) {
        timeout();
      }
    }
    readButtons();
    actOnButtons();
  }
}

void displayCountdown() {
  int secs = countDown;
  big.writeint(0, 9, secs, 2, true);
}

void displayStart() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Time");
  lcd.setCursor(0, 1);
  lcd.print("Left");
}

void printName() {
  int ms = 2000;
  lcd.clear(); big.writechar(0, 1, 'T'); big.writechar(0, 4, 'H'); big.writechar(0, 7, 'E');
  delay(ms);
  lcd.clear(); big.writechar(0, 2, 'C'); big.writechar(0, 5, 'L'); big.writechar(0, 8, 'A'); big.writechar(0, 11, 'W');
  delay(ms);
}

void printInsert() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Insert");
  lcd.setCursor(2, 1);
  lcd.print("Coin");
  big.writechar(0, 11, 'n');

}

void readButtons() {

  btnRightPressed = false;
  btnLeftPressed = false;
  btnFrontPressed = false;
  btnBackPressed = false;
  btnClawPressed = false;

  xMinState = false;
  xMaxState = false;
  yMinState = false;
  yMaxState = false;

  if (digitalRead(btnLeft) == LOW) {
    btnLeftPressed = true;
  }
  if (digitalRead(btnRight) == LOW) {
    btnRightPressed = true;
  }
  if (digitalRead(btnFront) == LOW) {
    btnFrontPressed = true;
  }
  if (digitalRead(btnBack) == LOW) {
    btnBackPressed = true;
  }
  if (digitalRead(btnClaw) == LOW) {
    btnClawPressed = true;
  }
  if (digitalRead(xMin) == LOW) {
    xMinState = true;
  }
  if (digitalRead(xMax) == LOW) {
    xMaxState = true;
  }
  if (digitalRead(yMin) == LOW) {
    yMinState = true;
  }
  if (digitalRead(yMax) == LOW) {
    yMaxState = true;
  }
}

void actOnButtons() {
  if (btnClawPressed == true) {
    lcd.clear();
    digitalWrite(zDirectionPin, HIGH);
    clawDrop(true, Height);
    digitalWrite(zDirectionPin, LOW);
    clawRise(false, Height);
  }
  if ((btnLeftPressed == true) && (xMaxState == false)) {
    digitalWrite(xDirectionPin, LOW);
    xStep(true, 15);
  }
  else if ((btnRightPressed == true) && (xMinState == false)) {
    digitalWrite(xDirectionPin, HIGH);
    xStep(false, 15);
  }
  else if ((btnFrontPressed == true) && (yMinState == false)) {
    digitalWrite(yDirectionPin, LOW);
    yStep(true, 15);
  }
  else if ((btnBackPressed == true) && (yMaxState == false)) {
    digitalWrite(yDirectionPin, HIGH);
    yStep(false, 15);
  }

}

void xStep(boolean dir, int steps) {
  for (int i = 0; i < steps; i++) {
    digitalWrite(xStepPin, HIGH);
    delayMicroseconds(800);
    digitalWrite(xStepPin, LOW);
    delayMicroseconds(800);
  }
}

void yStep(boolean dir, int steps) {
  for (int i = 0; i < steps; i++) {
    digitalWrite(yStepPin, HIGH);
    delayMicroseconds(800);
    digitalWrite(yStepPin, LOW);
    delayMicroseconds(800);
  }
}

void clawDrop(boolean dir, int steps) {
  for (int i = 0; i < steps; i++) {
    digitalWrite(zStepPin, HIGH);
    delayMicroseconds(800);
    digitalWrite(zStepPin, LOW);
    delayMicroseconds(800);
  }
  delay(2000);
  digitalWrite(Claw, HIGH);
  delay(500);

}
void clawRise(boolean dir, int steps) {
  for (int i = 0; i < steps; i++) {
    digitalWrite(zStepPin, HIGH);
    delayMicroseconds(800);
    digitalWrite(zStepPin, LOW);
    delayMicroseconds(800);
  }
  delay(2000);
  clawHome(true);
}

void timeout() {
  lcd.clear();
  digitalWrite(zDirectionPin, HIGH);
  clawDrop(true, Height);
  digitalWrite(zDirectionPin, LOW);
  clawRise(false, Height);
}

void clawHome(boolean dir) {
  digitalWrite(xDirectionPin, HIGH);
  do {
    digitalWrite(xStepPin , HIGH);
    delayMicroseconds(800);
    digitalWrite(xStepPin , LOW);
    delayMicroseconds(800);
  }
  while (digitalRead(xMin) == HIGH);
  delay(500);
  digitalWrite(yDirectionPin, LOW);
  do {
    digitalWrite(yStepPin , HIGH);
    delayMicroseconds(800);
    digitalWrite(yStepPin , LOW);
    delayMicroseconds(800);
  }
  while (digitalRead(yMin) == HIGH);
  delay(500);
  digitalWrite(Claw, LOW);
  canPlay = LOW;
  digitalWrite(LED_PIN, canPlay);
  Serial.println("Insert Coin");
  printInsert();
  countDown = 40;
}
