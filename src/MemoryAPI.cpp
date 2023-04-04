#include "MemoryAPI.h"
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
        IS_LOG_ENABLED ? Serial.println(F("SD initialization failed.")) : 0;
        delay(1000);
        IS_LOG_ENABLED ? Serial.println(F("Trying to reinitialize.")) : 0;
        delay(1000);
    }
    IS_LOG_ENABLED ? Serial.println(F("SD initialization success.")) : 0;

    IS_LOG_ENABLED ? sd.ls("/", LS_R) : 0;
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
            return false;
        }
    }

    file = sd.open(filePath, O_WRONLY | O_CREAT);
    if (!file)
    {
        IS_LOG_ENABLED ? Serial.println(F("file open failed!")) : 0;
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