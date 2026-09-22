#include <RCSwitch.h>

RCSwitch rfReceiver;

const uint8_t receiverDataPin = 34;

void setup()
{
    Serial.begin(115200);

    // RXB60 data output is connected to ESP32 GPIO 34.
    rfReceiver.enableReceive(digitalPinToInterrupt(receiverDataPin));

    Serial.println("RCSwitch signal test");
    Serial.println("Press the remote control button...");
}

void loop()
{
    if (!rfReceiver.available())
        return;

    Serial.println();
    Serial.println("===== RECEIVED SIGNAL =====");

    Serial.print("Value: ");
    Serial.println(rfReceiver.getReceivedValue());

    Serial.print("Bit length: ");
    Serial.println(rfReceiver.getReceivedBitlength());

    Serial.print("Protocol: ");
    Serial.println(rfReceiver.getReceivedProtocol());

    Serial.print("Delay: ");
    Serial.println(rfReceiver.getReceivedDelay());

    Serial.println("Raw timings:");

    unsigned int* rawTimings = rfReceiver.getReceivedRawdata();

    for (uint8_t i = 0; i < 50; i++)
    {
        if (rawTimings[i] == 0)
            break;

        if (i > 0)
            Serial.print(" ");

        Serial.print(rawTimings[i]);
    }

    Serial.println();
    Serial.println("===========================");

    rfReceiver.resetAvailable();
}
