#include "ConversationAPI.h"
#include "NetWorkManager.h"
#include "MemoryAPI.h"

// when ever you receive any instruction always run clearbuffer after the instructin
// when ever you send multiline instructin always send \n except last line

ConversationAPI::ConversationAPI()
{
}

/**
 * @brief
 *
 */
void ConversationAPI::setup()
{
    isReady = false;
    testInfoReset();
    onNoOfChannel = 0;
    onNoOfRowOfCh = 0;
    noOfChannels = 0;
    noOfRows = 0;
    testId = "";
    rowInfo = "";
    filePostion = 0;
    isRowInfoSent = false;
    isDriveCycleForRowSent = true;
}

/**
 * @brief
 *
 */
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

/**
 * @brief
 *
 * @param net_man
 * @param mem_api
 */
void ConversationAPI::checkForEXP(NetWorkManager &net_man, MemoryAPI &mem_api)
{
    // look for experiment
    String config = net_man.fetchExp();
    const char *exp_config = config.c_str();

    if (strcmp(exp_config, "null") == 0)
    {
        // matched
        Serial.print(F("NO\n"));
    }
    else
    {
        if (!mem_api.writeFile(exp_config, expConfigPath))
        {
            Serial.print(F("NO\n"));
            return;
        }
        // mem_api.sd.ls("/", LS_R);
        if (IS_LOG_ENABLED)
            mem_api.readFile(expConfigPath);

        testInfoReset();
        testId = mem_api.valueInBetween(expConfigPath, "],\"testId\":\"", "\"");
        if (testId.length() < 1)
        {
            Serial.print(F("NO\n"));
            return;
        }
        net_man.testId = testId;
        testId = testId.substring(testId.length() - 4, testId.length()); // 64252acf428a0869f9a3c7e8 -> c7e8
        IS_LOG_ENABLED
        ? Serial.println(testId)
        : 0;
        Serial.print(F("YES\n"));
    }
}

/**
 * @brief
 *
 * @param net_man
 * @param mem_api
 */
void ConversationAPI::detectMsgID(NetWorkManager &net_man, MemoryAPI &mem_api)
{
    // arduino Serial.println() sends \n\n at the end
    if (Serial.available())
    {
        String msgId = Serial.readStringUntil('\n');
        blink();

        if (msgId == "IS_READY")
        {
            clearIncomingBuffer(); // clear all the input buffer after receiving any command
            isReady ? Serial.print(F("YES\n")) : Serial.print(F("NO\n"));
        }
        else if (msgId == "IS_EXP")
        {
            clearIncomingBuffer(); // clear all the input buffer after receiving any command
            checkForEXP(net_man, mem_api);
        }
        else if (msgId == "SEND_TEST")
        {
            clearIncomingBuffer(); // clear all the input buffer after receiving any command
            sendTestInfo(mem_api, net_man);
        }
        else if (msgId == "START")
        {
            // initiate sd card for output measurement
            //  START
            //  1,4,2,5
            String ch = "";
            bool isAnyChannel = false;
            while (Serial.available())
            {
                char c = Serial.read();
                if (c != ',' && c != '\n')
                {
                    ch += c;
                }
                else
                {
                    isAnyChannel = true;
                    if (!initSDForEXP(mem_api, ch.toInt()))
                    {
                        Serial.print(F("NO\n"));
                        return;
                    }
                    ch = "";
                }
            }
            if (!isAnyChannel)
            {
                Serial.print(F("NO\n"));
                return;
            }
            net_man.setStatus("Running");
            if (net_man.testId != "" && isPrevReqSuccess[0])
            {
                Serial.print(F("YES\n"));
                // mem_api.beginInterrupt();
            }
            else
            {
                Serial.print(F("NO\n"));
                return;
            }

            mem_api.isContinueReadingInsSerial = true;
            while (mem_api.isContinueReadingInsSerial)
            {
                mem_api.readAndSendInstruction(net_man);
            }
            mem_api.wrapup(&net_man); // END statement has received and send the final status and wrap up everything
            // mem_api.endInterrupt();
            mem_api.setup();
            setup();
            isReady = true;
            clearIncomingBuffer();
        }

        else
        {
            Serial.print(F("INVALID\n"));
            testId = "";
            clearIncomingBuffer();
        }
    }
}

