#include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();

const int TX_PIN = 25;

// Set own measured data
const unsigned long RF_CODE = 0;
const int RF_BIT_LENGTH = 12;

// Impulse timing in microseconds (us)
// Set according to your own measurements
const int RF_PULSE_US = 0;

void setup() {
  Serial.begin(115200);

  mySwitch.enableTransmit(TX_PIN);
  mySwitch.setProtocol(11);
  mySwitch.setRepeatTransmit(10);

  // No transmission without setting opening key
  if (RF_CODE == 0 || RF_PULSE_US <= 0) {
    Serial.println("Configure RF_CODE and RF_PULSE_US first.");
    return;
  }

  mySwitch.setPulseLength(RF_PULSE_US);
  mySwitch.send(RF_CODE, RF_BIT_LENGTH);
}

void loop() {
  // No automatic loop
}