#include <RCSwitch.h>

RCSwitch rfReceiver;

const uint8_t receiverDataPin = 34;

const uint8_t buttonCount = 3;
const uint8_t remoteCount = 2;
const uint8_t samplesPerButton = 10;
const uint8_t maxRawTimings = 50;

uint8_t currentButton = 1;
uint8_t currentRemote = 1;
uint8_t currentSample = 1;

void printMeasurementHeader();
void advanceToNextMeasurement();

void printMeasurementHeader()
{
    Serial.println();
    Serial.println("====================================");

    Serial.print("Button: ");
    Serial.print(currentButton);
    Serial.print("/");
    Serial.print(buttonCount);

    Serial.print("   Remote: ");
    Serial.print(currentRemote);
    Serial.print("/");
    Serial.print(remoteCount);

    Serial.print("   Sample: ");
    Serial.print(currentSample);
    Serial.print("/");
    Serial.println(samplesPerButton);

    Serial.println("Press the corresponding remote button...");
    Serial.println("====================================");
}

void advanceToNextMeasurement()
{
    currentSample++;

    if (currentSample > samplesPerButton)
    {
        currentSample = 1;
        currentRemote++;

        if (currentRemote > remoteCount)
        {
            currentRemote = 1;
            currentButton++;

            if (currentButton > buttonCount)
            {
                Serial.println();
                Serial.println("####################################");
                Serial.println(" ALL MEASUREMENTS COMPLETED");
                Serial.println("####################################");

                while (true)
                {
                }
            }

            Serial.println();
            Serial.println("====================================");
            Serial.println(">>> NEXT BUTTON <<<");
            Serial.print("Now measuring button ");
            Serial.print(currentButton);
            Serial.println(".");
            Serial.println("====================================");
        }
        else
        {
            Serial.println();
            Serial.println("====================================");
            Serial.println(">>> SWITCH TO THE OTHER REMOTE <<<");
            Serial.println("====================================");
        }
    }

    delay(1200);
    printMeasurementHeader();
}

void setup()
{
    Serial.begin(115200);

    // RXB60 data output is connected to ESP32 GPIO 34.
    rfReceiver.enableReceive(digitalPinToInterrupt(receiverDataPin));
    Serial.println("Measurement log.");

    printMeasurementHeader();
}

void loop()
{
    if (!rfReceiver.available())
        return;

    Serial.print("DATA,");
    Serial.print(currentButton);
    Serial.print(",");
    Serial.print(currentRemote);
    Serial.print(",");
    Serial.print(currentSample);
    Serial.print(",");
    Serial.print(rfReceiver.getReceivedValue());
    Serial.print(",");
    Serial.print(rfReceiver.getReceivedBitlength());
    Serial.print(",");
    Serial.print(rfReceiver.getReceivedProtocol());
    Serial.print(",");
    Serial.print(rfReceiver.getReceivedDelay());

    unsigned int* rawTimings = rfReceiver.getReceivedRawdata();

    for (uint8_t index = 0; index < maxRawTimings; index++)
    {
        if (rawTimings[index] == 0)
            break;

        Serial.print(",");
        Serial.print(rawTimings[index]);
    }

    Serial.println();

    Serial.println("Measurement recorded.");

    rfReceiver.resetAvailable();

    advanceToNextMeasurement();
}