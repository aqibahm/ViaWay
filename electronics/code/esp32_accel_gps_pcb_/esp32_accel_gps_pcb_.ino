#include <MPU6050.h>

#include <TinyGPS++.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <WiFi.h>

// The TinyGPS++ object
TinyGPSPlus gps;

#define RXD2 3
#define TXD2 1

//I2C Address for MPU6050
const int MPU=0x68;  // Can be 0x69 if AD0 Pin is True (VCC)

//Variables for raw values from MPU ADC
int AcX,AcY,AcZ,GyX,GyY,GyZ;
int16_t Tmp;

int minVal=0;
int maxVal=65536; //2^16 levels from MPU6050 ADC 

int offsetTemp = 35; // from MPU datasheet
int offsetTempLevels = 521; // from MPU datasheet

float latitude = 1.2345; 
float longitude = 6.7890;



// Code for the ESP8266
//#include "ESP8266WiFi.h"  // Enables the ESP8266 to connect to the local network (via WiFi)
//#define DHTPIN D5         // Pin connected to the DHT sensor

// #define DHTTYPE DHT22  // DHT11 or DHT22
// DHT dht(DHTPIN, DHTTYPE);

// WiFi
const char* ssid = "University of Washington";                 // Your personal network SSID
const char* wifi_password = ""; // Your personal network password

// MQTT
const char* mqtt_server = "10.18.239.147";  // IP of the MQTT broker
const char* accel_x_topic = "module/wireless/acceleration_x";
const char* accel_y_topic = "module/wireless/acceleration_y";
const char* accel_z_topic = "module/wireless/acceleration_z";
const char* gps_latitude_topic = "module/wireless/gps_lat";
const char* gps_longitude_topic = "module/wireless/gps_long";
const char* mqtt_username = "aqib"; // MQTT username
const char* mqtt_password = "aqib@123"; // MQTT password
const char* clientID = "ViaWay Wireless Module 1"; // MQTT client ID

// Initialise the WiFi and MQTT Client objects
WiFiClient wifiClient;
// 1883 is the listener port for the Broker
PubSubClient client(mqtt_server, 1883, wifiClient); 

