#include <SD.h>
#include <SPI.h>
#include <stdlib.h>
#include <math.h>
#include <HardwareSerial.h>

#define SD_CS_PIN PA4
#define cells 80
#define INJECTOR_1 PB_6
#define INJECTOR_2 PB_7
#define INJECTOR_3 PB_8
#define INJECTOR_4 PB_9

Sd2Card card;
SdVolume volume;
SdFile root;
File file;
HardwareSerial Serial2(USART2); // RX,TX

int i=0;

//--> SD variable
char dataChar;
String dataString = "";
uint16_t val;
int integerFromPC = 0;
float floatFromPC = 0.0;
boolean newData = false;
long randNum;

//--> I/O variable
float injection, throttle_read, rpm_read, throttle, pressure_read, pressure;
volatile int rpmcount = 0;
volatile float rpm = 0;  //contador de rpm
volatile unsigned long timeold = 0;
volatile unsigned long tnew;
boolean done = false;
uint32_t duty_fs = 0;
uint32_t out_duty_fs = 0;
uint32_t duty_nx = 10;

//--> pseudo table
struct table_cell {
  uint16_t throttle;
  uint16_t rpm;
  uint16_t injection;
};

struct table_cell table_cell;

union table_in_SD {
  byte raw[cells*sizeof(struct table_cell)];
  uint16_t int_data[cells*3];
  struct table_cell parsed[cells];
} table;

const byte numChars = cells*sizeof(struct table_cell);
char receivedChars[numChars];
char tempChars[numChars];        // temporary array for use when parsing
char messageFromPC[numChars] = {0}; // variables to hold the parsed data

long long pwm_t;

//================================================================================//

void setup() {
  SPI.begin();
  Serial2.begin(115200);
  pinMode(PA0, INPUT_ANALOG); //PA0
  pinMode(PA1, INPUT_ANALOG);
//  analogReadResolution(10);
  pinMode(INJECTOR_1, OUTPUT); 
  pinMode(INJECTOR_2, OUTPUT);
  pinMode(INJECTOR_3, OUTPUT);
  pinMode(INJECTOR_4, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(PA12), interruptPin, LOW);
  delay(100);
  
  initSD();
  readSD();
}

void loop() {
//    Serial2.println("[323.23;344.33;51]");
  if (Serial2.available()>0){
    char n = Serial2.read();
    switch (n){
      case 'w':
        pseuSD();
        break;
      case 'r':
        readSD();
        break;
      default:
        break;
      }
  }
  readInputOutput();
}

//================================================================================//

void interruptPin() {
  rpmcount++;
}

float calculatePressure(float pressureAnalog){
  float pressureVolt = pressureAnalog / 1023;
  float pressureRead = ((pressureVolt - 0.33) / 3) * 5;
  return pressureRead;
}

//================================================================================//

void initSD(){
  // Initialize SD card module
  while (!SD.begin(SD_CS_PIN)) {
    Serial2.println("SD card initialization failed!");
    return;
  }
  Serial2.println("SD card initialization successful!");
  }

void readSD(){
  initSD();
  uint16_t val;
  int j=0;
  
  // Read file from SD card
  for(int y = 0;y<8;y++){
    String nameFile = "data"+String(y)+".csv";
    File file = SD.open(nameFile, FILE_READ);
    delay(20);
    if (file) {
      while (file.available()) {
        for(i=0;i<cells*3;i++){
          dataChar = file.read();
          switch (dataChar){
            case ';':
              val = (uint16_t)strtoul(dataString.c_str(), NULL, 10);
              table.int_data[j] = val;
              dataString = "";
              j++;
              break;
            default:
              dataString += dataChar;
              break;
          }
        }
      }
      file.close();
    } else {
      Serial2.println("File not found.");
    }
   }
      for (j = 0; j < cells; j++) {
        Serial2.print("Throttle = ");
        Serial2.print(table.parsed[j].throttle);
        Serial2.print(" RPM = ");
        Serial2.print(table.parsed[j].rpm);
        Serial2.print(" Injection = ");
        Serial2.println(table.parsed[j].injection);
      }
    Serial2.println("Done read.");
  }

void writeSD(char tempChars[numChars], char index){
//  Serial2.println("writing..");
   String nameFile = "data"+String(index)+".csv";
   File logFile = SD.open(nameFile, FILE_WRITE | O_TRUNC);
   delay(20);
   logFile.println(tempChars);
   logFile.close();
   Serial2.println(tempChars);
  }
  
// w0CC<2131;4124;5125;1251;...> , data delimiter dengan ;
// w+IDX+HEX+<data>, HEX disini niatnya buat pointer tabel cell yang mau diubah atau 0 untuk write semua
// ex: w00F<2131;4124;5125>, 0 = file ke-0, 0F = cell ke-10

