#include "functionPrototype.h"

/**
 * @brief Don't call it after setting interrupts on this pin
 *
 * @param delay_
 */
void blink(int delay_)
{
    // built in led pin works opposite
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    delay(delay_);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(delay_);
}