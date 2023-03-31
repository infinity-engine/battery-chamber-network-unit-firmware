#include "MemoryAPI.h"

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

bool MemoryAPI::cleanDir()
{
    if (!file)
    {
        IS_LOG_ENABLED ? Serial.println(F("invalid file.")) : 0;
        return false;
    }
    char f_name[20];
    file = file.openNextFile(FILE_WRITE);
    while (file)
    {
        if (!file.isDirectory())
        {
            file.getName(f_name, 20);
            IS_LOG_ENABLED ? Serial.println(f_name) : 0;
            if (!sd.remove(f_name))
            {
                IS_LOG_ENABLED ? Serial.println(F("file remove failed")) : 0;
                file.close();
                return false;
            }
            else
            {
                IS_LOG_ENABLED ? Serial.println(F("file remove success")) : 0;
            }
        }
        file = file.openNextFile();
    }
    file.close();
    return true;
}