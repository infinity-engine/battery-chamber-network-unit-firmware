#include "MemoryAPI.h"
#include "NetWorkManager.h"
#include "InstructionsHandler.h"
bool IS_LIVE_UPDATE_ENABLE = false;
char line[40];

//------------------------------------------------------------------------------
// Store error strings in flash to save RAM.
#define error(s) sd.errorHalt(&Serial, F(s))
//------------------------------------------------------------------------------
// Check for extra characters in field or find minus sign.
char *skipSpace(char *str)
{
    while (isspace(*str))
        str++;
    return str;
}
//------------------------------------------------------------------------------
bool parseLine(char *str)
{
    char *ptr;

    // Set strtok start of line.
    str = strtok(str, ",");
    if (!str)
        return false;

    // Print text field.
    Serial.println(str);

    // Subsequent calls to strtok expects a null pointer.
    str = strtok(nullptr, ",");
    if (!str)
        return false;

    // Convert string to long integer.
    int32_t i32 = strtol(str, &ptr, 0);
    if (str == ptr || *skipSpace(ptr))
        return false;
    Serial.println(i32);

    str = strtok(nullptr, ",");
    if (!str)
        return false;

    // strtoul accepts a leading minus with unexpected results.
    if (*skipSpace(str) == '-')
        return false;

    // Convert string to unsigned long integer.
    uint32_t u32 = strtoul(str, &ptr, 0);
    if (str == ptr || *skipSpace(ptr))
        return false;
    Serial.println(u32);

    str = strtok(nullptr, ",");
    if (!str)
        return false;

    // Convert string to double.
    double d = strtod(str, &ptr);
    if (str == ptr || *skipSpace(ptr))
        return false;
    Serial.println(d);

    // Check for extra fields.
    return strtok(nullptr, ",") == nullptr;
}
//------------------------------------------------------------------------------
MemoryAPI::MemoryAPI() {}

void MemoryAPI::setup()
{
    while (!sd.begin(SD_CONFIG))
    {
        IS_LOG_ENABLED ? Serial.println(F("SD init failed.")) : 0;
        delay(1000);
        IS_LOG_ENABLED ? Serial.println(F("Trying to re-init.")) : 0;
        delay(1000);
    }
    IS_LOG_ENABLED ? Serial.println(F("SD init success.")) : 0;
    if (IS_LOG_ENABLED)
    {
        Serial.println(F("--SD LS OUTPUT--"));
        sd.ls("/", LS_R);
        Serial.println(F("--END--"));
    }
    overallStatus = "";
    isContinueReadingInsSerial = false;
    for (uint8_t i = 0; i < MAX_NO_CHANNELS; i++)
    {
        irhArray[i].reset();
    }
}

void MemoryAPI::errorPrint()
{
    if (sd.sdErrorCode())
    {
        cout << F("SD errorCode: ") << hex << showbase;
        printSdErrorSymbol(&Serial, sd.sdErrorCode());
        cout << F(" = ") << int(sd.sdErrorCode()) << endl;
        cout << F("SD errorData = ") << int(sd.sdErrorData()) << dec << endl;
    }
}

void MemoryAPI::printCardType()
{
    if (!sd.card()->readCID(&cid) ||
        !sd.card()->readCSD(&csd) ||
        !sd.card()->readOCR(&ocr) ||
        !sd.card()->readSCR(&scr))
    {
        cout << F("readInfo failed\n");
        errorPrint();
        return;
    }
    cout << F("\nCard type: ");

    switch (sd.card()->type())
    {
    case SD_CARD_TYPE_SD1:
        cout << F("SD1\n");
        break;

    case SD_CARD_TYPE_SD2:
        cout << F("SD2\n");
        break;

    case SD_CARD_TYPE_SDHC:
        if (csd.capacity() < 70000000)
        {
            cout << F("SDHC\n");
        }
        else
        {
            cout << F("SDXC\n");
        }
        break;

    default:
        cout << F("Unknown\n");
    }
    cout << F("sdSpecVer: ") << 0.01 * scr.sdSpecVer() << endl;
    cout << F("HighSpeedMode: ");
    if (scr.sdSpecVer() &&
        sd.card()->cardCMD6(0X00FFFFFF, cmd6Data) && (2 & cmd6Data[13]))
    {
        cout << F("true\n");
    }
    else
    {
        cout << F("false\n");
    }
}

