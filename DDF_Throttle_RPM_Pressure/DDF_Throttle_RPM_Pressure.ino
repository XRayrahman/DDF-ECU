#include <EEPROM.h>
#include <stdlib.h>
#include <math.h>

#define pin PB5
#define cells 50
#define rows 1
#define columns 3
#define values 3

float raw, injection, throttle_read, rpm_read, throttle, pressure_read, pressure;
float pseudoEEPROM[cells], val[rows][columns];
float f = 0.00f, adcVal = 0;
byte b;
int i, j, flag;
int k = 0;
int totalValues = values * cells;

volatile int rpmcount = 0;
volatile float rpm = 0;  //contador de rpm
volatile unsigned long timeold = 0;
volatile unsigned long tnew;
boolean done = false;
uint32_t duty_fs = 0;
uint32_t out_duty_fs = 0;
uint32_t duty_nx = 10;

struct table_cell {
  uint16_t throttle;
  uint16_t rpm;
  uint16_t injection;
} table_cell;

union table_in_ram {
  byte raw[cells * sizeof(struct table_cell)];
  // float raw[values * cells];
  struct table_cell parsed[cells];
} table;

HardwareSerial Serial2(PA3, PA2);
HardwareTimer pwmtimer(TIM1);

void interruptPin();

void setup() {
  Serial2.begin(115200);
  pinMode(PC1, INPUT_ANALOG);
  pinMode(PA4, INPUT_ANALOG);
  analogReadResolution(10);
  pinMode(PE_8, OUTPUT);
  pinMode(PE_10, OUTPUT);
  pinMode(PE_12, OUTPUT);
  pinMode(PE_14, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(PE9), interruptPin, FALLING);

  rpm = 0;
  flag = 0;
  bacaEEPROM();
}

void loop() {

  if (Serial2.available() > 0) {
    if (flag == 0) {
      tulisEEPROM();
      flag = 1;
    }
  }
  // Serial2.println(millis());
  if (millis() - timeold >= 1000) { /*Update every one second, this will be equal to reading frecuency (Hz).*/
    // Serial2.println("line 70");
    detachInterrupt(0);  //Disable interrupt when calculating

    pressure_read = analogRead(PA4);
    pressure = calculatePressure(pressure_read);

    rpm_read = rpmcount * 60;
    Serial2.println(rpm_read);
    // rpm_read = 500;
    throttle_read = analogRead(PC1);
    Serial2.println(throttle_read);
    // throttle_read = 550;
    // throttle_read = random(0, 1000);

    // for (i = 0; i < totalValues; i++) {
    //   if (table.parsed[i].rpm == rpm && table.parsed[i].throttle == adcVal) {
    //     duty_nx = table.parsed[i].injection;
    //   } else {term
    //     continue;
    //   }
    // }

    duty_nx = dedaf_solve_injection_delay(&table_cell, cells, throttle_read, rpm_read);

    if (duty_nx != duty_fs && pressure > 1.2) {
      duty_fs = duty_nx;
      // out_duty_fs = (duty_fs * 50 / 1000) * 2047;
      pwm_start(PE_8, 50, duty_fs, RESOLUTION_11B_COMPARE_FORMAT);
      pwm_start(PE_10, 50, duty_fs, RESOLUTION_11B_COMPARE_FORMAT);
      pwm_start(PE_12, 50, duty_fs, RESOLUTION_11B_COMPARE_FORMAT);
      pwm_start(PE_14, 50, duty_fs, RESOLUTION_11B_COMPARE_FORMAT);  // used for Dutycycle: [0 .. 2047]
      // pwm_start(PE_12, 50, 200, RESOLUTION_11B_COMPARE_FORMAT);
    } else {
      duty_fs = 0;

      pwm_start(PE_8, 50, duty_fs, RESOLUTION_11B_COMPARE_FORMAT);
      pwm_start(PE_10, 50, duty_fs, RESOLUTION_11B_COMPARE_FORMAT);
      pwm_start(PE_12, 50, duty_fs, RESOLUTION_11B_COMPARE_FORMAT);
      pwm_start(PE_14, 50, duty_fs, RESOLUTION_11B_COMPARE_FORMAT);
    }
    Serial2.print("duty_fs = ");
    Serial2.println(duty_fs);


    rpmcount = 0;        // Restart the RPM counter
    timeold = millis();  // Update lasmillis
    attachInterrupt(digitalPinToInterrupt(PE9), interruptPin, FALLING);
  }
  // Serial2.println("line 97");
}

void interruptPin() {
  rpmcount++;
}

float calculatePressure(float pressureAnalog){
  float pressureVolt = pressureAnalog / 1023;
  float pressureRead = ((pressureVolt - 0.33) / 3) * 5;
  return pressureRead;
}

void bacaEEPROM() {
  k = 0;
  Serial2.println("reading EEPROM...");
  // for (i = 0; i < totalValues * sizeof(float); i += sizeof(float)) {
  //   EEPROM.get(i, f);
  //   table.raw[k] = f;
  //   k++;
  //   if (k == totalValues) {
  //     k = 0;
  //   }
  // }

  for (i = 0; i < cells * sizeof(struct table_cell); i++) {
    table.raw[i] = EEPROM.read(i);
    // Serial2.println(table.raw[i], HEX);
  }


  for (j = 0; j < cells; j++) {

    // Serial2.println(table.parsed[j].throttle);
    throttle = table.parsed[j].throttle;
    rpm = table.parsed[j].rpm;
    injection = table.parsed[j].injection;

    Serial2.print("Throttle = ");
    Serial2.print(throttle);

    Serial2.print(" RPM = ");
    Serial2.print(rpm);

    Serial2.print(" Injection = ");
    Serial2.println(injection);
    // delay();
  }
}

