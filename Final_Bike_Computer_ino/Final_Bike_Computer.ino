/* Johanna Lynn
Bike Computer
Version 1.5
7/19/14

This sketch has 4 main methods:
  showDateTime()
  showSpeed()
  showTemp()
  showMain()
*/

// these are necessary to use the RTC
#include <Wire.h>
#include "RTClib.h"

// required to use the LCD
#include <SoftwareSerial.h>

// Create a software serial port!
SoftwareSerial lcd = SoftwareSerial(0,2); 

// this declaration allows communication with the RTC chip
RTC_DS1307 RTC;

// this constant shows where we'll find the temp sensor
const int tempPin = A0;   // must be an analog pin !!
const int btnLed = 13;
const int magLed = 8;  // for testing
const int hallPin = 7;

//variables for RPM and Speed
int prevMag = HIGH;
long prevRevTime;
float mph;
int rpm;
float maxMPH;
const int revTimeout = 3000;
const int magRadius = 700;
const float wheelCircum = magRadius* 2 * PI / 25.4 / 12 / 5280;

// these variables are used to calculate the temperature
int tempValue;    // raw analog data
int tempMV;       // corresponding voltage
float tempCent;   // degrees Fahrenheit
float tempFahr;   // degrees Centigrade
float highTemp = -1023;
float lowTemp = 1023;

const int button = 12;
int mode = 3;

