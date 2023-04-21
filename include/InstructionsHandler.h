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
    InstructionsHandler();
    void handleInstruction(NetWorkManager *nwm, MemoryAPI *mpi);
    void wrapUp(MemoryAPI *mpi);
    void writeUntil(MemoryAPI *mpi, int delay = 1000, char terminator = '>');
    bool isInstructionAvailable;
    void reset();
    bool isSetUp;
    void setup(uint8_t ch);
};

#endif