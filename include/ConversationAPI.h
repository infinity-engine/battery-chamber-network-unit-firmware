#ifndef ConversationAPI_H
#define ConversationAPI_H
#include <Arduino.h>

#include "ArduinoJson.h"
class NetWorkManager;
class MemoryAPI;
#include "InstructionsHandler.h"

class ConversationAPI
{
public:
    ConversationAPI();
    // to know whether everything in esp is ok, including sd, wifi, network, etc
    bool isReady;
    void detectMsgID(NetWorkManager &net_man, MemoryAPI &mem_api);
    void clearIncomingBuffer();
    void testInfoReset();
    void setup();
    void sendTestInfo(MemoryAPI &mem_api, NetWorkManager &net_man);
    void checkForEXP(NetWorkManager &net_man, MemoryAPI &mem_api);
    bool initSDForEXP(MemoryAPI &mem_api, uint8_t channelNo);
    uint8_t onNoOfChannel; // tells on which channel curently data are to be sent
    uint8_t onNoOfRowOfCh; // tells about on which rows info has to be been sent to for the current channel
    uint8_t noOfChannels;  // total no of chennels available
    uint8_t noOfRows;      // total no of rows for the current channel
    String testId;
    String rowInfo; // stores row info of a particular row
    uint64_t filePostion;
    const char *expConfigPath = "EXP_CONFIG.json";
    StaticJsonDocument<256> rowInfoDoc;
    StaticJsonDocument<400> configDoc;
    bool isRowInfoSent;
    bool isDriveCycleForRowSent;
};
#endif