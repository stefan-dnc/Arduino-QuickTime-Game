// MASTER
#include <Arduino.h>
#include <LiquidCrystal.h>
#include <Servo.h>
#include <SPI.h>

#define RS 4
#define EN 5
#define D4 6
#define D5 7
#define D6 8
#define D7 9

#define SERVO_PIN 3

#define DUMMY 255
#define PL1_BTN_SIG 10
#define PL2_BTN_SIG 20
#define TIMEOUT_SIG 128

#define PLAYER_BITMASK 0b00011111 

LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);
Servo motor;

int pl1_pts = 0, pl2_pts = 0; // player points
int servoProgress = 0;

bool gameStarted = false;

unsigned long timeout = 2000;

// pre-declaration of functions
byte sendByte(byte data);
void initialState();
void printPts();
void playRound();
void endGame();

void setup() {
  Serial.begin(9600);

  SPI.begin();
  pinMode(SS, OUTPUT);
  digitalWrite(SS, HIGH); // disable slave

  lcd.begin(16, 2);

  motor.attach(SERVO_PIN);
  motor.write(0);

  randomSeed(analogRead(0));
}

byte l = 0;

void loop() {
  // set servo to 0 degrees
  motor.write(0);

  initialState();
  while(gameStarted) {
    printPts();
    playRound();
    if(servoProgress >= 180) {
      gameStarted = false;
      return;
    }
    delay(500);
  }
  if(servoProgress >= 180)
    endGame();
}

byte sendByte(byte data) {
  // enable slave
  digitalWrite(SS, LOW);

  // send data
  byte slaveResponse = SPI.transfer(data);

  // disable slave
  digitalWrite(SS, HIGH);

  if(data != DUMMY) {
    Serial.print("Sent byte to slave: ");
    Serial.print(data);
    Serial.println("");
  }
  
  return slaveResponse;
}

void initialState() {
  sendByte(TIMEOUT_SIG);

  // display initial message
  lcd.home();
  lcd.print("Press any button");
  lcd.setCursor(0, 1);
  lcd.print("to begin.");
  byte receivedFromSlave = sendByte(DUMMY);
  if ((receivedFromSlave & PLAYER_BITMASK) == PL1_BTN_SIG || (receivedFromSlave & PLAYER_BITMASK) == PL2_BTN_SIG) {
    gameStarted = true;
    sendByte(DUMMY);
  }
}

void printPts() {
  lcd.clear();
  lcd.home();
  lcd.print("Player 1: "); lcd.print(pl1_pts); lcd.print(" pts");
  lcd.setCursor(0, 1);
  lcd.print("Player 2: "); lcd.print(pl2_pts); lcd.print(" pts");
}

void playRound() {
  byte col1 = random(1, 4);
  byte col2 = random(1, 4);
  byte colsToSend = col1 | (col2 << 2);
  byte slaveResponse = sendByte(colsToSend);
  Serial.println("COLOR SENT TO SLAVE");
  unsigned long startTime = millis(); // start time of the round

  do {
    // send a dummy byte to the slave and receive the response
    slaveResponse = sendByte(DUMMY); 
    Serial.print("Slave response: ");
    Serial.print(slaveResponse);
    Serial.println("");
  }
  while ((slaveResponse == DUMMY || !(slaveResponse & 0b01100000)) && millis() - startTime < timeout); // wait for a response from the slave

  if (millis() - startTime >= timeout) {
    sendByte(TIMEOUT_SIG);
    Serial.println("TIMEOUT SIGNAL SENT!");
  } else {
    byte color = (slaveResponse & 0b01100000) >> 5;
    slaveResponse = slaveResponse & PLAYER_BITMASK;
    if(slaveResponse == PL1_BTN_SIG) {
      if(color == col1) {
        Serial.println("Player 1 pressed the correct button");

        // add points to player 1 based on the time taken to press the button
        pl1_pts += (millis() - startTime)/100;
      } else {
        Serial.println("Player 1 pressed the wrong button");
        // subtract points from player 1 if the wrong button was pressed
        pl1_pts -= 20;
        if(pl1_pts < 0)
          pl1_pts = 0;
      }
      sendByte(TIMEOUT_SIG);
    } else if(slaveResponse == PL2_BTN_SIG) {
      if(color == col2) {
        Serial.println("Player 2 pressed the correct button");
        
        // add points to player 2 based on the time taken to press the button
        pl2_pts += (millis() - startTime)/100;
      } else {
        Serial.println("Player 2 pressed the wrong button");
        
        // subtract points from player 2 if the wrong button was pressed
        pl2_pts -= 20;
        if(pl2_pts < 0)
          pl2_pts = 0;
      }
      sendByte(TIMEOUT_SIG);
    } else {
      Serial.println("An error occured! The player bitmask is incorrect.");
      // Serial.print("Slave response: ");
      // Serial.print(slaveResponse);
      // Serial.println("");
    }
  }

  // move the servo
  servoProgress += 18;
  motor.write(servoProgress);
}

void endGame() {
  lcd.clear();
  lcd.home();
  lcd.print("Calculating");
  lcd.setCursor(0, 1);
  lcd.print("score");
  delay(600);

  for(int i=0; i<3; i++) {
    sendByte(129+i);
    lcd.print(".");
    delay(600);
  }

  lcd.clear();
  lcd.home();
  lcd.print("Player 1: ");

  // player 1 points animation
  for(int i=0; i<= pl1_pts; i++) {
    lcd.setCursor(10, 0);
    lcd.print(i);
    lcd.print(" pts");
    delay(50);
  }

  lcd.setCursor(0, 1);

  // player 2 points animation
  lcd.print("Player 2: ");
  for(int i=0; i<= pl2_pts; i++) {
    lcd.setCursor(10, 1);
    lcd.print(i);
    lcd.print(" pts");
    delay(50);
  }

  // blink animation
  for(int i=0; i<3; i++) {
    lcd.noDisplay();
    delay(300);
    lcd.display();
    delay(300);
  }

  delay(1000);
  lcd.home();

  // display the winner
  if(pl1_pts > pl2_pts) {
    for (int i=0; i<16; i++) {
      lcd.clear();
      lcd.setCursor(15-i, 0);
      lcd.print("Player 1 wins!!!");
      sendByte(4);
      delay(200);
    }
    delay(1000);
  } else if(pl1_pts < pl2_pts) {
    for (int i=0; i<16; i++) {
      lcd.clear();
      lcd.setCursor(15-i, 0);
      lcd.print("Player 2 wins!!!");
      sendByte(4);
      delay(200);
    }
    delay(1000);
  } else {
    lcd.clear();
    lcd.home();
    lcd.print("Wow! It's a tie!");
    sendByte(4);
  }
  servoProgress = 0;
  motor.write(servoProgress);
  pl1_pts = 0;
  pl2_pts = 0;
}