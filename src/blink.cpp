#include "functionPrototype.h"

void blink(int delay_)
{
    // built in led pin works opposite
    digitalWrite(LED_BUILTIN, LOW);
    delay(delay_);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(delay_);
}