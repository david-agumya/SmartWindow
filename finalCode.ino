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
int MAX_TEMP = 20; // max temp is the temp in celcius the user defines and wants to be alerted if exceeded USER DEFINED
int MIN_TEMP = 9; // minimal temperature in degrees celcius below which the window closes USER DEFINED



//uncomment this line if using a Common Anode LED
#define COMMON_ANODE

/**
 * Set up the environment variables. This runs only once.
 */
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

 /**
  * Listen for alerts from the another device with Xbee communication
  * ALERTS that are responded to are
  * 1) FIRE -> Open window to help disperse the flames
  * 2) FLOOD -> Close windows to prevent water getting into the home.
  * 3) BURGLARY -> Close the windows to prevent the burglar from getting away
  */
void listenForAlerts(){
  int alert = myNode.alertReceived();
  if (alert != AlertNode::NO_ALERT) {

    Serial.print("*** Alert received: ");
    Serial.print(alert);
    Serial.print(" ");
    Serial.println(myNode.alertName(alert));
     if (alert == AlertNode::FIRE) {
       openWindow();
     }else if(alert == AlertNode::FLOOD){
      closeWindow();
     }else if(alert == AlertNode::BURGLARY){
      closeWindow();
     }else if(alert == AlertNode::GAS){
      openWindow();
     }
  }
}

/**
 * Given a type of 0 or 1 broadcast the appropriate message =
 * @param : type : type of alert to broadcast 0 < type < 1
 * @type 1 : FIRE incident detected by temp sensor, broadcast this to other smart home devices connected to Xbee radio receivers
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

/**
 * Given a UV index set the color to the corresponding color in the range that the index falls
 * @param index : integer representing the categorized uv index derived from the raw UV light sensor readings.
 */
void categorizeUVReading(int index){
  if (index >= 11){
    setColor(128,0,128); // purple - Extreme
  }else if (index >= 8 && index <= 10){
    setColor(255,0,0);  // red - Very High
  }else if (index >= 6 && index <= 7){
    setColor(125,123,31); // orange - High  
  }else if (index >= 3 && index <= 5){
    setColor(255,255,40);  // yellow - moderate
  }else{
    setColor(50,121,8); // green - low
  }
 }

/**
 * Given a raw UV light index return a category between 0 and 11 based on the range that the raw sensor reading falls
 * @param raw : Integer representing the raw UV light sensor reading.
 * Reference :  Ranges were derived from the UV VEML6070 sensor product specification
 */
 int convertUVReading(int raw){
  if ( raw >=  ) {
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

/**
 * Given a raw sensor reading from the water sensor return a category between 0 and 5 representing water level category.
 * @param raw : Integer represening the raw sensor readings from the water sensor.
 */
 int categorizeWaterLevel(int raw){
  if (raw > 400){
    return 5;
  }else if(raw > 350 && raw <= 400 ){
    return 4;
  }else if(raw > 300 && raw <= 350){
    return 3;
  }else if(raw > 150 && raw <= 300){
    return 2;
  }else if(raw > 100 && raw <= 150){
    return 1;
  }else if(raw >= 0 && raw <= 100){
    return 0;
  }
 }


 
/**
 *  METHODS TO HANDLE SENOSR READINGS
 */

/**
 * Read u v light sensor measurements, use an average sampling techniqiue 
 * @return : integer representing the standardized u.v light readings
 */
int readUVLight(){
  int sum = 0;
  for(int i=0;  i < 10; i++){
    int reading = uv.readUV();
    Serial.print("uv : ");
  Serial.println(reading);
    int readingStandardized = convertUVReading(reading);
    sum += readingStandardized;
    delay(100);
  }
  int avg = sum / 10;
  categorizeUVReading(avg);
  
  return avg;
}

/**
 * Read temperature sensor measurements from the BMP280 sensor
 */
int readTemp(){
  int sum = 0;
  for(int i=0;  i < 10; i++){
    sum += bmp.readTemperature();
    delay(100);
  }
  int avg = sum / 10;
  return avg;
}
/**
 * Read water level sensor measurements
 */
int readWaterLevel(){
  int sum = 0;
  for(int i=0;  i < 10; i++){
    int waterLevel = analogRead(waterSensor);
    sum += waterLevel;
    delay(100);
  }
  int avg = sum / 10;
  Serial.print("water : ");
  Serial.println(avg);
  return avg;
}

/**
 *  METHODS TO HANDLE RESPONSE TO SENSOR READINGS
 */

/*
 * Drive the DC motor to open the window.
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
 * Drive the DC motor to close the window.
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
/**
 * When flood conditions are detected then this function is called 
 * Flashes RGB Led to alert the user to flood conditions.
 */
void flood(){
  Serial.println("Floood !!!!!");
  for(int i =0; i < 20; i++){
    setColor(249, 74, 62);
    delay(100);
    setColor(64, 180, 209);
    delay(100);
  }
}
/**
 * When 60 degrees celcius is exceeded then flash RGB Led to warn user.
 */
void fire(){
  Serial.println("Fire !!!!!");
  for(int i =0; i < 20; i++){
    setColor(236, 62, 249);
    delay(100);
    setColor(242, 14, 41);
    delay(100);
  }
}

/**
 * Flash LED to alert the user
 */
void flashLED(){
  Serial.println("Quick Warning !!!!!");
  for(int i=0; i < 10; i++){
    digitalWrite(ledPin, HIGH);
    delay(50);
    digitalWrite(ledPin, LOW);
    delay(50);
    }
  }
/**
 * Flash LED to alert the user
 */
void tempAlert(){
  Serial.println("Desired temp exceeded !!!!!");
  for(int i=0; i < 30; i++){
    digitalWrite(ledPin, HIGH);
    delay(50);
    digitalWrite(ledPin, LOW);
    delay(50);
    }
}

/**
 * MAIN LOGIC 
 */
void loop() {
  
  // >>> LISTEN FOR ALERTS <<<<
//  listenForAlerts();
  
  // >>> SENSING <<<<
  int lightLevel = readUVLight();
  int tempReading = readTemp();
  int val = readWaterLevel(); // read water val 
  int valCat = categorizeWaterLevel(val);
    Serial.println(lightLevel);
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
//      broadcastAlerts(1);
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

/**
 * Given 3 integer values representing the intended the color intensity, write the corresponding 
 * values to the RGB Led.
 */
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
