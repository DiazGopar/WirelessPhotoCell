#include "configuration_app.h"
#include "assert.h"

Configuration::Configuration(uint8_t pin)
{
    assert(pin != 0);
    _configuration_pin = pin;
    pinMode(pin, INPUT_PULLUP);
}

bool Configuration::readConfiguration()
{
    return (digitalRead(_configuration_pin) == 0);
}
