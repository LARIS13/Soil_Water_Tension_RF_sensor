#include <Manchester.h>
#include <avr/sleep.h>
#include "tinysnore.h"

#define TX_PIN 0 
#define LED_PIN 1
#define num_of_read 1 

uint8_t WM1_ResistanceA = 0, WM1_ResistanceB = 0, avg_A1 = 0, avg_A2 = 0;
uint8_t ARead_A1 = 0, ARead_A2 = 0;

int i = 0 ;
const int SupplyV = 5;
const int Rx = 10000; 

#define asize 3

uint8_t mssg[4] = {4,avg_A1, avg_A2,1};

void readWMsensor(uint8_t data[4]) {  //read ADC and get resistance of sensor
  ARead_A1 = 0;
  ARead_A2 = 0;

  for (i = 0; i < num_of_read; i++) //the num_of_read initialized above, controls the number of read successive read loops that is averaged.
  {
    digitalWrite(3, HIGH);   //Set pin 3 as Vs
    delayMicroseconds(90); //wait 90 micro seconds and take sensor read
    ARead_A1 += analogRead(A1); // read the analog pin and add it to the running total for this direction

    digitalWrite(3, LOW);      //set the excitation voltage to OFF/LOW

    delay(100); //0.1 second wait before moving to next channel or switching MUX

    // Now lets swap polarity, pin 3 is already low

    digitalWrite(4, HIGH); //Set pin 4 as Vs
    delayMicroseconds(90); //wait 90 micro seconds and take sensor read
    ARead_A2 += analogRead(A1); // read the analog pin and add it to the running total for this direction
    //Serial.print("\n");
    //Serial.print(ARead_A2);
    //Serial.print("\n");
    digitalWrite(4, LOW);      //set the excitation voltage to OFF/LOW
  }
  avg_A1 = ARead_A1 / num_of_read;
  avg_A2 = ARead_A2 / num_of_read;

  data[1] = avg_A1;
  data[2] = avg_A2;
  // cant send double or float do all this math in reciever instead
}
 
void setup() {
  // initialize serial communications at 9600 bps:
  //Serial.begin(4800);
  // initialize the pins, 5 and 11 randomly chosen. In the voltage divider circuit example in figure 1(www.irrometer.com/200ss.html), pin 11 is the "Output Pin" and pin 5 is the "GND".
  // if the direction is reversed, the WM1_Resistance A and B formulas would have to be swapped.
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  //set both low
  digitalWrite(3, LOW);
  digitalWrite(4, LOW);
  //add this in order for transmitter to work with 1Mhz Attiny85/84
  //man.workAround1MhzTinyCore(); 
  man.setupTransmit(TX_PIN, MAN_4800);
  
  pinMode(LED_PIN, OUTPUT);  
  digitalWrite(LED_PIN, HIGH);
}

void loop() {
  //WM1_ResistanceA,WM1_ResistanceB = readWMsensor();
//------------------------- 2-28-26------------------------------------------------
  readWMsensor(mssg);

  man.transmitArray(sizeof(mssg), (uint8_t *)&mssg);
  //rf.transmitByte(WM1_ResistanceA);
  digitalWrite(LED_PIN, LOW); //blink the LED on trans

  snore(2000);

  digitalWrite(LED_PIN, HIGH);
}