/**
 * @brief
 *
 * @param mem_api
 * @return true
 * @return false
 */
bool ConversationAPI::initSDForEXP(MemoryAPI &mem_api, uint8_t channelNo)
{
    if (!mem_api.sd.chdir("/"))
    {
        return false;
    }
    String fileName = "";
    fileName += channelNo;
    fileName += "_instructions.txt";
    if (mem_api.sd.exists(fileName) && !mem_api.sd.remove(fileName))
    {
        return false;
    }
    mem_api.file = mem_api.sd.open(fileName, O_WRONLY | O_CREAT); // create new file
    mem_api.file.close();                                         // save the file
    mem_api.irhArray[channelNo - 1].setup(channelNo);
    return mem_api.irhArray[channelNo - 1].isSetUp;
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
        Serial.print(F("INVALID\n"));
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
        Serial.print(F("NULL\n"));
        testInfoReset();
        return;
    }

    if (onNoOfChannel == 0)
    {
        // send instruction to create a directory
        noOfChannels = mem_api.valueInBetween(expConfigPath, "\"noOfChannels\":", "}").toInt();
        if (noOfChannels > 0)
        {
            Serial.print(testId);
            Serial.print("\n");
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
            Serial.print(F("NULL\n"));
            testInfoReset();
            return;
        }
        noOfRows = (uint8_t)rowInfoDoc["noOfSubExp"];
        String channelFolder = testId + "_" + (uint8_t)rowInfoDoc["channelNumber"];
        Serial.print(F("DIR\n"));
        delay(100);
        Serial.print(testId + "/" + channelFolder + "/inputs\n");
        delay(100);
        Serial.print(testId + "/" + channelFolder + "/outputs\n");
        onNoOfRowOfCh += 1;
        return;
    }
    if (!isRowInfoSent)
    {
        String channelFolder = testId + "_" + (uint8_t)rowInfoDoc["channelNumber"];
        Serial.print(F("FILE\n"));
        delay(100);
        Serial.print(testId + "/" + channelFolder + "/inputs/config.json\n");
        delay(100);
        Serial.print(rowInfo);
        Serial.print("\n");
        isRowInfoSent = true;
        return;
    }
    if (!isDriveCycleForRowSent)
    {
        // send drive cyle
        String channelFolder = testId + "_" + (uint8_t)rowInfoDoc["channelNumber"];
        Serial.print(F("FILE\n"));
        delay(100);
        Serial.print(testId + "/" + channelFolder + "/inputs/" + onNoOfRowOfCh + "_" + "driveCycle.csv\n");
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
        Serial.print(F("NULL\n"));
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
        Serial.print(F("NULL\n"));
        return;
    }

    String channelFolder = testId + "_" + (uint8_t)rowInfoDoc["channelNumber"];
    Serial.print(F("FILE\n"));
    delay(100);
    Serial.print(testId + "/" + channelFolder + "/inputs/" + onNoOfRowOfCh + "_" + "config.json\n");
    delay(100);
    Serial.print(config);
    Serial.print("\n");

    if (configDoc["mode"] == 7)
    {
        isDriveCycleForRowSent = false;
        // if it is drive cycle then you have to fetch drive cycle.
        char *driveCycleRow = net_man.getDriveCycle(rowInfoDoc["channelNumber"], onNoOfRowOfCh);
        mem_api.writeFile(driveCycleRow, "driveCycle.csv");
    }
    else
    {
        onNoOfRowOfCh += 1;
    }
}

/**
 * @brief
 *
 */
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

void ConversationAPI::recvWithStartEndMarkers()
{
    static boolean recvInProgress = false;
    static unsigned int ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;

    while (Serial.available() > 0 && newData == false)
    {
        rc = Serial.read();

        if (recvInProgress == true)
        {
            if (rc != endMarker)
            {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars)
                {
                    ndx = numChars - 1;
                }
            }
            else
            {
                receivedChars[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        }

        else if (rc == startMarker)
        {
            recvInProgress = true;
        }
    }
}
bool ConversationAPI::fillNewData(char *buffer)
{
    if (newData == true)
    {
        strcpy(buffer, receivedChars);
        newData = false;
        return false;
    }
    return false;
}