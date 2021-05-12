#ifndef _BATTERY_H_
#define _BATTERY_H_

#include "Arduino.h"

class Battery{
    private:
        int voltage_mv;
        int analog_raw;
        uint8_t battery_pin;
        uint8_t battery_enable_pin;

        void activateReading();
        int readRaw();
        //int readmv();

    public:
        Battery(uint8_t,uint8_t);
        byte soc();
        int readmv();
};

//int soc_values = {4000,3800,3700,3650,3600,3550,3500,3400,3300,3200};

#endif