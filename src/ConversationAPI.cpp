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
                mem_api.sd.ls("/", LS_R);
                testInfoReset();
                testId = mem_api.valueInBetween(expConfigPath, ",\"testId\":\"", "\"");
                if (testId.length() < 1)
                {
                    Serial.println("NO");
                    return;
                }
                IS_LOG_ENABLED ? Serial.println(testId) : 0;
                Serial.println("YES");
            }
        }
        else if (msgId == "SEND_TEST_INFO")
        {
            sendTestInfo(mem_api);
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
void ConversationAPI::sendTestInfo(MemoryAPI &mem_api)
{
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
        }
        noOfRows = rowInfoDoc["noOfSubExp"];
        String channelFolder = testId + "_" + rowInfoDoc["channelNumber"];
        Serial.println(F("DIR"));
        Serial.println(testId + "/" + channelFolder + "/inputs");
        Serial.println(testId + "/" + channelFolder + "/outputs");
        onNoOfRowOfCh += 1;
        return;
    }
    if (!isRowInfoSent)
    {
        String channelFolder = testId + "_" + rowInfoDoc["channelNumber"];
        Serial.println(F("FILE"));
        Serial.println(testId + "/" + channelFolder + "/inputs/config.json");
        isRowInfoSent = true;
        return;
    }
    if (onNoOfChannel > noOfChannels)
    {
        Serial.println(F("NULL"));
        testInfoReset();
    }
    // now send other steps
    String config = "";
    String startKey = "";
    String endKey = "";
    if (onNoOfChannel == 1)
        startKey = "steps:\"[";
    else
        startKey = ",";
    if (onNoOfChannel == noOfChannels)
        endKey = "]";
    else
        endKey = ",";

    config = mem_api.valueInBetween(expConfigPath, startKey.c_str(), endKey.c_str(), &filePostion);
    if (config.length() == 0)
    {
        Serial.println(F("NULL"));
        return;
    }
    String channelFolder = testId + "_" + rowInfoDoc["channelNumber"];
    Serial.println(F("FILE"));
    Serial.println(testId + "/" + channelFolder + "/inputs/" + onNoOfRowOfCh + "_" + "config.json");
    Serial.println(config);
    onNoOfRowOfCh += 1;

    if (onNoOfRowOfCh > noOfRows)
    {
        onNoOfChannel += 1;
        onNoOfRowOfCh = 0;
        isRowInfoSent = false;
    }
}
void ConversationAPI::ClearIncomingBuffer()
{
    while (Serial.available())
    {
        Serial.read();
    }
}

/**
 * @brief tells and seek the fiel untill a key is found
 *
 * @param file file pointer
 * @param key const char*
 * @return true on matched
 * @return false on no matched to the end of file
 */
bool ConversationAPI::findKeyInFile(File &file, const char *key)
{
    size_t keyLen = strlen(key);
    char buffer[keyLen + 1];

    while (file.available())
    {
        size_t bytesRead = file.readBytes(buffer, keyLen);
        buffer[bytesRead] = '\0';

        if (strcmp(buffer, key) == 0)
        {
            return true;
        }

        // Move the file pointer back by (keyLen - 1) bytes
        // to check the next possible match
        file.seek(file.position() - bytesRead + 1);
    }

    return false;
}
