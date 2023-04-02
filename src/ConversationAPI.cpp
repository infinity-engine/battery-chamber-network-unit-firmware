#include "ConversationAPI.h"

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
    driveCycleRow = "";
}

void ConversationAPI::detectMsgID(NetWorkManager &net_man, MemoryAPI &mem_api)
{
    if (Serial.available())
    {
        String msgId = Serial.readStringUntil('\n');
        if (msgId == "IS_READY")
        {
            Serial.println(isReady);
        }
        else if (msgId == "IS_EXP")
        {
            // look for experiment
            String config = net_man.fetchExp();
            const char *exp_config = config.c_str();

            if (strcmp(exp_config, "nulll") == 0)
            {
                // matched
                Serial.println("NO");
            }
            else
            {
                if (!mem_api.writeFile(exp_config, expConfigPath))
                {
                    Serial.println("NO");
                    return;
                }
                // mem_api.sd.ls("/", LS_R);
                if (IS_LOG_ENABLED)
                    mem_api.readFile(expConfigPath);

                testInfoReset();
                testId = mem_api.valueInBetween(expConfigPath, "],\"testId\":\"", "\"");
                if (testId.length() < 1)
                {
                    Serial.println("NO");
                    return;
                }
                net_man.testId = testId;
                testId = testId.substring(testId.length() - 4, testId.length()); // 64252acf428a0869f9a3c7e8 -> c7e8
                IS_LOG_ENABLED
                ? Serial.println(testId)
                : 0;
                Serial.println("YES");
            }
        }
        else if (msgId == "SEND_TEST")
        {
            sendTestInfo(mem_api, net_man);
        }
        else
        {
            Serial.println(F("INVALID"));
            ClearIncomingBuffer();
        }
    }
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
        Serial.println(F("NULL"));
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
        Serial.println(F("NULL"));
        testInfoReset();
        return;
    }

    if (onNoOfChannel == 0)
    {
        // send instruction to create a directory
        noOfChannels = mem_api.valueInBetween(expConfigPath, "\"noOfChannels\":", "}").toInt();
        if (noOfChannels > 0)
        {
            Serial.println(F("DIR"));
            Serial.println(testId);
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
            Serial.println("NULL");
            testInfoReset();
            return;
        }
        noOfRows = (uint8_t)rowInfoDoc["noOfSubExp"];
        String channelFolder = testId + "_" + (uint8_t)rowInfoDoc["channelNumber"];
        Serial.println(F("DIR"));
        Serial.println(testId + "/" + channelFolder + "/inputs");
        Serial.println(testId + "/" + channelFolder + "/outputs");
        onNoOfRowOfCh += 1;
        return;
    }
    if (!isRowInfoSent)
    {
        String channelFolder = testId + "_" + (uint8_t)rowInfoDoc["channelNumber"];
        Serial.println(F("FILE"));
        Serial.println(testId + "/" + channelFolder + "/inputs/config.json");
        Serial.println(rowInfo);
        isRowInfoSent = true;
        return;
    }
    if (!isDriveCycleForRowSent && driveCycleRow.length() > 0)
    {
        // send drive cyle
        String channelFolder = testId + "_" + (uint8_t)rowInfoDoc["channelNumber"];
        Serial.println(F("FILE"));
        Serial.println(testId + "/" + channelFolder + "/inputs/" + onNoOfRowOfCh + "_" + "driceCycle.csv");
        Serial.println(driveCycleRow);
        Serial.flush();
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
        Serial.println(F("NULL"));
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
        Serial.println("NULL");
        return;
    }

    String channelFolder = testId + "_" + (uint8_t)rowInfoDoc["channelNumber"];
    Serial.println(F("FILE"));
    Serial.println(testId + "/" + channelFolder + "/inputs/" + onNoOfRowOfCh + "_" + "config.json");
    Serial.println(config);

    if (configDoc["mode"] == 7)
    {
        isDriveCycleForRowSent = false;
        // if it is drive cycle then you have to fetch drive cycle.
        driveCycleRow = net_man.getDriveCycle(rowInfoDoc["channelNumber"], onNoOfRowOfCh);
    }
    else
    {
        onNoOfRowOfCh += 1;
    }
}

void ConversationAPI::ClearIncomingBuffer()
{
    while (Serial.available())
    {
        Serial.read();
    }
}
