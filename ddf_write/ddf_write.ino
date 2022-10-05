#include <EEPROM.h>
#define cells 9
#define rows 3
#define columns 3

float pseudoEEPROM[cells], val[rows][columns];
int i = 0;

struct table_cell {
  float throttle;
  float rpm;
  float injection;
} table_cell;

union table_in_ram {
  byte raw[sizeof(struct table_cell) * cells];
  struct table_cell parsed[cells];
} table;

void setup() {
  // put your setup code here, to run once:
  Serial2.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);

  //baca serial
  while (Serial2.available() > 0) {
    for (i = 0; i < sizeof(table_cell); i++) {
      table.raw[i] = Serial2.parseFloat();
    }
  }

  //update EEPROM
  for (i = 0; i < sizeof(table_cell); i++) {
    EEPROM.update(j, table.raw[j]);
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  for (i = 0; i < cells; i++) {
    Serial2.print("throttle = ");
    Serial2.println(table.parsed[i].throttle);

    Serial2.print("RPM = ");
    Serial2.println(table.parsed[i].rpm);

    Serial2.print("Injection = ")
    Serial2.println(table.parsed[i].injection)
  }
}
