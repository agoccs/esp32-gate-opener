#include <RCSwitch.h>

RCSwitch rfReceiver = RCSwitch();

void setup()
{
  Serial.begin(115200);

  // RXB60 data output is connected to GPIO 34.
  mySwitch.enableReceive(digitalPinToInterrupt(34));

  Serial.println("RCSwitch raw signal analyzer");
  Serial.println("Press the remote...");
}

void loop()
{
  if (mySwitch.available())
  {
    Serial.println();
    Serial.println("===== SIGNAL =====");

    Serial.print("Value: ");
    Serial.println(mySwitch.getReceivedValue());

    Serial.print("Bits: ");
    Serial.println(mySwitch.getReceivedBitlength());

    Serial.print("Protocol: ");
    Serial.println(mySwitch.getReceivedProtocol());

    Serial.print("Delay: ");
    Serial.println(mySwitch.getReceivedDelay());

    Serial.println("Raw:");

    unsigned int* raw = mySwitch.getReceivedRawdata();

    for (unsigned int i = 0; i < 50; i++)
    {
      if (raw[i] == 0)
        break;

      Serial.print(raw[i]);

      if (i < 49)
        Serial.print(" ");
    }

    Serial.println();
    Serial.println("==================");

    mySwitch.resetAvailable();
  }
}