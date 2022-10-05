#include <EEPROM.h>
int incomingByte = 1; // for incoming Serial2 data
int pseudoEEPROM[9];
int newEEPROM[9];
int i=0;
boolean done = false;

HardwareSerial Serial2(PA3,PA2);
void setup() {
  Serial2.begin(9600); // opens Serial2 port, sets data rate to 9600 bps
  i=0;
}

void loop() {
  // send data only when you receive data:
}
