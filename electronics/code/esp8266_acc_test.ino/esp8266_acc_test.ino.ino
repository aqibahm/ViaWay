//Include Wire library to use I2C communication protocl
#include<Wire.h>
 
//I2C Address for MPU6050
const int MPU=0x68;  // Can be 0x69 if AD0 Pin is True (VCC)

//Variables for raw values from MPU ADC
int AcX,AcY,AcZ,GyX,GyY,GyZ;
int16_t Tmp;

int minVal=0;
int maxVal=65536; //2^16 levels from MPU6050 ADC 

int offsetTemp = 35; // from MPU datasheet
int offsetTempLevels = 521; // from MPU datasheet

void setup()
{
  Serial.begin(9600); // Begin Serial communication

  Wire.begin();
  Wire.beginTransmission(MPU); // begin transmission with MPU address
  Wire.write(0x6B); 
   
  //Start MPU-6050 sensor 
  Wire.write(0); 
  Wire.endTransmission(true);    

}

void loop()
{
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  //Get data from Sensor (14 consecutive bytes)
  Wire.requestFrom(MPU,14,true);  
  //Read data byte by byte (16 bits is 8 bits | 8 bits)
  //Each value is composed by 16 bits (2 bytes)
  GyX=Wire.read()<<8|Wire.read();  //0x3B (GYRO_XOUT_H) & 0x3C (GYRO_XOUT_L)     
  GyY=Wire.read()<<8|Wire.read();  //0x3D (GYRO_YOUT_H) & 0x3E (GYRO_YOUT_L)
  GyZ=Wire.read()<<8|Wire.read();  //0x3F (GYRO_ZOUT_H) & 0x40 (GYRO_ZOUT_L)
  Tmp=Wire.read()<<8|Wire.read();  //0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
  AcX=Wire.read()<<8|Wire.read();  //0x43 (ACCEL_XOUT_H) & 0x44 (ACCEL_XOUT_L)
  AcY=Wire.read()<<8|Wire.read();  //0x45 (ACCEL_YOUT_H) & 0x46 (ACCEL_YOUT_L)
  AcZ=Wire.read()<<8|Wire.read();  //0x47 (ACCEL_ZOUT_H) & 0x48 (ACCEL_ZOUT_L)

  //Adapts to range from 0 to 2000 m/s^2
  AcX = map(AcX,minVal,maxVal,2000,0);
  AcY = map(AcY,minVal,maxVal,2000,0);
  AcZ = map(AcZ,minVal,maxVal,2000,0);
  
  //Adapts to range from -180º to 180º
  int xAng = map(GyX,minVal,maxVal,180,-180);
  int yAng = map(GyY,minVal,maxVal,180,-180);
  int zAng = map(GyZ,minVal,maxVal,180,-180); 
   
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
  // delay(10000);
}