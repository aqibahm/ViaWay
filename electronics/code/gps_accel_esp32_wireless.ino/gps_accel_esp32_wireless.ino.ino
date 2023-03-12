#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include<esp_now.h>
#include<WiFi.h>
#include<Wire.h>

/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp-now-many-to-one-esp32/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

// ===================================================================
// ========== BROADCASTING ===========================================
// ===================================================================

// 1. Broadcast MAC address of server board:
// 30:C6:F7:22:91:44
uint8_t broadcastAddress[] = {0x30, 0xC6, 0xF7, 0x22, 0x91, 0x44};

// 2. Structure to represent collected acceleration and coordinate data:
typedef struct struct_message {
    int id; // must be unique for each sender board
    int x_accel;
    int y_accel;
    int z_accel;
    double gps_lat;
    double gps_long;
} struct_message;

struct_message myData;
// 3. A structure object for representing this sketch's collected data:
// struct struct_message *myData = (struct_message*) malloc( sizeof(struct struct_message));



// 4. Peer interface:
esp_now_peer_info_t peerInfo;

// 5. Setup broadcasting serial stream:
void setup_serial_broadcasting() {
    // Init Serial Monitor -- Broadcasting
    Serial.begin(9600);

    // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
      Serial.println("Error initializing ESP-NOW");
      return;
    }

    // Once ESPNow is successfully Init, we will register for Send CB to
    // get the status of Trasnmitted packet
    esp_now_register_send_cb(OnDataSent);
    
    // Register peer
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    
    // Add peer        
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
      Serial.println("Failed to add peer");
      return;
    }
}

// 5. Callback when data is sent:
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// 6. Sending data to server:
// void send_data_2_server() {
//   // Send message via ESP-NOW
//   esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
   
//   if (result == ESP_OK) {
//     Serial.println("Sent with success");
//     Serial.print(myData.x_accel);
//     Serial.println();
//     Serial.print(myData.y_accel);
//     Serial.println();
//     Serial.print(myData.z_accel);
//     Serial.println();
//     // Serial.print(myData.gps_lat);
//     // Serial.println();
//     // Serial.print(myData -> gps_long);
//     // Serial.println();
//   }
//   else {
//     Serial.println("Error sending the data");
//   }
// }

// ===================================================================
// ========== GPS ====================================================
// ===================================================================

// 1. Wiring:
static const int RXPin = 16, TXPin = 17;
static const uint32_t GPSBaud = 9600;

// 2. Software Serial connection:
SoftwareSerial ss(RXPin, TXPin);

// 3. The TinyGPS++ object:
TinyGPSPlus gps;

// 4. Setup Serial connection for GPS:
void setup_gps_serial() {
    Serial.begin(9600);
    ss.begin(9600);
}

// 5. Reading GPS coordinates data:
void read_gps() {
    // This sketch displays information every time a new sentence is correctly encoded.
    while (ss.available() > 0){
        gps.encode(ss.read());
        if (gps.location.isUpdated()){
            Serial.print("Latitude= "); 
            Serial.print(gps.location.lat(), 6);
            Serial.print(" Longitude= "); 
            Serial.println(gps.location.lng(), 6);

            myData.gps_lat = gps.location.lat();
            myData.gps_long = gps.location.lng();
            // send_data_2_server();
        }
    }
}

// ===================================================================
// ========== ACCELEROMETER ==========================================
// ===================================================================

// 1. I2C Address for MPU6050:
const int MPU = 0x68;  // Can be 0x69 if AD0 Pin is True (VCC)

// 2. Variables for raw values from MPU ADC:
int AcX, AcY, AcZ, GyX, GyY, GyZ;
int16_t Tmp;
int minVal = 0;
int maxVal = 65536; //2^16 levels from MPU6050 ADC 
int offsetTemp = 35; // from MPU datasheet
int offsetTempLevels = 521; // from MPU datasheet

// 3. Setup serial stream for accel data:
void setup_accel_serial() {
    // Accelerometer data gathering:
    Serial.begin(9600); // Begin Serial communication

    Wire.begin();
    Wire.beginTransmission(MPU); // begin transmission with MPU address
    Wire.write(0x6B); 
   
    // Start MPU-6050 sensor 
    Wire.write(0); 
    Wire.endTransmission(true);    
}

