#include <Wire.h>
#include "Adafruit_VEML6070.h"
#include <Adafruit_BMP280.h>
#include <AlertNodeLib.h>   // to use XBee serial communications

/**
 * ALERTHOME SET UP
 */
const String myNodeName = "SMART WINDOW";
// USING pin2 for TX pin3 for RX
AlertNode myNode;

/**
 * GLOBAL VARIABLES
 */
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
int waterSensor = 1;
int DANGER_TEMP = 60;  // temp given in degrees celcius;
int MAX_TEMP = 30; // max temp is the temp in celcius the user defines and wants to be alerted if exceeded USER DEFINED
int MIN_TEMP = 9; // minimal temperature in degrees celcius below which the window closes USER DEFINED



//uncomment this line if using a Common Anode LED
#define COMMON_ANODE


void setup() {
  
  /**
   * General Set Up
   */
  Serial.begin(9600);
  uv.begin(VEML6070_1_T);  // pass in the integration time constant
  if (!bmp.begin()) {  
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    while (1);
  }
  /**
   * Set Up for RGB Led
   */
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

/**
 *  METHODS TO DEAL WITH COMUNICATION
 *  PROTOCOL: AlertHome
 */
void listenForAlerts(){
  int alert = myNode.alertReceived();
  if (alert != AlertNode::NO_ALERT) {

    Serial.print("*** Alert received: ");
    Serial.print(alert);
    Serial.print(" ");
    Serial.println(myNode.alertName(alert));
     if (alert == AlertNode::FIRE) {
       openWindow(); // Action
     }else if(alert == AlertNode::FLOOD){
      closeWindow();
     }else if(alert == AlertNode::BURGLARY){
      closeWindow();
     }else if(alert == AlertNode::GAS){
      openWindow(); // Action
     }
  }
}
/**
 * @param : type : type of alert to broadcast 0 < type < 1
 */
void broadcastAlerts(int type){
  int alert;
  if (type == 0){ // flood
     alert = AlertNode::FLOOD;
  }else if(type == 1){
    alert = AlertNode::FIRE; // fire
  }
  myNode.sendAlert(alert);

    Serial.print("*** Alert sent: ");
    Serial.print(alert);
    Serial.print(" ");
    Serial.println(myNode.alertName(alert));
}
/**
 *  METHODS TO HANDLE SENSOR READINGS CONVERSION AND STANDARDIZATION
 */

void categorizeUVReading(int index){
  if (index >= 11){
    setColor(128,0,128);
  }else if (index >= 8 && index <= 10){
    setColor(255,0,0);
  }else if (index >= 6 && index <= 7){
    setColor(125,123,31);
  }else if (index >= 3 && index <= 5){
    setColor(255,255,40);
  }else{
    setColor(50,121,8);
  }
 }

 int convertUVReading(int raw){
  if ( raw >= 2055 ) {
    // max reading
    return 11;
  }else if(raw >= 1867 && raw <= 2054){
    return 10;
  }else if(raw >= 1682 && raw <= 1868){
    return 9;
  }else if(raw >= 1494 && raw <= 1681){
    return 8;
  }else if(raw >= 1308 && raw <= 1493){
    return 7;
 }else if(raw >= 1121 && raw <= 1307){
    return 6;
  }else if(raw >= 933 && raw <= 1120){
    return 5;
  }else if(raw >= 746 && raw <= 932){
    return 4;
  }else if(raw >= 560 && raw <= 932){
    return 3;
  }else if(raw >= 372 && raw <= 560){
    return 2;
  }else if(raw >= 185 && raw <= 371){
    return 1;
  }else {
    return 0;
  }
 }

 int categorizeWaterLevel(int raw){
  if (raw > 100){
    return 5;
  }else if(raw > 80 && raw <= 100 ){
    return 4;
  }else if(raw > 60 && raw <= 80){
    return 3;
  }else if(raw > 40 && raw <= 60){
    return 2;
  }else if(raw > 20 && raw <= 40){
    return 1;
  }else if(raw >= 0 && raw <= 20){
    return 0;
  }
 }


 
/**
 *  METHODS TO HANDLE SENOSR READINGS
 */

int readUVLight(){
  int sum = 0;
  for(int i=0;  i < 10; i++){
    int reading = uv.readUV();
    int readingStandardized = convertUVReading(reading);
    Serial.println(reading);
    sum += readingStandardized;
    delay(100);
  }
  int avg = sum / 10;
  categorizeUVReading(avg);
  return avg;
}

int readTemp(){
  int sum = 0;
  for(int i=0;  i < 10; i++){
    sum += bmp.readTemperature();
    delay(100);
  }
  int avg = sum / 10;
  return avg;
}

int readWaterLevel(){
  int sum = 0;
  for(int i=0;  i < 10; i++){
    int waterLevel = analogRead(waterSensor);
    sum += waterLevel;
    delay(100);
  }
  int avg = sum / 10;
  return avg;
}

/**
 *  METHODS TO HANDLE RESPONSE TO SENSOR READINGS
 */
void openWindow(){
  
  if (!iSWINDOWOPEN){
    Serial.println("Window opening");
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
  
  if (iSWINDOWOPEN){
    Serial.println("Window closing");
    analogWrite(enableM1, SPEED);
    digitalWrite(in1Pin, LOW);
    digitalWrite(in2Pin, HIGH);
    delay(5000);
    analogWrite(enableM1, 0);
    iSWINDOWOPEN = false;
  }
  
}

void flood(){
  Serial.println("Floood !!!!!");
  for(int i =0; i < 20; i++){
    setColor(249, 74, 62);
    delay(100);
    setColor(64, 180, 209);
    delay(100);
  }
}

void fire(){
  Serial.println("Fire !!!!!");
  for(int i =0; i < 20; i++){
    setColor(236, 62, 249);
    delay(100);
    setColor(242, 14, 41);
    delay(100);
  }
}

void flashLED(){
  Serial.println("Quick Warning !!!!!");
  for(int i=0; i < 10; i++){
    digitalWrite(ledPin, HIGH);
    delay(50);
    digitalWrite(ledPin, LOW);
    delay(50);
    }
  }

void tempAlert(){
  Serial.println("Desired temp exceeded !!!!!");
  for(int i=0; i < 30; i++){
    digitalWrite(ledPin, HIGH);
    delay(50);
    digitalWrite(ledPin, LOW);
    delay(50);
    }
}

void loop() {
  // >>> LISTEN FOR ALERTS <<<<
  listenForAlerts();
  
  // >>> SENSING <<<<
  int lightLevel = readUVLight();
  int tempReading = readTemp();
  int val = readWaterLevel(); // read water val 
  int valCat = categorizeWaterLevel(val);
  Serial.print("UV Light index : ");
    Serial.println(lightLevel);
  Serial.print("Temp reading :");
    Serial.println(tempReading);
  Serial.print("Water Level Index :");
    Serial.println(valCat);
    
    // >>> ARCTUATING <<<<
    
    if (valCat >= 4){
      flood();
      // broadcast
      broadcastAlerts(0);
    }
     // >>> ARCTUATING - TEMP -> LED + RGB LED  <<<<
     
    if(tempReading >= DANGER_TEMP) {
      // danger temp is always > max temp
      fire();
      // broadcast
      broadcastAlerts(1);
    }else if(tempReading >= MAX_TEMP) {
      tempAlert();
    }
    // >>> ARCTUATING - UV LIGHT + TEMP + WATER  -> MOTOR -> { WINDOW }  <<<<
    
    if ( lightLevel > 1 && tempReading > MIN_TEMP && valCat < 1){
     // if sunlight + normal temp + no rainfall -> open window
     openWindow();
    }else{
      // else close in rain conditions or cold conditions or nigh fall.
      closeWindow();
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
