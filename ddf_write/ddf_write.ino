#include <EEPROM.h>
int incomingByte = 1;  // for incoming Serial2 data
int pseudoEEPROM[9];
int newEEPROM[9];
int i = 0;
boolean done = false;

HardwareSerial Serial2(PA3, PA2);
void setup() {
  Serial2.begin(9600);  // opens Serial2 port, sets data rate to 9600 bps
  i = 0;
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  // send data only when you receive data:
  while (i < 9) {
    if (Serial2.available() > 0) {
      pseudoEEPROM[i] = Serial2.parseInt();
      //EEPROM.write(i, pseudoEEPROM[i]);
      Serial2.print("serial :");
      Serial2.println(pseudoEEPROM[i]);
      i++;
    }
  }
  if (done == false) {
    for (int r = 0; r < 9; r++) {
      EEPROM.write(r, pseudoEEPROM[r]);
      delay(100);
    }
    for (int x = 0; x < 9; x++) {
      newEEPROM[x] = EEPROM.read(x);
      Serial2.print("EEPROM :");
      Serial2.println(newEEPROM[x]);
      delay(100);
    }
    digitalWrite(LED_BUILTIN, HIGH);
    done = true;
  }
}