/**
 * Writes the given content to a file with the specified file path.
 *
 * @param fileContent The content to be written to the file.
 * @param filePath The file path to write the content to.
 * @return `true` if the content was successfully written to the file, `false` otherwise.
 */
bool MemoryAPI::writeFile(const char *fileContent, const char *filePath)
{
    if (!sd.chdir("/"))
    {
        IS_LOG_ENABLED ? Serial.println(F("chdir failed!")) : 0;
        return false;
    }

    if (sd.exists(filePath))
    {
        if (!sd.remove(filePath))
        {
            IS_LOG_ENABLED ? Serial.println(F("remove failed!")) : 0;
            file.close();
            return false;
        }
    }

    file = sd.open(filePath, O_WRONLY | O_CREAT);
    if (!file)
    {
        IS_LOG_ENABLED ? Serial.println(F("file open failed!")) : 0;
        file.close();
        return false;
    }
    file.print(fileContent);
    file.close();

    return true;
}

/**
 * @brief Read the file content into Serial
 *
 * @param filePath
 */
void MemoryAPI::readFile(const char *filePath)
{
    if (!sd.chdir("/") || !sd.exists(filePath))
    {
        Serial.println(F("Something wrong with chdir or file doesn't exist."));
        return;
    }
    file = sd.open(filePath);
    if (file)
    {
        Serial.println(F("------Start-------"));
        while (file.available())
        {
            Serial.write(file.read());
        }
        Serial.println(F("\n------End-------"));
        file.close();
    }
    else
    {
        Serial.println(F("Could not open file."));
    }
}

/**
 * @brief Read a file from the SD card and extract a substring of characters between
 *        two given keys.
 *
 * @param filePath The path of the file to read.
 * @param startKey The starting key for the substring.
 * @param endKey The ending key for the substring.
 * @return A String object containing the substring, or an empty string if either the
 *         file could not be opened or the keys were not found.
 */
String MemoryAPI::valueInBetween(const char *filePath, const char *startKey, const char *endKey, u_int64_t *position)
{
    String value = "";
    // Check if the file exists
    if (!sd.exists(filePath))
    {
        IS_LOG_ENABLED ? Serial.println(F("File not exists.")) : 0;
        return value;
    }

    // Open the file
    file = sd.open(filePath, FILE_READ);
    if (!file)
    {
        IS_LOG_ENABLED ? Serial.println(F("File open failed.")) : 0;
        file.close();
        return value;
    }
    if (position != NULL)
    {
        file.seek(*position);
    }

    if (!findKeyInFile(startKey) || !readFileUntil(value, endKey))
    {
        value = "";
    }
    if (position != NULL)
    {
        *position = file.position() - strlen(endKey);
    }
    file.close();
    return value;
}

/**
 * @brief Check if a key exists in a file and seek the file pointer after the key.
 *
 * @param key The key to search for.
 * @return true if the key is found, false otherwise.
 */
bool MemoryAPI::findKeyInFile(const char *key)
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

/**
 * @brief Read a file from the SD card and add each character to a String variable
 *        until the given key is found in the file.
 *
 * @param str The String object to append the characters to.
 * @param key The key to search for.
 * @return true if the key is found, false otherwise.
 */
bool MemoryAPI::readFileUntil(String &str, const char *key)
{
    while (file.available())
    {
        char c = file.read();
        if (c == *key)
        {
            size_t i;
            for (i = 1; i < strlen(key); i++)
            {
                if (file.read() != key[i])
                {
                    break;
                }
            }
            if (i == strlen(key))
            {
                // Found the key, return true
                return true;
            }
            else
            {
                // The characters read didn't match the key, add them to the string
                str += c;
                file.seek(file.position() - i); // Move the file pointer back to the next character
            }
        }
        else
        {
            // The current character is not part of the key, add it to the string
            str += c;
        }
    }
    return false; // The key was not found in the file
}

