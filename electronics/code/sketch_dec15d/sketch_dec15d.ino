/*
  LiquidCrystal Library - Hello World

 Demonstrates the use a 16x2 LCD display.  The LiquidCrystal
 library works with all LCD displays that are compatible with the
 Hitachi HD44780 driver. There are many of them out there, and you
 can usually tell them by the 16-pin interface.

 This sketch prints "Hello World!" to the LCD
 and shows the time.

  The circuit:
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)

 Library originally added 18 Apr 2008
 by David A. Mellis
 library modified 5 Jul 2009
 by Limor Fried (http://www.ladyada.net)
 example added 9 Jul 2009
 by Tom Igoe
 modified 22 Nov 2010
 by Tom Igoe
 modified 7 Nov 2016
 by Arturo Guadalupi

 This example code is in the public domain.

 http://www.arduino.cc/en/Tutorial/LiquidCrystalHelloWorld

*/

// include the library code:
#include <LiquidCrystal.h>
#include <OneWire.h>
#include <time.h>

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
OneWire ds(10);  // on pin 10 (a 4.7K resistor is necessary)

const int firstButtonPin = 6;  // the number of the pushbutton pin
const int secondButtonPin = 7;
const int thirdButtonPin = 8;
const int buzzer = 9;
const int analogInPin = A0;  // Analog input pin that the potentiometer is attached to

int firstButtonState = 0;
int secondButtonState = 0;
int thirdButtonState = 0;
int bufferState = 0;
int tempReached = 0;
int steeped = 0;
float tempVal = 0.0;

float targetTemp = 20.0;

void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Custom - 70 C");
  targetTemp = 70;
  // Print a message to the LCD.
  Serial.begin(9600);

  pinMode(firstButtonPin, INPUT);
  pinMode(secondButtonPin, INPUT);
  pinMode(thirdButtonPin, INPUT);
  pinMode(buzzer, OUTPUT);
}

float initialSensorValue = 0;
float finalSensorValue = 0;

void loop() {
  initialSensorValue = finalSensorValue;
  finalSensorValue = analogRead(analogInPin);
  Serial.println(finalSensorValue);

  // read the analog in value:
  // check sequential change in analogInput

  lcd.setCursor(0, 0);

  if (finalSensorValue != initialSensorValue) {
    tempVal = (60 * (finalSensorValue / 1023)) + 20;
    // Show Custom Temp Screen with new temperature.

    String temp_s = "Custom - " + String(tempVal) + " C";
    targetTemp = tempVal;
    lcd.print(temp_s);
  }
  
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  firstButtonState = digitalRead(firstButtonPin); 
  secondButtonState = digitalRead(secondButtonPin);
  thirdButtonState = digitalRead(thirdButtonPin);

  lcd.setCursor(0, 0);

  if(firstButtonState == HIGH) {
    lcd.print("Green Tea - 70 C");
    targetTemp = 70.0;
  } 
  else if (secondButtonState == HIGH) {
    lcd.print("Black Tea - 85 C");
    targetTemp = 85.0;
  } 
  else if (thirdButtonState == HIGH) {
    lcd.print("Coffee - 95 C");
    targetTemp = 95.0;
  }

  lcd.setCursor(0, 1);

  byte i;
  byte present = 0;
  byte type_s;
  byte data[9];
  byte addr[8];
  float celsius, fahrenheit;
  
  if ( !ds.search(addr)) {
    Serial.println("No more addresses.");
    Serial.println();
    ds.reset_search();
    delay(250);
    return;
  }
  
  Serial.print("ROM =");
  for( i = 0; i < 8; i++) {
    Serial.write(' ');
    Serial.print(addr[i], HEX);
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return;
  }
  Serial.println();
 
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      Serial.println("  Chip = DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      Serial.println("  Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
      Serial.println("  Chip = DS1822");
      type_s = 0;
      break;
    default:
      Serial.println("Device is not a DS18x20 family device.");
      return;
  } 

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  Serial.print("  Data = ");
  Serial.print(present, HEX);
  Serial.print(" ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.print(" CRC=");
  Serial.print(OneWire::crc8(data, 8), HEX);
  Serial.println();

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;
  Serial.print("  Temperature = ");
  Serial.print(celsius);
  Serial.print(" Celsius, ");
  Serial.print(fahrenheit);
  Serial.println(" Fahrenheit");
    // print the number of seconds since reset:

  String tempStat = "Probe: " + String(celsius) + " C";
  lcd.print(tempStat);

  if (tempReached == 0) {
    if (celsius >= targetTemp) {
      tempReached = 1;
      tone(buzzer,261);
      //digitalWrite(buzzer, HIGH);
      delay(500);
      noTone(buzzer); 
      delay(500);
      tone(buzzer,261);
      delay(500);
      noTone(buzzer); 
      delay(500);
      tone(buzzer,261);
      delay(500);
      noTone(buzzer); 
      delay(500);
      tone(buzzer,261);
      delay(500);
      noTone(buzzer); 

      // print countdown on display.
      int i;
      String s = "";
      
      for (i = 30; i >= 0; i--) {
        delay(1000);
        lcd.setCursor(0, 0); 
        lcd.clear();
        s = "Steep for " + String(i) + " s.";
        lcd.print(s);
      }

      lcd.setCursor(0, 0);
      lcd.print("Enjoy the brew!");
      
      digitalWrite(buzzer, HIGH);
      delay(500);
      digitalWrite(buzzer, LOW);
      delay(500);
      digitalWrite(buzzer, HIGH);
      delay(500);
      digitalWrite(buzzer, LOW);
      delay(500);
      digitalWrite(buzzer, HIGH);
      delay(500);
      digitalWrite(buzzer, LOW);
      delay(500);
      digitalWrite(buzzer, HIGH);
      delay(500);
      digitalWrite(buzzer, LOW);

      lcd.setCursor(0, 0);
      s = "Custom - " + String(tempVal) + " C";
      lcd.print(s);
      targetTemp = tempVal;
    }

    else if (celsius < targetTemp) {
      tempReached = 0;
    }
  }
  
}

