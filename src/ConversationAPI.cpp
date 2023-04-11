#include "ConversationAPI.h"

// when ever you receive any instruction always run clearbuffer after the instructin
// when ever you send multiline instructin always send \r except last line

ConversationAPI::ConversationAPI()
{
    isReady = false;
    testInfoReset();
}
void ConversationAPI::testInfoReset()
{
    testId = "";
    onNoOfChannel = 0;
    onNoOfRowOfCh = 0;
    noOfChannels = 0;
    noOfRows = 0;
    rowInfo = "";
    filePostion = 0;
    isRowInfoSent = false;
    isDriveCycleForRowSent = true;
}
void ConversationAPI::checkForEXP(NetWorkManager &net_man, MemoryAPI &mem_api)
{
    // look for experiment
    String config = net_man.fetchExp();
    const char *exp_config = config.c_str();

    if (strcmp(exp_config, "null") == 0)
    {
        // matched
        Serial.print(F("NO\r"));
    }
    else
    {
        if (!mem_api.writeFile(exp_config, expConfigPath))
        {
            Serial.print(F("NO\r"));
            return;
        }
        // mem_api.sd.ls("/", LS_R);
        if (IS_LOG_ENABLED)
            mem_api.readFile(expConfigPath);

        testInfoReset();
        testId = mem_api.valueInBetween(expConfigPath, "],\"testId\":\"", "\"");
        if (testId.length() < 1)
        {
            Serial.print(F("NO\r"));
            return;
        }
        net_man.testId = testId;
        testId = testId.substring(testId.length() - 4, testId.length()); // 64252acf428a0869f9a3c7e8 -> c7e8
        IS_LOG_ENABLED
        ? Serial.println(testId)
        : 0;
        Serial.print(F("YES\r"));
    }
}

