#include "InstructionsHandler.h"
#include "MemoryAPI.h"
#include "NetWorkManager.h"

InstructionsHandler::InstructionsHandler()
{
    isSetUp = false;
}
void InstructionsHandler::setup(uint8_t ch)
{
    channelNo = ch;
    fileName = "";
    fileName += channelNo;
    fileName += "_instructions.txt";
    filePosition = 0;
    prevFilePosition = 0;
    isInstructionAvailable = false;
    isSetUp = true;
}

void InstructionsHandler::reset()
{
    channelNo = 0;
    fileName = "";
    filePosition = 0;
    prevFilePosition = 0;
    isInstructionAvailable = false;
    isSetUp = false;
}

/**
 * @brief Read instruction form the memory card for experiment data and send it to cloud
 * instruction could be measuremnt, increment ch multiplier, status change
 *
 * @param nwm
 * @param mpi
 */
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
        IS_LOG_ENABLED ? Serial.println(F("Prev. ins sent failed.")) : 0;
    }
    if (mpi->file.available())
    {
        unsigned long t2 = millis();
        unsigned long t = millis();
        String ins = mpi->file.readStringUntil('\n'); // instructions; takes 1ms
        Serial.print("Absolute time ");
        Serial.println(millis());
        if (ins == "MT")
        {
            // send measurement
            char buffer[Measuremnt_JSON_Buff_Size];
            if (!mpi->readDataFromFileAndConvertToJson(buffer))
            {
                // takes atmost 1ms
                //  skip this measurement
                return;
            }
            t = millis();
            nwm->sendMeasurement(channelNo, buffer); // take at most 3ms
            Serial.print("Send time ");
            Serial.println(millis() - t);
        }
        else if (ins == "IM")
        {
            // increment multiplier
            int row = mpi->file.readStringUntil('\n').toInt();
            unsigned long t = millis();
            nwm->incrementMultiPlierIndex(channelNo, row);
            Serial.print("Send time ");
            Serial.println(millis() - t);
        }
        else if (ins == "SS")
        {
            // set status
            int row = mpi->file.readStringUntil('\n').toInt();
            uint8_t status = mpi->file.readStringUntil('\n').toInt();
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
            Serial.print("Row ");
            Serial.println(row);
            Serial.print("Status ");
            Serial.print(status);
            Serial.println(statusS);
            unsigned long t = millis();
            nwm->setStatus(statusS, channelNo, row);
            Serial.print("Send time ");
            Serial.println(millis() - t);
        }
        else
        {
            Serial.println(F("Not Matched"));
            prevFilePosition = mpi->file.position();
        }
        Serial.print(F("Time to sent req "));
        Serial.println(millis() - t2);
    }
    isInstructionAvailable = mpi->file.available();
    filePosition = mpi->file.position();
    mpi->file.close();
}

void InstructionsHandler::wrapUp(MemoryAPI *mpi)
{
    mpi->sd.chdir("/");
    mpi->sd.remove(fileName);
    reset();
}

void InstructionsHandler::writeUntil(MemoryAPI *mpi, int delay, char terminator)
{
    if (!mpi->sd.chdir("/"))
        return;

    // Serial.println(fileName);
    mpi->file = mpi->sd.open(fileName, O_WRONLY | O_APPEND);
    if (!mpi->file)
    {
        IS_LOG_ENABLED ? Serial.println(F("File Open Failed")) : 0;
        return;
    }
    unsigned long t = millis();
    while (millis() < t + delay)
    {
        while (Serial.available())
        {
            char c = Serial.read();
            if (c == terminator)
            {
                mpi->file.close();
                return;
            }
            else
            {
                mpi->file.print(c);
                t = millis();
            }
        }
    }
    mpi->file.close();
}