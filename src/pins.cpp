#include <Arduino.h>
#include <pins.h>
#include <config.h>

void pins_init(void)
{
    pinMode(PIN_RESET, INPUT_PULLUP);
}

void pins_set_clear_interrupt(void (*f)(void))
{
    attachInterrupt(PIN_RESET, f, RISING);
}