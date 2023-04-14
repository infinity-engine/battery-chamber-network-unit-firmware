#ifndef INS_H
#define INS_H
#include <Arduino.h>

class MemoryAPI;
class NetWorkManager;

class InstructionsHandler
{
private:
    uint8_t channelNo;
    String fileName;
    uint32_t filePosition;
    uint32_t prevFilePosition;

public:
    InstructionsHandler(uint8_t channelNo);
    void handleInstruction(NetWorkManager *nwm, MemoryAPI *mpi);
    void wrapUp(MemoryAPI *mpi);
    void writeUntil(MemoryAPI *mpi, char terminator = '|');

    bool isInstructionAvailable;
};

#endif