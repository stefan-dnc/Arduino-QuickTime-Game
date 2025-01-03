// SLAVE
#include <Arduino.h>
#include <SPI.h>

#define BTN_PL1 2
#define BTN_PL2 3
#define R1 4
#define G1 5
#define B1 6
#define R2 7
#define G2 8
#define B2 9

#define DUMMY 255
#define PL1_BTN_SIG 10
#define PL2_BTN_SIG 20
#define TIMEOUT_SIG 128

#define R_MIN 910
#define R_MAX 970
#define G_MIN 830
#define G_MAX 890
#define B_MIN 680
#define B_MAX 740

// SPI variables
volatile byte receivedByte, responseByte;

void player1_ISR();
void player2_ISR();
void setRGB(int player, int color);
void ledCycle(byte c1, byte c2);
byte mapColor(int analogData);

ISR(SPI_STC_vect) {
    receivedByte = SPDR;

    // checking if the received byte is not a dummy byte
    if(SPDR != DUMMY) {
        Serial.print("Slave received value: ");
        Serial.print(receivedByte);
        Serial.println("");
    }
    
    // checking if the received byte is a color code
    if(receivedByte >= 5 && receivedByte <= 15) {
        byte col1 = receivedByte & 3; // the last 2 bits
        byte col2 = (receivedByte >> 2) & 3; // the previous 2 bits
        ledCycle(col1, col2);
    } else if(receivedByte == TIMEOUT_SIG) {
        setRGB(1, 0);
        setRGB(2, 0);
    }

    if(receivedByte == 4 || (receivedByte >= 129 && receivedByte <= 131)) {
        setRGB(1, receivedByte);
        setRGB(2, receivedByte);
    }
}

void setup() {
    Serial.begin(9600);
    pinMode(SCK, INPUT);
    pinMode(MOSI, INPUT);
    pinMode(MISO, OUTPUT);
    pinMode(SS, INPUT);
    SPI.attachInterrupt();
    SPCR |= _BV(SPE); // enable SPI in slave mode
    attachInterrupt(digitalPinToInterrupt(BTN_PL1), player1_ISR, RISING);
    attachInterrupt(digitalPinToInterrupt(BTN_PL2), player2_ISR, RISING);
    for(int i=R1; i<=B2; i++) {
        pinMode(i, OUTPUT);
    }
}

void loop() {
    // Serial.println("SLAVE ALIVE");
    // setRGB(1, value) where value was sent by the master
}

void player1_ISR() {
    int readVal = analogRead(A0);
    Serial.print("Player 1: ");
    Serial.print(readVal);
    Serial.println("");
    byte colorCode = mapColor(readVal);
    responseByte = PL1_BTN_SIG | (colorCode << 5); // setting the player 1 bit and the color code
    SPDR = responseByte;
}

void player2_ISR() {
    int readVal = analogRead(A1);
    Serial.print("Player 2: ");
    Serial.print(readVal);
    Serial.println("");
    byte colorCode = mapColor(readVal);
    responseByte = PL2_BTN_SIG |(colorCode << 5); // setting the player 2 bit and the color code
    SPDR = responseByte;
}

void setRGB(int player, int color) {
    int r, g, b;
    if(player == 1) {
        r = 4; g = 5; b = 6;
    } else if(player == 2) {
        r = 7; g = 8; b = 9;
    } else {
        Serial.println("Player select error");
        return;
    }
    Serial.print("Player ");
    Serial.print(player);
    Serial.println("");
    Serial.print("Color ");
    Serial.print(color);
    Serial.println("");
    Serial.println("");
    switch(color) {
        case 0:
            digitalWrite(r, LOW);
            digitalWrite(g, LOW);
            digitalWrite(b, LOW);
            break;
        case 1:
            digitalWrite(r, HIGH);
            digitalWrite(g, LOW);
            digitalWrite(b, LOW);
            break;
        case 2:
            digitalWrite(r, LOW);
            digitalWrite(g, HIGH);
            digitalWrite(b, LOW);
            break;
        case 3:
            digitalWrite(r, LOW);
            digitalWrite(g, LOW);
            digitalWrite(b, HIGH);
            break;
        case 4:
            digitalWrite(r, HIGH);
            digitalWrite(g, HIGH);
            digitalWrite(b, HIGH);
            break;
        case 129:
            digitalWrite(r, HIGH);
            digitalWrite(g, HIGH);
            digitalWrite(b, LOW);
            break;
        case 130:
            digitalWrite(r, HIGH);
            digitalWrite(g, LOW);
            digitalWrite(b, HIGH);
            break;
        case 131: 
            digitalWrite(r, LOW);
            digitalWrite(g, HIGH);
            digitalWrite(b, HIGH);
            break;
        default:
            Serial.println("Color select error.");
            // digitalWrite(r, HIGH);
            // digitalWrite(g, LOW);
            // digitalWrite(b, HIGH);
            return;
    }
}

void ledCycle(byte c1, byte c2) {
    setRGB(1, c1);
    setRGB(2, c2);
    // while(responseByte != TIMEOUT_SIG);
}

byte mapColor(int analogData) {
    // mapping the analog data to the color codes
    if(analogData >= R_MIN && analogData <= R_MAX) return 1;
    else if(analogData >= G_MIN && analogData <= G_MAX) return 2;
    else if(analogData >= B_MIN && analogData <= B_MAX) return 3;
    else return 0;
}