/**
   BasicHTTPClient.ino

    Created on: 24.05.2015

*/

#include <Arduino.h>
#include "functionPrototype.h"
#include "NetWorkManager.h"
#include "MemoryAPI.h"

NetWorkManager net_manager;
MemoryAPI memory_api;

void test();
void setup()
{

  Serial.begin(2000000);
  // Serial.setDebugOutput(true);
  net_manager.setup();
  memory_api.setup();
}
void loop()
{
  test();
}

void test()
{
  String measurement = "{\"chamberTemp\":[25,24.3,25,26.1,24.3,24],\"chamberHum\":[75,74,75,74.5,75,74.9],\"cellTemp\":[{\"sensorId\":1,\"values\":[26,25.9,25.8,25.8,25.7,25.9]},{\"sensorId\":2,\"values\":[26,25.9,25.8,25.8,25.7,25.9]}],\"current\":[1,2,3,4,5,6],\"voltage\":[3,3,3,4,3,4],\"time\":[50,51,52,53,54,56]}";
  if (Serial.available())
  {
    int c = Serial.parseInt();
    switch (c)
    {
    case 1:
      net_manager.fetchExp();
      break;
    case 2:
      Serial.println(net_manager.getDriveCycle(1, 1));
      break;
    case 3:
      Serial.println(net_manager.updatedUpto(1));
      break;
    case 4:
      Serial.println(net_manager.setStatus("Running"));
      break;
    case 5:
      Serial.println(net_manager.sendMeasurement(1, measurement));
      break;
    case 6:
      Serial.println(net_manager.incrementMultiPlierIndex(1));
      break;
    default:
      break;
    }
  }
}
