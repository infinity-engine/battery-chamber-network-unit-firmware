#include <Arduino.h>
#include "functionPrototype.h"
#include "NetWorkManager.h"
#include "MemoryAPI.h"
#include "ConversationAPI.h"

NetWorkManager net_manager;
MemoryAPI memory_api;
ConversationAPI conversation_api;
bool IS_LOG_ENABLED = false;

void test();
void setup()
{
  pinMode(D1, OUTPUT);
  digitalWrite(D1, LOW);
  blink(1000);
  interruptArduino(); // if ever inbetwwen sending the data esp crashes this trigger will help
  // arduino determine this situation
  Serial.setRxBufferSize(SERIAL_RX_BUFFER_SIZE);
  Serial.begin(115200);
  net_manager.setup();
  blink(300);
  // memory_api.setup();
  conversation_api.isReady = true;
  blink(2000);
  // test();
}
void loop()
{
  conversation_api.detectMsgID(net_manager, memory_api);
}

void test()
{
  // adding some delay between multiple request is important it reserves time for the reques interrupts to hit
  //  otherwise your code will break for AsyncLibrary
  // atleast add delay(1)
  //  for (uint8_t i = 0; i <= MAX_NO_CHANNELS; i++)
  //  {
  //    if (readyToSend[i])
  //    {
  //      net_manager.sendRequest("GET", HOST_NAME, "", i, false);
  //    }
  //  }
  //  net_manager.sendRequest("GET", HOST_NAME, "", 1, false);
  for (uint8_t i = 0; i < MAX_NO_CHANNELS; i++)
  {
    if (!conversation_api.initSDForEXP(memory_api, i + 1, false))
    {
      Serial.println("Failed");
      return;
    }
  }
  net_manager.testId = "644a4558c5bfc394a93d542d";
  memory_api.wrapup(&net_manager);
}

void espSoftReset()
{
  ESP.reset();
}

void interruptArduino()
{
  digitalWrite(D1, HIGH);
  delay(100);
  digitalWrite(D1, LOW);
}