bool MemoryAPI::cleanDir(String path)
{
    FatFile cwd;
    const char *path_c = path.c_str();
    if (!cwd.open(path_c))
    {
        IS_LOG_ENABLED ? Serial.println(F("CWD path open failed.")) : 0;
        return false;
    }
    if (!cwd.rmRfStar())
    {
        IS_LOG_ENABLED ? Serial.println(F("O/P dir exist. Remove failed.")) : 0;
        return false;
    }
    IS_LOG_ENABLED ? Serial.println(F("Cleaned o/p dir.")) : 0;
    cwd.close();
    return true;
}

int MemoryAPI::bytesAvailable(Stream *stream)
{
    int bytesAvailable = stream->available();

    if (bytesAvailable >= SERIAL_RX_BUFFER_SIZE - 5)
    {
        Serial.println(F("Warning: incoming buffer is almost full!"));
    }
    if (bytesAvailable == SERIAL_RX_BUFFER_SIZE)
    {
        Serial.println(F("Error: incoming buffer overflow!"));
    }
    return bytesAvailable;
}

bool MemoryAPI::writeToStream(const char *path, Stream *stream, int d)
{
    if (!sd.chdir("/"))
    {
        return false;
    }
    file = sd.open(path);
    if (!file)
    {
        file.close();
        return false;
    }
    while (file.available())
    {
        stream->write(file.read());
        delay(d);
    }
    file.close();
    return true;
}

/**
 * @brief Reads data from file and converts it into a JSON string.
 *
 * @param buffer Pointer to a character array to store the JSON string.
 * @return true If data was read and converted successfully.
 * @return false If there was an error while reading or converting the data.
 */
bool MemoryAPI::readDataFromFileAndConvertToJson(char *buffer)
{
    char line[LINE_LEN]; // Buffer to store a line from the file
    line[0] = '\0';
    // Check if there is data available in the file
    if (file.available())
    {
        // Read a line from the file
        int bytesRead = file.readBytesUntil('\n', line, LINE_LEN);
        line[bytesRead] = '\0'; // Add a null terminator to the end of the line

        // Skip empty lines or comments
        if (line[0] == '\0' || line[0] == '#')
        {
            return false;
        }
        char zero[] = "";
        char *time = strtok(line, ",");        // Time
        char *voltageStr = strtok(NULL, ",");  // Voltage as string
        char *currentStr = strtok(NULL, ",");  // Current as string
        char *chamTempStr = strtok(NULL, ","); // Chamber temperature as string
        if (strcmp("NAN", chamTempStr) == 0)
        {
            chamTempStr = zero;
        }
        char *chamHumStr = strtok(NULL, ","); // Chamber humidity as string
        if (strcmp("NAN", chamHumStr) == 0)
        {
            chamHumStr = zero;
        }
        char *cellTempStr[CELL_TEMP_COUNT]; // Array to store cell temperatures as strings
        int cellTempCount = 0;              // Number of cell temperatures found in the line
        for (char *cellTemp = strtok(NULL, ","); cellTemp != NULL; cellTemp = strtok(NULL, ","))
        {
            if (strcmp("NAN", cellTemp) == 0)
            {
                cellTemp = zero;
            }
            cellTempStr[cellTempCount++] = cellTemp;
        }

        // Convert the values to JSON string
        // Start with chamber temperature and humidity
        sprintf(buffer, "{\"chamberTemp\":[%s],\"chamberHum\":[%s],\"cellTemp\":[", chamTempStr, chamHumStr);

        // Add cell temperatures
        for (int i = 0; i < cellTempCount; i++)
        {
            // Each cell temperature is represented as a JSON object with "sensorId" and "values" properties
            sprintf(buffer + strlen(buffer), "{\"sensorId\":%d,\"values\":[%s]}", i + 1, cellTempStr[i]);
            if (i < cellTempCount - 1)
            {
                strcat(buffer, ","); // Add comma between cell temperature objects
            }
        }

        // Add remaining values: current, voltage, and time
        sprintf(buffer + strlen(buffer), "],\"current\":[%s],\"voltage\":[%s],\"time\":[%s]}", currentStr, voltageStr, time);
    }
    return true;
}