void hapusEEPROM() {
  for (i = 0; i < cells * sizeof(struct table_cell); i++) {
    EEPROM.update(i, 0xFF);
  }
}

void tulisEEPROM() {
  Serial2.println("writingEEPROM...");
  // for (i = 0; i < totalValues; i++) {
  //   table.raw[i] = Serial2.parseFloat();
  // }

  // for (i = 0; i < totalValues * sizeof(float); i += sizeof(float)) {
  //   EEPROM.put(i, table.raw[k]);
  //   k++;
  //   if (k == totalValues) {
  //     k = 0;
  //   }
  // }

  for (i = 0; i < cells * sizeof(struct table_cell); i++) {
    while (!Serial2.available())
      ;
    b = Serial2.read();
    EEPROM.update(i, b);
    // Serial2.println(b, HEX);
  }

  Serial2.println("EEPROM written!");
  return;
}

unsigned long dedaf_solve_injection_delay(
  struct table_cell *table_cells,
  unsigned int cell_count,
  float throttle,
  float rpm) {
  // Don't do anything if table is empty
  if (!cell_count)
    return 0;

  // We want closest cell to current coordinate, as possible.
  // Let's find out the constraints
  float min_throttle_constraint = -INFINITY;
  float min_rpm_constraint = -INFINITY;
  float max_throttle_constraint = INFINITY;
  float max_rpm_constraint = INFINITY;

  // ...so we have to solve 4 closest cell based on
  // throttle and RPM value
  for (int i = 0; i < cell_count; i++) {
    struct table_cell *current_cell = &(table_cells[i]);

    if (current_cell->throttle <= throttle && current_cell->throttle >= min_throttle_constraint)
      min_throttle_constraint = current_cell->throttle;
    if (current_cell->throttle >= throttle && current_cell->throttle <= max_throttle_constraint)
      max_throttle_constraint = current_cell->throttle;
    if (current_cell->rpm <= rpm && current_cell->rpm >= min_rpm_constraint)
      min_rpm_constraint = current_cell->rpm;
    if (current_cell->rpm >= rpm && current_cell->rpm <= max_rpm_constraint)
      max_rpm_constraint = current_cell->rpm;
  }

  /*
	Mathematically, we map those variable names to be:
		min_throttle_constraint -> x1
		     min_rpm_constraint -> y1
		max_throttle_constraint -> x2
		     max_rpm_constraint -> y2
	*/

  // Throttle is horizontal, RPM is vertical
  struct table_cell *top_left = NULL;
  struct table_cell *top_right = NULL;
  struct table_cell *bottom_left = NULL;
  struct table_cell *bottom_right = NULL;

  for (int i = 0; i < cell_count; i++) {
    struct table_cell *current_cell = &(table_cells[i]);

    if (!top_left
        || current_cell->throttle <= throttle && current_cell->throttle >= min_throttle_constraint
             && current_cell->rpm <= rpm && current_cell->rpm >= min_rpm_constraint) {
      top_left = current_cell;
    }

    if (!top_right
        || current_cell->throttle >= throttle && current_cell->throttle <= max_throttle_constraint
             && current_cell->rpm <= rpm && current_cell->rpm >= min_rpm_constraint) {
      top_right = current_cell;
    }

    if (!bottom_left
        || current_cell->throttle <= throttle && current_cell->throttle >= min_throttle_constraint
             && current_cell->rpm >= rpm && current_cell->rpm <= max_rpm_constraint) {
      bottom_left = current_cell;
    }

    if (!bottom_right
        || current_cell->throttle >= throttle && current_cell->throttle <= max_throttle_constraint
             && current_cell->rpm >= rpm && current_cell->rpm <= max_rpm_constraint) {
      bottom_right = current_cell;
    }
  }

  /*
	Those cells are now transpose of a matrix. Thus, we got:
	     __                         __     __       __
	    |   top_left     bottom_left  |   | Q11   Q12 |
	Q = |                             | = |           |
	    |   top_right    bottom_right |   | Q21   Q22 |
	     ‾‾                         ‾‾     ‾‾       ‾‾
	See Bilinear Interpolation:
		https://en.wikipedia.org/wiki/Bilinear_interpolation#Repeated_linear_interpolation
	*/

  return (
    (1 / ((max_throttle_constraint - min_throttle_constraint) * (max_rpm_constraint - min_rpm_constraint))) * ((top_left->injection * (max_throttle_constraint - throttle) * (max_rpm_constraint - rpm)) + (top_right->injection * (throttle - min_throttle_constraint) * (max_rpm_constraint - rpm)) + (bottom_left->injection * (max_throttle_constraint - throttle) * (rpm - min_rpm_constraint)) + (bottom_right->injection * (throttle - min_throttle_constraint) * (rpm - min_rpm_constraint))));
}