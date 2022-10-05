#include <EEPROM.h>
#define cells 9
#define rows 3
#define columns 3

float pseudoEEPROM[cells], val[rows][columns];
int i = 0, j = 0, k = 0, l = 0;

struct table_cell{
  float throttle;
  float rpm;
  float injection;
} table_cell;

union table_in_ram{
  byte raw[sizeof(struct table_cell) * cells];
  struct table_cell parsed[cells];
} table_in_ram;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  //baca EEPROM
  for (i = 0; i < rows; i++) {
    for (j = 0; j < columns; j++) {
        EEPROM.get(k * sizeof(float), val[i][j]);
        Serial.println(val[i][j]);
        k++;
    }
  }

  // EEPROM.clear();
}

void loop() {
  // put your main code here, to run repeatedly:

  //coba pake pseudo EEPROM (array)
  // if (Serial.available() && i < cells) {
  //   pseudoEEPROM[i] = Serial.parseFloat();
  //   Serial.println(pseudoEEPROM[i]);
  //   i++;
  // }

  //coba update EEPROM beneran
  if (Serial.available() && l < cells) {
   EEPROM.put(l * sizeof(float), Serial.parseFloat());
   l++;
  }
}
