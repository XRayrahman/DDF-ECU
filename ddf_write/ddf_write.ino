#include <EEPROM.h>
#define cells 9
#define rows 3
#define columns 3

float raw, rpm, injection, throttle;
float pseudoEEPROM[cells], val[rows][columns];
float f = 0.00f;
int i, j, flag;
int k = 0;
char input = 'i', output = 'o';

struct table_cell {
  float throttle;
  float rpm;
  float injection;
} table_cell;

union table_in_ram {
  // byte raw[sizeof(struct table_cell)];
  float raw[cells];
  struct table_cell parsed[cells];
} table;

HardwareSerial Serial2(PA3, PA2);
void setup() {
  // put your setup code here, to run once:
  Serial2.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  flag = 0;
}


void loop() {
  // put your main code here, to run repeatedly:
  // if (Serial2.available() > 0) {
  //   if (flag == 0) {
  //     tulisEEPROM();
  //     flag = 1;
  //   }
  // }
  bacaEEPROM();
}

void bacaEEPROM() {
  k = 0;
  Serial2.println("reading EEPROM...");
  for (i = 0; i < cells * sizeof(float); i += sizeof(float)) {
    EEPROM.get(i, f);
    table.raw[k] = f;
    k++;
    if(k == cells){
      k = 0;
    }
  }


  for (j = 0; j < rows; j++) {
    // Serial2.print("Raw = ");
    // Serial2.println(table.raw[j]);
    // delay(1000);

    throttle = table.parsed[j].throttle;
    rpm = table.parsed[j].rpm;
    injection = table.parsed[j].injection;

    Serial2.print("Throttle = ");
    Serial2.print(throttle);

    Serial2.print(" RPM = ");
    Serial2.print(rpm);

    Serial2.print(" Injection = ");
    Serial2.println(injection);
    delay(1000);
  }
}

void tulisEEPROM() {
  Serial2.println("writingEEPROM...");
    for (i = 0; i < cells; i++) {
      table.raw[i] = Serial2.parseFloat();
    }

    for (i = 0; i < cells * sizeof(float); i += sizeof(float)) {
      EEPROM.put(i, table.raw[k]);
      k++;
      if (k == cells) {
        k = 0;
      }
    }
    Serial2.println("EEPROM written!");

    for (i = 0; i < cells * sizeof(float); i += sizeof(float)) {
      EEPROM.get(i, f);
      Serial2.println(f);
    }
    return;
}