// 3. Reading accelerometer data:
void read_accel() {
    Wire.beginTransmission(MPU);
    Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
    Wire.endTransmission(false);
    //Get data from Sensor (14 consecutive bytes)
    Wire.requestFrom(MPU,14,true);  
    
    //Read data byte by byte (16 bits is 8 bits | 8 bits)
    //Each value is composed by 16 bits (2 bytes)

    GyX = Wire.read() << 8 | Wire.read();  //0x3B (GYRO_XOUT_H) & 0x3C (GYRO_XOUT_L)     
    GyY = Wire.read() << 8 | Wire.read();  //0x3D (GYRO_YOUT_H) & 0x3E (GYRO_YOUT_L)
    GyZ = Wire.read() << 8 | Wire.read();  //0x3F (GYRO_ZOUT_H) & 0x40 (GYRO_ZOUT_L)
    Tmp = Wire.read() << 8 | Wire.read();  //0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
    AcX = Wire.read() << 8 | Wire.read();  //0x43 (ACCEL_XOUT_H) & 0x44 (ACCEL_XOUT_L)
    AcY = Wire.read() << 8 | Wire.read();  //0x45 (ACCEL_YOUT_H) & 0x46 (ACCEL_YOUT_L)
    AcZ = Wire.read() << 8 | Wire.read();  //0x47 (ACCEL_ZOUT_H) & 0x48 (ACCEL_ZOUT_L)

    //Adapts to range from 0 to 2000 m/s^2
    AcX = map(AcX, minVal, maxVal, 2000, 0);
    AcY = map(AcY, minVal, maxVal, 2000, 0);
    AcZ = map(AcZ, minVal, maxVal, 2000, 0);
    
    //Adapts to range from -180º to 180º
    int xAng = map(GyX, minVal, maxVal, 180, -180);
    int yAng = map(GyY, minVal, maxVal, 180, -180);
    int zAng = map(GyZ, minVal, maxVal, 180, -180);

    //Send X axis accelerometer value for serial monitor
    Serial.print("AcX = "); Serial.print(AcX);
    
    //Send Y axis accelerometer value for serial monitor
    Serial.print(" | AcY = "); Serial.print(AcY);

    //Send Z axis accelerometer value for serial monitor
    Serial.print(" | AcZ = "); Serial.print(AcZ);

    //Send Temperature value for serial
    //Calculates the temperature given the datasheet values (ºC)
    // 340 is the number of levels per ºC (from datasheet)
    Serial.print(" | Tmp = "); Serial.print(float(Tmp + offsetTempLevels)/340 + offsetTemp);
    
    //Send X axis gyroscope angle value for serial monitor
    Serial.print(" | GyX = "); Serial.print(xAng);

    //Send Y axis gyroscope angle value for serial monitor  
    Serial.print(" | GyY = "); Serial.print(yAng);
    
    //Send Z axis gyroscope angle value for serial monitor
    Serial.print(" | GyZ = "); Serial.println(zAng);
    
    //Wait 300 ms, after that, start new measurement and display.
    myData.id = 3;
    myData.x_accel = AcX;
    myData.y_accel = AcY;
    myData.z_accel = AcZ;

    read_gps(); 

    // Send message via ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
   
    if (result == ESP_OK) {
      Serial.println("Sent with success");
      Serial.print(myData.x_accel);
      Serial.println();
      Serial.print(myData.y_accel);
      Serial.println();
      Serial.print(myData.z_accel);
      Serial.println();
      Serial.print(myData.gps_lat);
      Serial.println();
      Serial.print(myData.gps_long);
      Serial.println();
  }
  else {
    Serial.println("Error sending the data");
  }  
}

// ===================================================================
// ========== ARDUINO SETUP AND LOOP FUNCTIONS =======================
// ===================================================================

void setup(){
    // myData -> id = 1;
    // myData -> x_accel = -1;
    // myData -> y_accel = -1;
    // myData -> z_accel = -1;
    // myData -> gps_lat = -9999;
    // myData -> gps_long = -9999;
    setup_gps_serial();
    setup_serial_broadcasting();
    setup_accel_serial();
}

void loop(){
    // read_accel();
    // read_gps();
    read_accel();
    // send_data_2_server();
}
