#include <ManchesterRF.h>
#include <avr/sleep.h>
#include "tinysnore.h"

#define TX_PIN 0 
#define LED_PIN 1
#define num_of_read 1 

ManchesterRF rf(MAN_4800); //link speed, try also MAN_300, MAN_600, MAN_1200, MAN_2400, MAN_4800, MAN_9600, MAN_19200, MAN_38400

uint8_t data = 0;
float SenV10K = 0, SenVWM1 = 0, SenVWM2 = 0, ARead_A1 = 0, ARead_A2 = 0, WM_Resistance = 0, WM1_Resistance = 0 ;
int i = 0;
const int SupplyV = 5;
const int Rx = 10000; 

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

  //WDTCR |= (1 << WDCE) | (1 << WDE); // Enable Watchdog Timer configuration mode
  //WDTCR = (1 << WDP2) | (1 << WDP1) | (1 << WDP0); // Set timeout to 1 second
  //WDTCR |= (1 << WDIE); // Enable Watchdog Timer interrupt

  //wdt_reset();
  
  //sei(); // Enable global interrupts

  // Set the sleep mode to power-down
  //set_sleep_mode(SLEEP_MODE_PWR_DOWN);
}

//ISR(WDT_vect) {
//    // Watchdog Timer interrupt service routine (ISR)
//    // This code runs every time the Watchdog Timer triggers
//    // Example: Toggle an LED
//    PORTB ^= (1 << PB3); // Toggle PB3
//
//}

void loop() {

  WM1_Resistance = readWMsensor();
  rf.transmitByte(WM1_Resistance);
  digitalWrite(LED_PIN, LOW); //blink the LED on receive
  //*****************output************************************

  Serial.print("WM1 Resistance(Ohms)= ");
  Serial.print(WM1_Resistance);
  Serial.print("\n");

  snore(2000);
  //sleep_enable();
  //sleep_mode();
  //delay(5000);

  //sleep_disable();
  digitalWrite(LED_PIN, HIGH);
  snore(2000);
}

float readWMsensor() {  //read ADC and get resistance of sensor

  ARead_A1 = 0;
  ARead_A2 = 0;

  for (i = 0; i < num_of_read; i++) //the num_of_read initialized above, controls the number of read successive read loops that is averaged.
  {

    digitalWrite(3, HIGH);   //Set pin 5 as Vs
    delayMicroseconds(90); //wait 90 micro seconds and take sensor read
    ARead_A1 += analogRead(A1); // read the analog pin and add it to the running total for this direction
    Serial.print("\n");
    Serial.print(ARead_A1);
    digitalWrite(3, LOW);      //set the excitation voltage to OFF/LOW

    delay(100); //0.1 second wait before moving to next channel or switching MUX

    // Now lets swap polarity, pin 5 is already low

    digitalWrite(4, HIGH); //Set pin 11 as Vs
    delayMicroseconds(90); //wait 90 micro seconds and take sensor read
    ARead_A2 += analogRead(A1); // read the analog pin and add it to the running total for this direction
    Serial.print("\n");
    Serial.print(ARead_A2);
    Serial.print("\n");
    digitalWrite(4, LOW);      //set the excitation voltage to OFF/LOW
  }

  SenVWM1 = ((ARead_A1 / 1024) * SupplyV) / (num_of_read); //get the average of the readings in the first direction and convert to volts
  SenVWM2 = ((ARead_A2 / 1024) * SupplyV) / (num_of_read); //get the average of the readings in the second direction and convert to volts

  float WM_ResistanceA = (Rx * (SupplyV - SenVWM1) / SenVWM1); //do the voltage divider math, using the Rx variable representing the known resistor
  float WM_ResistanceB = Rx * SenVWM2 / (SupplyV - SenVWM2);  // reverse
  float WM_Resistance = ((WM_ResistanceA + WM_ResistanceB) / 2); //average the two directions
  return WM_Resistance;

  // cant send double or float do all this math in reciever instead
}