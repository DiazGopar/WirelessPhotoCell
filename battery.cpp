#include "battery.h"
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


Battery::Battery(uint8_t battery_pin, uint8_t battery_enable_pin)
{
    this->battery_pin = battery_pin;
    this->battery_enable_pin;

    pinMode(battery_enable_pin, OUTPUT);
    digitalWrite(battery_enable_pin, LOW);
    analogReference(DEFAULT);
    this->voltage_mv = 0;
}

int Battery::readmv()
{    
    int voltage_mv = 0;
    this->voltage_mv = this->readRaw() * 2 * (5 / 1024);  
    return this->voltage_mv;
}

int Battery::readRaw()
{
    this->activateReading();
    delayMicroseconds(100);
    this->analog_raw = analogRead(battery_pin);
    return this->analog_raw;
}

void Battery::activateReading()
{
    digitalWrite(battery_enable_pin, HIGH);
    delayMicroseconds(1);
    digitalWrite(battery_enable_pin, LOW);
}