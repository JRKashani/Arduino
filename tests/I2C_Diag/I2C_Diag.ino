#include <Wire.h>

void setup()
{
    Serial.begin(9600);
    Wire.begin();

    Serial.println("I2C scanner");
}

void loop()
{
    byte error;
    byte address;
    int devices = 0;

    Serial.println("Scanning...");

    for (address = 1; address < 127; address++)
    {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0)
        {
            Serial.print("I2C device found at 0x");
            if (address < 16)
                Serial.print("0");

            Serial.println(address, HEX);

            devices++;
        }
    }

    if (devices == 0)
        Serial.println("No I2C devices found.");

    delay(2000);
}