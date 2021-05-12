#ifndef _CONFIGURATION_APP_H_
#define _CONFIGURATION_APP_H_

#include "Arduino.h"
class Configuration {
    private:
        uint8_t _configuration_pin;
    public:
        Configuration(uint8_t _pin);
        bool readConfiguration();
};

#endif
