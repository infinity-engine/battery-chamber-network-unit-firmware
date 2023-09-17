#ifndef PROTOTYPE
#define PROTOTYPE

extern bool IS_LOG_ENABLED;
extern bool IS_LIVE_UPDATE_ENABLE;
#define SERIAL_RX_BUFFER_SIZE 256
#include <Arduino.h>
void blink(int delay_ = 100);

#define CELL_TEMP_COUNT 6

#define MAX_NO_CHANNELS 6

#define EXP_NOT_STARTED 0
#define EXP_RUNNING 1
#define EXP_STOPPED 2
#define EXP_FINISHED 3
#define EXP_PAUSED 4

#define ESP_INT_PIN 2
#define ATMEGA_INT_PIN 1

void espSoftReset();
void interruptArduino();

#endif