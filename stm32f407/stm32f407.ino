#define pin  PB5
#include <EEPROM.h>
float adcVal = 0;
volatile int rpmcount = 0;
volatile unsigned int rpm=0; //contador de rpm
volatile unsigned long timeold=0;
volatile unsigned long tnew;
boolean done = false;
uint32_t duty_fs = 0;
uint32_t duty_nx = 10;
int Table[3][3] = {
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0}
};
HardwareSerial Serial2(PA3, PA2);
HardwareTimer pwmtimer(TIM1);
void getEE();
void interruptPin();
void setup() {
  Serial2.begin(115200);
  pinMode(PC1,INPUT_ANALOG);
  attachInterrupt(digitalPinToInterrupt(PE9), interruptPin, FALLING);
  rpm = 0;
  getEE();
}

void loop() {
  if (millis() - timeold == 1000){  /*Uptade every one second, this will be equal to reading frecuency (Hz).*/
   detachInterrupt(0);    //Disable interrupt when calculating
   rpm = rpmcount * 60;
   int a = rpm/2400;
   adcVal = analogRead(PC1);
   int b = adcVal/300; //max1000
   duty_nx = Table[a][b];
   if (duty_nx != duty_fs){
    duty_fs = duty_nx;
    pwm_start(PB_5,50,duty_fs, RESOLUTION_11B_COMPARE_FORMAT);  // used for Dutycycle: [0 .. 2047]
  }
   Serial2.println("hasil");
   Serial2.println(rpm);
   Serial2.println(adcVal);
   Serial2.println(duty_fs);
   rpmcount = 0; // Restart the RPM counter
   timeold = millis(); // Uptade lasmillis
   attachInterrupt(digitalPinToInterrupt(PE9), interruptPin, FALLING); 
  } 
}

void getEE(){
if (done == false){
  //for (int r =0; r<9; r++){
    //EEPROM.write(r, pseudoEEPROM[r]);
    //delay(100);}
  for (int l = 0; l < 3; l++) {
    for (int j = 0; j < 3; j++) {
      Table[l][j] = EEPROM.read(((l+1)*(j+1)-1));
      Serial2.println(Table[l][j]);
      }
  }
  done = true;
}
}

void interruptPin()
{
  rpmcount++;
}
