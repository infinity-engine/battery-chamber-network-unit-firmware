#include "InstructionsHandler.h"
#include "MemoryAPI.h"
#include "NetWorkManager.h"

InstructionsHandler::InstructionsHandler(uint8_t ch)
{
    channelNo = ch;
    fileName = "";
    fileName = channelNo + "_instructions.txt";
    filePosition = 0;
    prevFilePosition = 0;
    isInstructionAvailable = true;
}

void InstructionsHandler::handleInstruction(NetWorkManager *nwm, MemoryAPI *mpi)
{
    if (!readyToSend[channelNo])
    {
        return;
    }
    if (!mpi->sd.chdir("/"))
    {
        return;
    }
    mpi->file = mpi->sd.open(fileName);
    if (!mpi->file)
    {
        return;
    }
    if (isPrevReqSuccess[channelNo])
    {
        prevFilePosition = filePosition;
        mpi->file.seek(filePosition);
    }
    else
    {
        mpi->file.seek(prevFilePosition);
    }
    if (mpi->file.available())
    {
        String ins = mpi->file.readStringUntil('\n');
        if (ins == "MT")
        {
            // send measurement
            char buffer[Measuremnt_JSON_Buff_Size];
            if (!mpi->readDataFromFileAndConvertToJson(buffer))
            {
                // skip this measurement
                return;
            }
            nwm->sendMeasurement(channelNo, String(buffer));
        }
        else if (ins == "IM")
        {
            // increment multiplier
            int row = Serial.readStringUntil('\n').toInt();
            nwm->incrementMultiPlierIndex(channelNo, row);
        }
        else if (ins == "SS")
        {
            // increment multiplier
            int row = Serial.readStringUntil('\n').toInt();
            uint8_t status = Serial.readStringUntil('\n').toInt();
            String statusS = "";
            switch (status)
            {
            case EXP_FINISHED:
                statusS = "Completed";
                break;
            case EXP_PAUSED:
                statusS = "Paused";
                break;
            case EXP_STOPPED:
                statusS = "Stopped";
                break;
            default:
                break;
            }
            nwm->setStatus(statusS, channelNo, row);
        }
    }
    isInstructionAvailable = mpi->file.available();
    filePosition = mpi->file.position();
    mpi->file.close();
}

void InstructionsHandler::wrapUp(MemoryAPI *mpi)
{
    mpi->sd.chdir("/");
    mpi->sd.remove(fileName);
}

void InstructionsHandler::writeUntil(MemoryAPI *mpi, char terminator)
{
    if (!mpi->sd.chdir("/"))
        return;
    mpi->file = mpi->sd.open(fileName, O_WRONLY | O_CREAT);
    if (!mpi->file)
        return;
    String ins = Serial.readStringUntil(terminator);
    if (ins.length())
        mpi->file.print(ins);

    mpi->file.close();
}