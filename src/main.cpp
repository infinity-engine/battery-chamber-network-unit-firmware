/**
   BasicHTTPClient.ino

    Created on: 24.05.2015

*/

#include <Arduino.h>
#include "functionPrototype.h"
#include "NetWorkManager.h"

NetWorkManager net_manager;

void setup()
{

  Serial.begin(2000000);
  // Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();

  for (uint8_t t = 4; t > 0; t--)
  {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }
  net_manager.setup();
}
void loop()
{
  // wait for WiFi connection
  // net_manager.fetchExp();

  // char* input;
  // size_t inputLength; (optional)

  // String measurement = "{\"chamberTemp\":[25,24.3,25,26.1,24.3,24],\"chamberHum\":[75,74,75,74.5,75,74.9],\"cellTemp\":[{\"sensorId\":1,\"values\":[26,25.9,25.8,25.8,25.7,25.9]},{\"sensorId\":2,\"values\":[26,25.9,25.8,25.8,25.7,25.9]}],\"current\":[1,2,3,4,5,6],\"voltage\":[3,3,3,4,3,4],\"time\":[50,51,52,53,54,56]}";

  // net_manager.sendMeasurement(1, measurement);
  delay(10000);
}
