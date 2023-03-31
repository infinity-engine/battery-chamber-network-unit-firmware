#ifndef MemoryAPI_H
#define MemoryAPI_H

#include <SdFat.h>
#include <sdios.h>
#include <MemoryFree.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include "functionPrototype.h"

#ifndef SDCARD_SS_PIN
const uint8_t SD_CS_PIN = SS;
#else  // SDCARD_SS_PIN
const uint8_t SD_CS_PIN = SDCARD_SS_PIN;
#endif // SDCARD_SS_PIN

// Try to select the best SD card configuration.
#if HAS_SDIO_CLASS
#define SD_CONFIG SdioConfig(FIFO_SDIO)
#elif ENABLE_DEDICATED_SPI
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(16))
#else // HAS_SDIO_CLASS
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(16))
#endif // HAS_SDIO_CLASS

static ArduinoOutStream cout(Serial);

class MemoryAPI
{
public:
    MemoryAPI();
    bool cleanDir();
    void setup();
    void printCardType();
    void errorPrint();
    SdFs sd;
    FsFile file;
    cid_t cid;
    csd_t csd;
    scr_t scr;
    uint8_t cmd6Data[64];
    uint32_t eraseSize;
    uint32_t ocr;
};

#endif