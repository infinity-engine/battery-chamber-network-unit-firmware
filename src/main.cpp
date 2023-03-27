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
  net_manager.fetchExp();
  delay(10000);
}