void MemoryAPI::readAndSendInstruction(NetWorkManager &nwm)
{
    for (uint8_t i = 0; i < MAX_NO_CHANNELS; i++)
    {
        readInstructions();
        if (!irhArray[i].isSetUp)
            continue;
        if (IS_LIVE_UPDATE_ENABLE)
            irhArray[i].handleInstruction(&nwm, this); // uncomment it to start live update
        readInstructions();
    }
}

/**
 * @brief read whatever instruction for experiment is send after the interrput is triggered and write it to the memory through serial
 * |<channelNo>
 * <instructions>
 * |
 * |<channelNo>
 * <instructions>
 * |
 * |...
 *
 * |END
 * <status code>
 * |
 *
 */
void MemoryAPI::readInstructions()
{
    // if (!isInstructionAvailable)
    //     return;
    // Serial.println(millis());
    while (bytesAvailable(&Serial))
    {
        bool flag = true;
        while (true)
        {
            if (bytesAvailable(&Serial))
            {
                char c = Serial.read();
                if (c == '<')
                    break;
                else if (flag)
                {
                    Serial.print(F("miss "));
                    Serial.print(c);
                    Serial.print("->");
                    Serial.println((int)c);
                    flag = false;
                }
            }
        }
        // wait for atleast 20ms
        unsigned long t = millis();
        uint8_t d = 20;
        while (millis() < t + d)
        {
            if (Serial.available())
            {
                break;
            }
        }
        if (!Serial.available())
        {
            return;
        }
        String msg = Serial.readStringUntil('\n');
        if (msg == "END")
        {
            uint8_t status = Serial.readStringUntil('\n').toInt();
            overallStatus = "";
            switch (status)
            {
            case EXP_FINISHED:
                overallStatus = "Completed";
                break;
            case EXP_PAUSED:
                overallStatus = "Paused";
                break;
            case EXP_STOPPED:
                overallStatus = "Stopped";
                break;
            default:
                break;
            }
            isContinueReadingInsSerial = false;
            Serial.print(F("Rec. End with status code "));
            Serial.println(status);
            clearIncomingBuffer();
            return;
        }
        else
        {
            uint8_t channel = msg.toInt();
            if (channel)
            {
                Serial.print("Ch ");
                Serial.println(channel);
                writeInstructions(&irhArray[channel - 1]); // takes around 12ms
            }
        }
    }
}

void IRAM_ATTR MemoryAPI::raiseFlagForInt()
{
    isInstructionAvailable = true;
}

void MemoryAPI::writeInstructions(InstructionsHandler *irh)
{
    if (irh->isSetUp)
        irh->writeUntil(this);
    else
        IS_LOG_ENABLED ? Serial.println("Not Set Up") : 0;
}

void MemoryAPI::wrapup(NetWorkManager *nwm)
{
    if (!IS_LIVE_UPDATE_ENABLE)
    {
        // now start sending data
        while (true)
        {
            nwm->setStatus("Running");
            if (isPrevReqSuccess[0])
            {
                break;
            }
            delay(1000);
        }
    }
    bool isContinue = true;
    while (isContinue)
    {
        isContinue = false;
        for (uint8_t i = 0; i < MAX_NO_CHANNELS; i++)
        {
            if (irhArray[i].isSetUp)
            {
                irhArray[i].handleInstruction(nwm, this);
                if (irhArray[i].isInstructionAvailable)
                    isContinue = true;
                else
                    ; // irhArray[i].wrapUp(this);
            }
        }
        delay(5); // very very important step
    }
    nwm->setStatus(overallStatus);
}

void MemoryAPI::beginInterrupt()
{
    pinMode(ESP_INT_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(ESP_INT_PIN), raiseFlagForInt, RISING);
}

void MemoryAPI::endInterrupt()
{
    detachInterrupt(digitalPinToInterrupt(ESP_INT_PIN));
}

void MemoryAPI::clearIncomingBuffer()
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

bool MemoryAPI::isInstructionAvailable;