void ConversationAPI::detectMsgID(NetWorkManager &net_man, MemoryAPI &mem_api)
{
    // arduino Serial.println() sends \r\n at the end
    if (Serial.available())
    {
        String msgId = Serial.readStringUntil('\r');
        clearIncomingBuffer(); // clear all the input buffer after receiving any command
        blink();

        if (msgId == "IS_READY")
        {
            isReady ? Serial.print(F("YES\r")) : Serial.print(F("NO\r"));
        }
        else if (msgId == "IS_EXP")
        {
            checkForEXP(net_man, mem_api);
        }
        ` else if (msgId == "SEND_TEST")
        {
            sendTestInfo(mem_api, net_man);
        }
        else if (msgId == "IN_SD_EXP")
        {
            // initiate sd card for output measurement
            //  followed by
            //  8c4a
            //  4
            //  1,4,2,5
            String expName = Serial.readStringUntil('\r');
            uint8_t noOfChannels = Serial.readStringUntil('\r').toInt();
            const char *channelStr = Serial.readStringUntil('\r').c_str();
            if (initSDForEXP(mem_api, expName, noOfChannels, channelStr))
                Serial.print(F("YES\r"));
            else
                Serial.println(F("NO\r"));
        }

        else if (msgId == "MNT")
        {
            // start receiving measurement and store them in sd card
            // this call happen on interrupt basis so do as least as you should do.
        }

        else
        {
            Serial.print(F("INVALID\r"));
            testId = "";
            clearIncomingBuffer();
        }
    }
}

/**
 * @brief make the output diroctory for measurements
 *
 * @param mem_api
 * @param expName "8c5a"
 * @param channels "1,3,5"
 * @return true
 * @return false
 */
bool ConversationAPI::initSDForEXP(MemoryAPI &mem_api, String expName, uint8_t noOfChannels, const char *channelsStr)
{
    uint8_t channels[noOfChannels];
    const char *c = channelsStr;
    uint8_t i;
    for (i = 0; i < noOfChannels; i++)
    {
        String digit = "";
        while (*c != ',' && *c != '\n' && *c != '\0')
        {
            digit += *c;
        }
        channels[i] = digit.toInt();
    }
    if (i < noOfChannels)
    {
        // All channels not found
        return false;
    }
    // make directory on the name of expName if exists delete
    if (mem_api.sd.chdir("/") && mem_api.sd.exists(expName) && !mem_api.cleanDir(expName))
    {
        return false;
    }

    // create output directory.
    if (!mem_api.sd.chdir("/") || !mem_api.sd.mkdir(expName))
    {
        return false;
    }
    for (i = 0; i < noOfChannels; i++)
    {
        mem_api.file = mem_api.sd.open(expName + "/" + channels[i] + ".csv", O_WRONLY | O_CREAT);
        if (!mem_api.file)
        {
            mem_api.file.close();
            return false;
        }
        mem_api.file.close();
    }
    return true;
}

/**
 * @brief Sends instruction to arduino with the file/dir name to create
 * instruction should look like
 * for directory
 * DIR
 * <DIR_NAME1/PATH1>
 * <DIR_NAME2/PATH2>
 *
 * for file
 * FILE
 * <FILE_NAME/PATH>
 * <FILE_CONTENT>
 *
 * for file directory should exists before
 * for end or error
 * NULL
 */
void ConversationAPI::sendTestInfo(MemoryAPI &mem_api, NetWorkManager &net_man)
{

    if (testId.length() == 0)
    {
        Serial.print(F("INVALID\r"));
        return;
    }
    if (onNoOfRowOfCh > noOfRows)
    {
        onNoOfChannel += 1;
        onNoOfRowOfCh = 0;
        isRowInfoSent = false;
    }
    if (onNoOfChannel > noOfChannels)
    {
        Serial.print(F("NULL\r"));
        testInfoReset();
        return;
    }

    if (onNoOfChannel == 0)
    {
        // send instruction to create a directory
        noOfChannels = mem_api.valueInBetween(expConfigPath, "\"noOfChannels\":", "}").toInt();
        if (noOfChannels > 0)
        {
            Serial.print(F("YES\r"));
            Serial.print(testId);
            Serial.print("\r");
            onNoOfChannel += 1;
        }
        return;
    }
    if (onNoOfRowOfCh == 0)
    {
        rowInfo = mem_api.valueInBetween(expConfigPath, "\"info\":", ",\"steps", &filePostion);
        DeserializationError error = deserializeJson(rowInfoDoc, rowInfo);
        if (error)
        {
            if (IS_LOG_ENABLED)
            {
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.f_str());
            }
            Serial.print(F("NULL\r"));
            testInfoReset();
            return;
        }
        noOfRows = (uint8_t)rowInfoDoc["noOfSubExp"];
        String channelFolder = testId + "_" + (uint8_t)rowInfoDoc["channelNumber"];
        Serial.print(F("DIR\r"));
        delay(100);
        Serial.print(testId + "/" + channelFolder + "/inputs\r");
        delay(100);
        Serial.print(testId + "/" + channelFolder + "/outputs\r");
        onNoOfRowOfCh += 1;
        return;
    }
    if (!isRowInfoSent)
    {
        String channelFolder = testId + "_" + (uint8_t)rowInfoDoc["channelNumber"];
        Serial.print(F("FILE\r"));
        delay(100);
        Serial.print(testId + "/" + channelFolder + "/inputs/config.json\r");
        delay(100);
        Serial.print(rowInfo);
        Serial.print("\r");
        isRowInfoSent = true;
        return;
    }
    if (!isDriveCycleForRowSent)
    {
        // send drive cyle
        String channelFolder = testId + "_" + (uint8_t)rowInfoDoc["channelNumber"];
        Serial.print(F("FILE\r"));
        delay(100);
        Serial.print(testId + "/" + channelFolder + "/inputs/" + onNoOfRowOfCh + "_" + "driveCycle.csv\r");
        delay(100);
        mem_api.writeToStream("driveCycle.csv", &Serial);
        Serial.print('\r'); // carriage return to be able to detect by arduino
        Serial.flush();
        // clear the memory
        onNoOfRowOfCh += 1;
        isDriveCycleForRowSent = true;
        return;
    }
    // now send other steps
    String config = "";
    String startKey = "";
    String endKey = "";
    if (onNoOfRowOfCh == 1)
        startKey = "steps\":[";
    else
        startKey = ",";
    if (onNoOfRowOfCh == noOfRows)
        endKey = "]";
    else
        endKey = ",{";

    config = mem_api.valueInBetween(expConfigPath, startKey.c_str(), endKey.c_str(), &filePostion);
    if (config.length() == 0)
    {
        Serial.print(F("NULL\r"));
        testInfoReset();
        return;
    }
    DeserializationError error = deserializeJson(configDoc, config);
    if (error)
    {
        if (IS_LOG_ENABLED)
        {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
        }
        testInfoReset();
        Serial.print(F("NULL\r"));
        return;
    }

    String channelFolder = testId + "_" + (uint8_t)rowInfoDoc["channelNumber"];
    Serial.print(F("FILE\r"));
    delay(100);
    Serial.print(testId + "/" + channelFolder + "/inputs/" + onNoOfRowOfCh + "_" + "config.json\r");
    delay(100);
    Serial.print(config);
    Serial.print("\r");

    if (configDoc["mode"] == 7)
    {
        isDriveCycleForRowSent = false;
        // if it is drive cycle then you have to fetch drive cycle.
        String driveCycleRow = net_man.getDriveCycle(rowInfoDoc["channelNumber"], onNoOfRowOfCh);
        mem_api.writeFile(driveCycleRow.c_str(), "driveCycle.csv");
    }
    else
    {
        onNoOfRowOfCh += 1;
    }
}

void ConversationAPI::clearIncomingBuffer()
{
    int d = 50; // wait for 50ms atleast
    unsigned long t = millis();
    while (millis() < t + d)
    {
        while (Serial.available())
        {
            Serial.read();
        }
    }
}