// these are used to print text abbreviations of months and weeks
const String dayName[7]  = {
  "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const String monName[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun", 
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

void setup() {
  //LED setup
  pinMode(btnLed, OUTPUT);
  pinMode(magLed, OUTPUT);
//  for(int i = 2; i < sizeof(ledPins); i++){
//    pinMode(ledPins[i], OUTPUT);
//  }
  
  //button setup
  pinMode(button, INPUT);
  
  //magnet setup
  pinMode(hallPin, INPUT);
  
  // setup the pin for the temp sensor input
  pinMode(tempPin, INPUT);  
  
  // setup the serial port for sending data to the serial monitor or host computer
  Serial.begin(9600);
 
  //for clock
  Wire.begin();
  RTC.begin();

  // following line sets the RTC to the date & time this sketch was compiled
  RTC.adjust(DateTime(__DATE__, __TIME__));

  // setup the LCD
  lcd.begin(9600); 
  // set the size of the display 
  lcd.write(0xFE);
  lcd.write(0xD1);
  lcd.write(16);  // 16 columns
  lcd.write(2);   // 2 rows
  delay(10);
  //set the contrast, 200 is a good place to start
  lcd.write(0xFE);
  lcd.write(0x50);
  lcd.write(200);
  delay(10);
  // set the brightness - we'll max it (255 is max brightness)
  lcd.write(0xFE);
  lcd.write(0x99);
  lcd.write(255);
  delay(10);

  // display a startup message
  Serial.println ("****Start****");
}

uint8_t red, green, blue;

void loop() {
  if( digitalRead(button) == HIGH){
      mode++;
      if(mode > 3){
        mode = mode % 4;
      }  
      digitalWrite(btnLed, HIGH);
  }else{
      digitalWrite(btnLed, LOW);
  }
  
  // read the raw value of the temp sensor
  tempValue = analogRead(tempPin); 

  //temperature
  calcTemp();
  
  detectMagnet();
  
  if(mode == 0){
    showTemp(tempCent, tempFahr);
  } else if( mode == 1){
    showSpeed();  
  } else if(mode ==2){
    showDateTime();
  } else {
    showMain();
  }
}

//COMPUTATION METHODS
void detectMagnet(){

  // check the sensor
  int magValue = digitalRead(hallPin);

  Serial.print("Magnet sensor");
  if(magValue == LOW){
    digitalWrite(magLed, HIGH);
  }else{
    digitalWrite(magLed, LOW);
  }
  Serial.println(digitalRead(hallPin));

  // see if the magnet is triggering this time through the loop - 
  // that happens if there is a change from the previous time through the loop (in/out) AND it is coming in 
  if ((magValue != prevMag) && (magValue == LOW)) {

    // see what time it is now
    long curRevTime = millis();

    // figure out the elapsed time 
    long thisRevTime = curRevTime - prevRevTime;	// this is in ms (1000th of a sec)

    // convert to RPM 
    // 1000 ms/sec * 60 sec/min = 60000 ms/min
    rpm =  60000 / thisRevTime;
    
    // and now to MPH, which depends on the circumference of the wheel, which is in miles
    // so, revs/minute * 60 min/hr * wheel circumference (miles)/rev = miles/hour
    mph = rpm * 60.0 * wheelCircum;

    if(mph > maxMPH){
      maxMPH = mph;
    }

    // write this to the serial monitor
    Serial.print("trigger @ ");
    Serial.print(curRevTime);
    Serial.print(" = ");
    Serial.print(thisRevTime);
    Serial.print(" ms/rev = ");
    Serial.print(rpm);
    Serial.print(" RPM");
    Serial.print(mph);
    Serial.println(" mph");
    
    // and save the trigger time for the next one
    prevRevTime = curRevTime;
  } else {
    // if no trigger has happened, we still need to check to see if the wheel has stopped,
    // in that case, we need to set the speed to zero; otherwise it will just stay at whatever it was last.
    // this is done by checking to see if the time since the last revolution is more than a fixed
    // timeout value. if so, we declare the wheel stopped.
    if ((millis() - prevRevTime) > revTimeout) {
      Serial.println("at rest");
      mph = 0.0;
      rpm = 0;
      delay(100);
    }
  }
  // and, regardless of whether the magnet has triggered, remember its in/out state for next time
  prevMag = magValue;
}

void calcTemp(){
  // an analog sensor on the Arduino returns a value between 0 and 1023, depending on the strength
  // of the signal it reads. (this is because the Arduino has a 10-bit analog to digital converter.)
  tempMV = map(tempValue, 0, 1023, 0, 4999);
  // now convert this voltage to degrees, both Fahrenheit and Centigrade
  // this calculation comes from the spec of the specific sensor we are using
  tempCent = (tempMV - 500) / 10.0;
  tempFahr = (tempCent * 9 / 5.0) + 32;

  if(highTemp < tempFahr){
    highTemp = tempFahr;
  }
  if(lowTemp > tempFahr){
    lowTemp = tempFahr;
  }
}

//Methods to display on LCD
void showMain(){
  green = 0;
  lcd.write(0xFE);
  lcd.write(0xD0);
  lcd.write(255 - green);
  lcd.write(green); 
  lcd.write((uint8_t)0);
  delay(10);

  refreshLCD();
  // create a custom character
  lcd.write(0xFE);
  lcd.write(0x4E);
  lcd.write((uint8_t)0); // location #0
  lcd.write((uint8_t)0x00); // 8 bytes of character data
  lcd.write(0x0A);
  lcd.write(0x15);
  lcd.write(0x11);
  lcd.write(0x11);
  lcd.write(0x0A);
  lcd.write(0x04);
  lcd.write((uint8_t)0x00);
  delay(10); 
    
  refreshLCD();

  lcd.println("Bike Computer");
  lcd.print("   I ");
  lcd.write((uint8_t)0); // to print the custom character, 'write' the location
  lcd.print(" Arduino!");
  delay(500); 
}

void showTemp(float cent, float fahr){
  red = 0;
  lcd.write(0xFE);
  lcd.write(0xD0);
  lcd.write(red); 
  lcd.write((uint8_t)0);
  lcd.write(255 - red);
  delay(10);

  refreshLCD();
  
  lcd.print(fahr);
  lcd.print((char)223); //print an ASCII character with the value of 223, the degree symbol
  lcd.print("F ");
  lcd.print(cent);
  lcd.print((char)223);
  lcd.println("C");
  
  delay(100); 
}

void showDateTime(){
  blue = 128;
  lcd.write(0xFE);
  lcd.write(0xD0);
  lcd.write((uint8_t)0);
  lcd.write(255 - blue);
  lcd.write(blue); 
  delay(10);
  
  refreshLCD();

  DateTime rightNow = RTC.now();
  int curYear = rightNow.year();
  int curMonth = rightNow.month();
  int curDay = rightNow.day();
  int curHour = rightNow.hour();
  int curMin = rightNow.minute();
  int curSec = rightNow.second();
  int curWeekday = rightNow.dayOfWeek();

  // reformat it into a single string that looks like "mm/dd/yyyy hh:mm:sec"
  char string[20];  // a string with 19 characters in it (it has to be one longer than you need)

  sprintf(string, "%02d/%02d/%4d %02d:%02d:%02d", curMonth, curDay, curYear, curHour, curMin, curSec);

  // and print that also, with the name of the day of the week
  Serial.print(string);
  Serial.print(" ");

  // now add the 3-letter abbreviation of the day of the week to the end, converting the numeric value
  // into a string by a single lookup. (this is equivalent to a big if/else/if/else... statement, just
  // neater.)
  Serial.print(dayName[curWeekday]);
  Serial.println();
 
  lcd.print(string);
  lcd.print(" ");
  lcd.print(dayName[curWeekday]);
  delay(100); 
}

void showSpeed(){
  refreshLCD();

  lcd.print(mph);
  lcd.print(" MPH");
  lcd.println();
  lcd.print(rpm);
  lcd.print(" RPM");
  delay(100);
}

void refreshLCD(){
  // clear screen
  lcd.write(0xFE);
  lcd.write(0x58);
  delay(10);
  // go 'home'
  lcd.write(0xFE);
  lcd.write(0x48);
  delay(10);
  //turn off cursors
  lcd.write(0xFE);
  lcd.write(0x4B);
  lcd.write(0xFE);
  lcd.write(0x54);
}
