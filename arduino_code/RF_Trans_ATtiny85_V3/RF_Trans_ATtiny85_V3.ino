#include <ManchesterRF.h>
#include <avr/sleep.h>
#include "tinysnore.h"


#define TX_PIN 0 
#define LED_PIN 1
#define num_of_read 1 

ManchesterRF rf(MAN_4800); //link speed, try also MAN_300, MAN_600, MAN_1200, MAN_2400, MAN_4800, MAN_9600, MAN_19200, MAN_38400

uint8_t data = 0, WM1_ResistanceA = 0, WM1_ResistanceB = 0, avg_A1 = 0, avg_A2 = 0;
int ARead_A1 = 0, ARead_A2 = 0;

int i = 0 ;
const int SupplyV = 5;
const int Rx = 10000; 

typedef struct {

    char ID;   // TRANSMITTER ID
    uint8_t A1;  // Analogue Sensor Read
    uint8_t A2;  // Analalogue Sensor read (current other direction)

} mssg_t;

void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(4800);
  // initialize the pins, 5 and 11 randomly chosen. In the voltage divider circuit example in figure 1(www.irrometer.com/200ss.html), pin 11 is the "Output Pin" and pin 5 is the "GND".
  // if the direction is reversed, the WM1_Resistance A and B formulas would have to be swapped.

  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  //set both low
  digitalWrite(3, LOW);
  digitalWrite(4, LOW);

  
  pinMode(LED_PIN, OUTPUT);  
  digitalWrite(LED_PIN, HIGH);
  rf.TXInit(TX_PIN);

}



void loop() {

  //WM1_ResistanceA,WM1_ResistanceB = readWMsensor();

//------------------------- 2-28-26------------------------------------------------
  mssg_t mssg = readWMsensor();


  rf.transmitArray(sizeof(mssg), (uint8_t *)&mssg);

  //rf.transmitByte(WM1_ResistanceA);
  digitalWrite(LED_PIN, LOW); //blink the LED on trans
  //*****************output************************************

  //Serial.print("WM1 Resistance(Ohms)= ");
  //Serial.print(WM1_ResistanceA);
  //Serial.print("\n");

  snore(2000);
  //sleep_enable();
  //sleep_mode();
  //delay(5000);

  //sleep_disable();
  digitalWrite(LED_PIN, HIGH);

  //rf.transmitByte(WM1_ResistanceA);
  //digitalWrite(LED_PIN, LOW); 
  //Serial.print("WM1 Resistance(Ohms)= ");
  //Serial.print(WM1_ResistanceB);
  //Serial.print("\n");
}

mssg_t readWMsensor() {  //read ADC and get resistance of sensor
  mssg_t Reads;
  ARead_A1 = 0;
  ARead_A2 = 0;

  for (i = 0; i < num_of_read; i++) //the num_of_read initialized above, controls the number of read successive read loops that is averaged.
  {

    digitalWrite(3, HIGH);   //Set pin 3 as Vs
    delayMicroseconds(90); //wait 90 micro seconds and take sensor read
    ARead_A1 += analogRead(A1); // read the analog pin and add it to the running total for this direction
    Serial.print("\n");
    Serial.print(ARead_A1);
    digitalWrite(3, LOW);      //set the excitation voltage to OFF/LOW

    delay(100); //0.1 second wait before moving to next channel or switching MUX

    // Now lets swap polarity, pin 3 is already low

    digitalWrite(4, HIGH); //Set pin 4 as Vs
    delayMicroseconds(90); //wait 90 micro seconds and take sensor read
    ARead_A2 += analogRead(A1); // read the analog pin and add it to the running total for this direction
    Serial.print("\n");
    Serial.print(ARead_A2);
    Serial.print("\n");
    digitalWrite(4, LOW);      //set the excitation voltage to OFF/LOW
  }
  avg_A1 = ARead_A1 / num_of_read;
  avg_A2 = ARead_A2 / num_of_read;

  Reads.ID = 'a';  
  Reads.A1 = avg_A1;
  Reads.A2 = avg_A2;

  return Reads;

  // cant send double or float do all this math in reciever instead
}