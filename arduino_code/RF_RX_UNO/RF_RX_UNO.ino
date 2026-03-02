#include <ManchesterRF.h>

#define RX_PIN 4 //any pin can transmit
#define LED_PIN 2

ManchesterRF rf(MAN_4800); //link speed, try also MAN_300, MAN_600, MAN_1200, MAN_2400, MAN_4800, MAN_9600, MAN_19200, MAN_38400

uint8_t a, b;
uint8_t size;

typedef struct {

    char ID;   // TRANSMITTER ID
    uint8_t A1;  // Analogue Sensor Read
    uint8_t A2;  // Analalogue Sensor read (current other direction)

} mssg_t;

mssg_t mssg;

void setup() {
  pinMode(LED_PIN, OUTPUT);  
  digitalWrite(LED_PIN, LOW);
  rf.RXInit(RX_PIN);
  Serial.begin(4800);
}

void loop() {
  if (rf.available()) { //something is in RX buffer
    if (rf.receiveArray(size, (uint8_t **)&mssg)) {
      Serial.println("Recieved rx:");
      Serial.println(mssg.ID);
      Serial.println("Recieved A1:");
      Serial.println(mssg.A1);
      Serial.println("Recieved A2:");
      Serial.println(mssg.A2);
      digitalWrite(LED_PIN, HIGH); //blink the LED on receive}
      delay(10000);
      digitalWrite(LED_PIN, LOW);
    }
  }
  else {
    Serial.println("nada");
  }

}