// Custom function to connet to the MQTT broker via WiFi
void connect_MQTT(){
  Serial.print("Connecting to ");
  Serial.println(ssid);

        // Connect to the WiFi
        WiFi.begin(ssid, wifi_password);

        // Wait until the connection has been confirmed before continuing
        while (WiFi.status() != WL_CONNECTED) {
            // delay(500);
            // Serial.print(".");
        }

        // Debugging - Output the IP Address of the ESP8266
        Serial.println("WiFi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());

        // Connect to MQTT Broker
        // client.connect returns a boolean value to let us know if the connection was successful.
        // If the connection is failing, make sure you are using the correct MQTT Username and Password (Setup Earlier in the Instructable)
        if (client.connect(clientID, mqtt_username, mqtt_password)) {
            Serial.println("Connected to MQTT Broker!");
        }
        else {
            Serial.println("Connection to MQTT Broker failed...");
        }
}


void setup(){
    Serial2.begin(9600, SERIAL_8N1,RXD2,TXD2);
    Serial.begin(115200);

    Wire.setClock(1000000);
    Wire.begin();
    Wire.beginTransmission(MPU); // begin transmission with MPU address
    Wire.write(0x6B); 

    //Start MPU-6050 sensor 
    Wire.write(0); 
    Wire.endTransmission(true);    
    // delay(10000);

    Serial.println();
}

void loop()
{
  // This sketch displays information every time a new sentence is correctly encoded.
  connect_MQTT();
    delay(20);
    
    for (int i = 0; i < 1000; i ++) {
      // delay(10);
    //Read data byte by byte (16 bits is 8 bits | 8 bits)
    //Each value is composed by 16 bits (2 bytes)
    Wire.beginTransmission(MPU);
    Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
    Wire.endTransmission(false);
    //Get data from Sensor (14 consecutive bytes)
    Wire.requestFrom(MPU, 14, true);

    GyX = Wire.read() << 8 | Wire.read();  //0x3B (GYRO_XOUT_H) & 0x3C (GYRO_XOUT_L)     
    GyY = Wire.read() << 8 | Wire.read();  //0x3D (GYRO_YOUT_H) & 0x3E (GYRO_YOUT_L)
    GyZ = Wire.read() << 8 | Wire.read();  //0x3F (GYRO_ZOUT_H) & 0x40 (GYRO_ZOUT_L)
    Tmp = Wire.read() << 8 | Wire.read();  //0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
    AcX = Wire.read() << 8 | Wire.read();  //0x43 (ACCEL_XOUT_H) & 0x44 (ACCEL_XOUT_L)
    AcY = Wire.read() << 8 | Wire.read();  //0x45 (ACCEL_YOUT_H) & 0x46 (ACCEL_YOUT_L)
    AcZ = Wire.read() << 8 | Wire.read();  //0x47 (ACCEL_ZOUT_H) & 0x48 (ACCEL_ZOUT_L)

    //Adapts to range from 0 to 2000 m/s^2
    // AcX = map(AcX, minVal, maxVal, 2000, 0);
    // AcY = map(AcY, minVal, maxVal, 2000, 0);
    AcZ = map(AcZ, minVal, maxVal, 2000, 0);
  // PUBLISH to the MQTT Broker (topic = Temperature, defined at the beginning)
  
    if (client.publish(accel_z_topic, String(AcZ).c_str())) {
    // client.publish(accel_x_topic, String(AcX).c_str())) && 
        // client.publish(accel_y_topic, String(AcY).c_str()) &&
      Serial.println("Z Acceleration data sent!");
  }

  // Again, client.publish will return a boolean value depending on whether it succeded or not.
  // If the message failed to send, we will try again, as the connection may have broken.
  else {
    Serial.println("Acceleration data failed to send. Reconnecting to MQTT Broker and trying again");
    client.connect(clientID, mqtt_username, mqtt_password);
    delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
    // client.publish(accel_x_topic, String(AcX).c_str());
    // client.publish(accel_y_topic, String(AcY).c_str());
    client.publish(accel_z_topic, String(AcZ).c_str()); 
    }  
  }
    
    //Adapts to range from -180º to 180º
    // int xAng = map(GyX, minVal, maxVal, 180, -180);
    // int yAng = map(GyY, minVal, maxVal, 180, -180);
    // int zAng = map(GyZ, minVal, maxVal, 180, -180); 
    
    // //Send X axis accelerometer value for serial monitor
    // Serial.print("AcX = "); Serial.print(AcX);
    
    // //Send Y axis accelerometer value for serial monitor
    // Serial.print(" | AcY = "); Serial.print(AcY);

    // //Send Z axis accelerometer value for serial monitor
    // Serial.print(" | AcZ = "); Serial.print(AcZ);

    // //Send Temperature value for serial
    // //Calculates the temperature given the datasheet values (ºC)
    // // 340 is the number of levels per ºC (from datasheet)
    // Serial.print(" | Tmp = "); Serial.print(float(Tmp + offsetTempLevels) / 340 + offsetTemp);
    
    // //Send X axis gyroscope angle value for serial monitor
    // Serial.print(" | GyX = "); Serial.print(xAng);
    
    // //Send Y axis gyroscope angle value for serial monitor  
    // Serial.print(" | GyY = "); Serial.print(yAng);

    // //Send Z axis gyroscope angle value for serial monitor
    // Serial.print(" | GyZ = "); Serial.println(zAng);

  if (Serial2.available() > 0) {
    if (gps.encode(Serial2.read())) {
      latitude = gps.location.lat();
      longitude = gps.location.lng();
    }
  }

    if ((client.publish(gps_latitude_topic, String(latitude).c_str())) && (client.publish(gps_longitude_topic, String(longitude).c_str()))) {
    Serial.println("GPS data sent!");
  } else { 
    Serial.println("GPS data failed to send. Reconnecting to MQTT Broker and trying again");
    client.connect(clientID, mqtt_username, mqtt_password);
    delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
    client.publish(gps_latitude_topic, String(latitude).c_str());
    client.publish(gps_longitude_topic, String(longitude).c_str());    
  }
//   if (millis() > 5000 && gps.charsProcessed() < 10)
//   {
//     Serial.println(F("No GPS detected: check wiring."));
//     while(true);
//   }

    client.disconnect();
} 

// void displayInfo()
// {
//   // Serial.print(F("Location: ")); 
//   if (gps.location.isValid())
//   {
//     Serial.print(gps.location.lat(), 6);
//     Serial.print(F(","));
//     Serial.print(gps.location.lng(), 6);
//   }
//   else
//   {
//     Serial.print(F("INVALID"));
//   }

//   Serial.print(F("  Date/Time: "));
//   if (gps.date.isValid())
//   {
//     Serial.print(gps.date.month());
//     Serial.print(F("/"));
//     Serial.print(gps.date.day());
//     Serial.print(F("/"));
//     Serial.print(gps.date.year());
//   }
//   else
//   {
//     Serial.print(F("INVALID"));
//   }

//   Serial.print(F(" "));
//   if (gps.time.isValid())
//   {
//     if (gps.time.hour() < 10) Serial.print(F("0"));
//     Serial.print(gps.time.hour());
//     Serial.print(F(":"));
//     if (gps.time.minute() < 10) Serial.print(F("0"));
//     Serial.print(gps.time.minute());
//     Serial.print(F(":"));
//     if (gps.time.second() < 10) Serial.print(F("0"));
//     Serial.print(gps.time.second());
//     Serial.print(F("."));
//     if (gps.time.centisecond() < 10) Serial.print(F("0"));
//     Serial.print(gps.time.centisecond());
//   }
//   else
//   {
//     Serial.print(F("INVALID"));
//   }

//   Serial.println();
// }