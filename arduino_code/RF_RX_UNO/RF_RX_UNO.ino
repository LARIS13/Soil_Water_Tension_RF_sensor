#include <Manchester.h>

#define RX_PIN 4 //any pin can transmit
#define LED_PIN 2

uint8_t a, b;
uint8_t size;

#define BUFFER_SIZE 4
uint8_t buffer[BUFFER_SIZE];

void setup() {
  pinMode(LED_PIN, OUTPUT);  
  digitalWrite(LED_PIN, LOW);
  Serial.begin(9600);
  man.setupReceive(RX_PIN, MAN_4800);
  man.beginReceiveArray(BUFFER_SIZE, buffer);
}

void loop() {
  if (man.receiveComplete()) { //something is in RX buffer
      Serial.println("Recieved TX:");
      Serial.println(buffer[3]);
      Serial.println("Recieved A1:");
      Serial.println(buffer[1]);
      Serial.println("Recieved A2:");
      Serial.println(buffer[2]);
      digitalWrite(LED_PIN, HIGH); //blink the LED on receive
      man.beginReceiveArray(BUFFER_SIZE, buffer);
      delay(200);
      digitalWrite(LED_PIN, LOW);
  }
}