void pseuSD(){
    char hexCell[2];
    char n;

    // baca index file
    n = Serial2.read();
    char fileIndex = n;

    // baca index cell
    for (int i = 0;i<2;i++){
      n = Serial2.read();
      hexCell[i] = n;
    }
    unsigned long ul = strtoul(hexCell, NULL, 16);
    Serial2.println(ul);
    
    recvWithStartEndMarkers();
    if (newData == true) {
        strcpy(tempChars, receivedChars);
        // this temporary copy is necessary to protect the original data
        writeSD(tempChars, fileIndex);
        parseData();
        newData = false;
    }
  }

//================================================================================//

void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;
    int scCount=0;
    int cellCount=0;
    int x,y,z = 0;  

    while (newData == false) {
        while(!Serial2.available())
          ;
        rc = Serial2.read();
        if (recvInProgress == true) {
            if (rc == endMarker ||scCount == cells) {
                  receivedChars[ndx] = '\0'; // terminate the string
                  recvInProgress = false;
                  ndx = 0;
                  newData = true;
            }
            else{
                receivedChars[ndx] = rc;
                ndx++;
                if (rc==';'){
                  scCount++;
                  Serial2.println(scCount);
                  if ((scCount)%3==0){
                    receivedChars[ndx]='\n';
                    ndx++;
                    }
                }
            }
        }
        else if (rc == startMarker) {
            recvInProgress = true;
        }
        cellCount =0;
    }
}

//================================================================================//

void parseData() {      // split the data into its parts
    char * strtokIndx; // this is used by strtok() as an index
    char * pseuCell[cells];
    byte index = 0;
    strtokIndx = strtok(tempChars,";");      // get the first part - the string

    while (strtokIndx != NULL)
    {
      pseuCell[index] = strtokIndx;
      index++;
      strtokIndx = strtok(NULL,";");
    }
    Serial2.println("The Pieces separated by strtok()");
    for (int n = 0; n < index; n++)
    {
       Serial2.print(n);
       Serial2.print("  ");
       Serial2.println(pseuCell[n]);
    }
}

//================================================================================//

 void manualPWM(int dutyCycle){
  pwm_t = millis();
  while (millis() - pwm_t <= 20){
    if (millis() - pwm_t < (dutyCycle * 100 / 2048) * 0.2){
      digitalWrite(INJECTOR_1, HIGH);
      digitalWrite(INJECTOR_2, HIGH);
      digitalWrite(INJECTOR_3, HIGH);
      digitalWrite(INJECTOR_4, HIGH);
    } else {
      digitalWrite(INJECTOR_1, LOW);
      digitalWrite(INJECTOR_2, LOW);
      digitalWrite(INJECTOR_3, LOW);
      digitalWrite(INJECTOR_4, LOW);
    }
  }
  pwm_t = 0;
 }
//============================//

void readInputOutput() {
  if (millis() - timeold >= 1000) { /*Update every one second, this will be equal to reading frecuency (Hz).*/
    detachInterrupt(0);  //Disable interrupt when calculating

    pressure_read = analogRead(PA1);
    // pressure = calculatePressure(pressure_read);
    pressure = 2;

    rpm_read = round(rpmcount * 60);
//    Serial2.println(rpm_read);
    
    throttle_read = analogRead(PA0) * 3300 / 1023;
    
     for (i = 0; i <cells; i++) {
//         Serial2.println(table.parsed[i].rpm);
     if (table.parsed[i].rpm > rpm_read && table.parsed[i].throttle > throttle_read) {
         table_cell = table.parsed[i-1];
         break;
       } else {
       };
     }

//  IMPORTANT!
    duty_nx = dedaf_solve_injection_delay(&table_cell, cells, throttle_read, rpm_read);
      
//    manualPWM(duty_nx);
      duty_fs = duty_nx-1;
        
    if (duty_nx != duty_fs && pressure > 1.2) {
      // out_duty_fs = (duty_fs * 50 / 1000) * 2047;
      pwm_start(INJECTOR_1, 50, duty_fs, RESOLUTION_11B_COMPARE_FORMAT);
      pwm_start(INJECTOR_2, 50, duty_fs, RESOLUTION_11B_COMPARE_FORMAT);
      pwm_start(INJECTOR_3, 50, duty_fs, RESOLUTION_11B_COMPARE_FORMAT);
      pwm_start(INJECTOR_4, 50, duty_fs, RESOLUTION_11B_COMPARE_FORMAT);  // used for Dutycycle: [0 .. 2047]
    } 

    Serial2.print("[");
    Serial2.print(throttle_read);
    Serial2.print(",");
    Serial2.print(rpm_read);
    Serial2.print(",");

    rpmcount = 0;        // Restart the RPM counter
    timeold = millis();  // Update lasmillisin
    attachInterrupt(digitalPinToInterrupt(PA12), interruptPin, LOW);
  }
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
