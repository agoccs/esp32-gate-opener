#include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();

const int DATA_PIN = 34;

const int BUTTONS = 3;
const int REMOTES = 2;
const int SAMPLES = 10;

int buttonID = 1;
int remoteID = 1;
int sampleID = 1;

void printHeader()
{
  Serial.println();
  Serial.println("====================================");
  Serial.print("GOMB: ");
  Serial.print(buttonID);
  Serial.print("/3   ");

  Serial.print("Távirányító: ");
  Serial.print(remoteID);
  Serial.print("/2   ");

  Serial.print("Nyomás: ");
  Serial.print(sampleID);
  Serial.print("/");
  Serial.println(SAMPLES);

  Serial.println("Nyomd meg a megfelelő gombot...");
  Serial.println("====================================");
}

void nextMeasurement()
{
  sampleID++;

  if(sampleID > SAMPLES)
  {
    sampleID = 1;
    remoteID++;

    if(remoteID > REMOTES)
    {
      remoteID = 1;
      buttonID++;

      if(buttonID > BUTTONS)
      {
        Serial.println();
        Serial.println("####################################");
        Serial.println(" MINDEN MÉRÉS ELKÉSZÜLT");
        Serial.println("####################################");

        while(true);
      }

      Serial.println();
      Serial.println("====================================");
      Serial.println(">>> KÖVETKEZŐ GOMB <<<");
      Serial.print("Most a ");
      Serial.print(buttonID);
      Serial.println(". gomb következik.");
      Serial.println("====================================");
    }
    else
    {
      Serial.println();
      Serial.println("====================================");
      Serial.println(">>> VÁLTS A 2. TÁVIRÁNYÍTÓRA <<<");
      Serial.println("====================================");
    }
  }

  delay(1200);
  printHeader();
}

void setup()
{
  Serial.begin(115200);

  mySwitch.enableReceive(digitalPinToInterrupt(DATA_PIN));

  Serial.println("RXB60 - ESP32 MÉRÉSI ÜZEMMÓD");
  Serial.println("CSV log készül.");

  printHeader();
}

void loop()
{
  if(mySwitch.available())
  {
    Serial.print("DATA,");
    Serial.print(buttonID);
    Serial.print(",");
    Serial.print(remoteID);
    Serial.print(",");
    Serial.print(sampleID);
    Serial.print(",");
    Serial.print(mySwitch.getReceivedValue());
    Serial.print(",");
    Serial.print(mySwitch.getReceivedBitlength());
    Serial.print(",");
    Serial.print(mySwitch.getReceivedProtocol());
    Serial.print(",");
    Serial.print(mySwitch.getReceivedDelay());

    unsigned int* raw = mySwitch.getReceivedRawdata();

    for(int i=0;i<50;i++)
    {
      if(raw[i]==0) break;

      Serial.print(",");
      Serial.print(raw[i]);
    }

    Serial.println();

    Serial.println("✓ Mentve");

    mySwitch.resetAvailable();

    nextMeasurement();
  }
}