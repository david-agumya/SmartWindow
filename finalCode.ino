#include <Wire.h>
#include "Adafruit_VEML6070.h"
#include <Adafruit_BMP280.h>

#define BMP_SCK 13
#define BMP_MISO 12
#define BMP_MOSI 11
#define BMP_CS 10

Adafruit_VEML6070 uv = Adafruit_VEML6070();
Adafruit_BMP280 bmp(BMP_CS); // hardware SPI

/**
 * Set up for RGB Diode
 */
int bluePin = 6;
int greenPin = 5;
int redPin = 3;
/*
 * Set Up for Motor 
 */
int enableM1 = 9;    // connected to Pin 1 on motor to enable motor and control speed
int in1Pin = 8;      // connected to input 1  on H Bridge 
int in2Pin = 7;       // connected to input 2 on H Bridge
int SPEED = 500;      // standard speed of the motor
boolean iSWINDOWOPEN = false;
int ledPin = 4; 



//uncomment this line if using a Common Anode LED
#define COMMON_ANODE


void setup() {
  Serial.begin(9600);
  Serial.println("VEML6070 Test");
  uv.begin(VEML6070_1_T);  // pass in the integration time constant
  if (!bmp.begin()) {  
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    while (1);
  }
  // RGB
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT); 
  pinMode(ledPin, OUTPUT);
  /*
  * Set Up for Motor 
   */
  pinMode(in1Pin, OUTPUT);
  pinMode(in2Pin, OUTPUT);
  pinMode(enableM1, OUTPUT);
  
}

/*
 * HELPER FUNCTIONS 
 */
 /**
 * When called turn anticlockwise to open window
 */
void openWindow(){
  if (!iSWINDOWOPEN){
    analogWrite(enableM1, SPEED);
    digitalWrite(in1Pin, HIGH);
    digitalWrite(in2Pin, LOW);
    delay(5000);
    analogWrite(enableM1, 0);
    iSWINDOWOPEN = true;
  }
}
/**
 * When called turn clockwise to close window
 */
void closeWindow(){
  Serial.println(iSWINDOWOPEN, "Window open");
  if (iSWINDOWOPEN){
    analogWrite(enableM1, SPEED);
    digitalWrite(in1Pin, LOW);
    digitalWrite(in2Pin, HIGH);
    delay(5000);
    analogWrite(enableM1, 0);
    iSWINDOWOPEN = false;
  }
  
}

void flood(){
  for(int i =0; i < 20; i++){
    setColor(249, 74, 62);
    delay(100);
    setColor(64, 180, 209);
    delay(100);
  }
}

void fire(){
  for(int i =0; i < 20; i++){
    setColor(236, 62, 249);
    delay(100);
    setColor(242, 14, 41);
    delay(100);
  }
}

void loop() {

  int lightLevel = uv.readUV();
  int tempReading = bmp.readTemperature();
  int val = analogRead(1); // read water val 
  Serial.println(val);
   if (val > 100){
    flood();
   }else{
    digitalWrite(ledPin, LOW);
   }
   
   Serial.println(val);
  if (tempReading > 20){
    
    fire();
    openWindow();
    
  }else if (tempReading < 10 ) {
    closeWindow();
  }else if(tempReading > 30){
    for(int i = 0; i < 20; i++){
      digitalWrite(ledPin, HIGH);
      delay(500);
      digitalWrite(ledPin, LOW);
    }
  }
  Serial.println(lightLevel);
  if (lightLevel >= 0 && lightLevel <= 2){
    setColor(66, 244, 107);  // green
    delay(5000);
    closeWindow();
  }else if(lightLevel >= 3 && lightLevel <= 5){
    setColor(244, 241, 65);  // yellow
    delay(5000);
    openWindow();
    
  }else if(lightLevel >= 6 && lightLevel <= 7){
    setColor(249, 174, 99);  // orange
    delay(5000);
    openWindow();
  }else if(lightLevel >= 8 && lightLevel <= 10){
    setColor(242, 75, 53);  // red
    delay(5000);
    openWindow();
  }else{
    // greater than 11
    setColor(2, 93, 249);  // purple
    delay(5000);
    openWindow();
  }
}


void setColor(int red, int green, int blue)
{
  #ifdef COMMON_ANODE
    red = 255 - red;
    green = 255 - green;
    blue = 255 - blue;
  #endif
  analogWrite(redPin, red);
  analogWrite(greenPin, green);
  analogWrite(bluePin, blue);  
}
