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
  Serial.setRxBufferSize(SERIAL_RX_BUFFER_SIZE);
  Serial.begin(115200);
  net_manager.setup();
  memory_api.setup();
  conversation_api.isReady = true;
  blink(2000);
  //test();
}
void loop()
{
  conversation_api.detectMsgID(net_manager, memory_api);
}

void test()
{
  //adding some delay between multiple request is important it reserves time for the reques interrupts to hit
  // otherwise your code will break for AsyncLibrary
  //atleast add delay(1)
  // for (uint8_t i = 0; i <= MAX_NO_CHANNELS; i++)
  // {
  //   if (readyToSend[i])
  //   {
  //     net_manager.sendRequest("GET", HOST_NAME, "", i, false);
  //   }
  // }
  // net_manager.sendRequest("GET", HOST_NAME, "", 1, false);
  for (uint8_t i = 0; i < MAX_NO_CHANNELS; i++)
  {
    if (!conversation_api.initSDForEXP(memory_api, i + 1, false))
    {
      Serial.println("Failed");
      return;
    }
  }
  net_manager.testId = "64426047c492337a7c8d119a";
  memory_api.wrapup(&net_manager);
}
