#include <ManchesterRF.h>
#include <avr/sleep.h>
#include "tinysnore.h"
#include <math.h>

#define num_of_read 1 // number of iterations, each is actually two reads of the sensor (both directions)
#define TX_PIN 0 
#define LED_PIN 2

//As a simplified example, this version reads one sensor only and assumes a default temperature of 24C.
//NOTE: the 0.09 excitation time may not be sufficient depending on circuit design, cable lengths, voltage, etc. Increase if necessary to get accurate readings, do not exceed 0.2
//NOTE: this code assumes a 10 bit ADC. If using 12 bit, replace the 1024 in the voltage conversions to 4096

const int Rx = 10000;  //fixed resistor attached in series to the sensor and ground...the same value repeated for all WM and Temp Sensor.
const long default_TempC = 20;
const long open_resistance = 35000; //check the open resistance value by replacing sensor with an open and replace the value here...this value might vary slightly with circuit components
const long short_resistance = 200; // similarly check short resistance by shorting the sensor terminals and replace the value here.
const long short_CB = 240, open_CB = 255 ;
const int SupplyV = 5; // Assuming 5V output for SupplyV, this can be measured and replaced with an exact value if required
const float cFactor = 1.1; //correction factor optional for adjusting curve, 1.1 recommended to match IRROMETER devices as well as CS CR1000
int i, j = 0, WM1_CB = 0;
float SenV10K = 0, SenVWM1 = 0, SenVWM2 = 0, ARead_A1 = 0, ARead_A2 = 0, WM_Resistance = 0, WM1_Resistance = 0 ;

ManchesterRF rf(MAN_9600); //link speed, try also MAN_300, MAN_600, MAN_1200, MAN_2400, MAN_4800, MAN_9600, MAN_19200, MAN_38400

uint8_t data = 0;

void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(9600);
  // initialize the pins, 5 and 11 randomly chosen. In the voltage divider circuit example in figure 1(www.irrometer.com/200ss.html), pin 11 is the "Output Pin" and pin 5 is the "GND".
  // if the direction is reversed, the WM1_Resistance A and B formulas would have to be swapped.
  pinMode(5, OUTPUT);
  pinMode(11, OUTPUT);
  //set both low
  digitalWrite(5, LOW);
  digitalWrite(11, LOW);

  
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

  rf.transmitByte(data);
  digitalWrite(LED_PIN, LOW); //blink the LED on receive

  snore(2000);
  //sleep_enable();
  //sleep_mode();
  //delay(5000);

  //sleep_disable();
  digitalWrite(LED_PIN, HIGH);
  snore(2000);
}

