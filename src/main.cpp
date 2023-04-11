/**
   BasicHTTPClient.ino

    Created on: 24.05.2015

*/

#include <Arduino.h>
#include "functionPrototype.h"
#include "NetWorkManager.h"
#include "MemoryAPI.h"
#include "ConversationAPI.h"

NetWorkManager net_manager;
MemoryAPI memory_api;
ConversationAPI conversation_api;

void test();
void setup()
{
  // Serial.setRxBufferSize(SERIAL_RX_BUFFER_SIZE);
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  // Serial.setDebugOutput(true);
  net_manager.setup();
  memory_api.setup();
  conversation_api.isReady = true;
  blink(2000);
}
void loop()
{
  conversation_api.detectMsgID(net_manager, memory_api);
}

void test()
{
}
