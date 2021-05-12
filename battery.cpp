#include "battery.h"



Battery::Battery(uint8_t battery_analog_pin, uint8_t battery_enable_pin)
{
    this->battery_pin = battery_pin;
    this->battery_enable_pin = battery_enable_pin;

    pinMode(this->battery_enable_pin, OUTPUT);
    digitalWrite(this->battery_enable_pin, LOW);
    analogReference(DEFAULT);
    this->voltage_mv = 0;
}

int Battery::readmv()
{    
    //WORKAROUND for reading battery, onlyu one read dont work
    for(uint8_t i=0; i < 3; i++)
    {
        this->voltage_mv = this->readRaw() * 1.93 /*manual correction factor*/ * (double)(5000.0 / 1024); 
    }
    return this->voltage_mv;
}

int Battery::readRaw()
{  
    this->analog_raw = analogRead(this->battery_pin);
    digitalWrite(this->battery_enable_pin, LOW);
    this->analog_raw = analogRead(this->battery_pin);
    delayMicroseconds(100);
    digitalWrite(this->battery_enable_pin, HIGH);
    return this->analog_raw;
}

void Battery::activateReading()
{
    digitalWrite(this->battery_enable_pin, LOW);
    delayMicroseconds(10);
    digitalWrite(this->battery_enable_pin, HIGH);
}

byte Battery::soc()
{
    int batterymv = this->readmv();
    byte soc = (0.1255*batterymv - 403.01);
    //return (byte)-(((batterymv*batterymv*batterymv)*(double)0.0000004) + (batterymv*batterymv*(double)0.004)- (double)14.29*batterymv + 16664);
    soc = soc > 100 ? 100 : soc;
    return soc;
}