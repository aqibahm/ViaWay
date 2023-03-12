#include "BluetoothSerial.h"
BluetoothSerial SerialBT;
 
void setup() {
  SerialBT.begin("Aqib_ESP32");
}
 
void loop() {
  SerialBT.println("Hello Aqib!");
  delay(1000);
}