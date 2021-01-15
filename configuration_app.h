#ifndef _CONFIGURATION_APP_H_
#define _CONFIGURATION_APP_H_


class Configuration {
    private:
        uint16_t _configuration_pin;
    public:
        Configuration(uint16_t _pin);
        bool readConfiguration();
};

#endif