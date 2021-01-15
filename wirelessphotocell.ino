#include "Arduino.h"
#include <SPI.h>
#include "printf.h"
#include "RF24.h"
#include "SevenSegmentTM1637.h"
#include "SevenSegmentExtended.h"
#include "configuration.h"
#include "pins.h"

// instantiate an object for the nRF24L01 transceiver
RF24 radio(CE_PIN, CSN_PIN); 

// initialize global TM1637 Display object
SevenSegmentExtended      display(TM1637_CLK_PIN, TM1637_DIO_PIN);

// Let these addresses be used for the pair
uint8_t address[][6] = {"1Node", "2Node"};
// It is very helpful to think of an address as a path instead of as
// an identifying device destination

// to use different addresses on a pair of radios, we need a variable to
// uniquely identify which address this radio will use to transmit
bool radioNumber = 1; // 0 uses address[0] to transmit, 1 uses address[1] to transmit

// Used to control whether this node is sending or receiving
bool role = false;  // true = TX role, false = RX role

// For this example, we'll be using a payload containing
// a single float number that will be incremented
// on every successful transmission
float payload = 0.0;

// Used for determining what operation mode use: Master or Slave
Configuration config = Configuration(OPERATION_MODE_PIN);

bool master = false; // true = master, false = slave node. Master is the starting node, and slave are partial timers


void setup()
{
    Serial.begin(115200);
    //while (!Serial) {} // some boards need to wait to ensure access to serial over USB

    display.begin();            // initializes the 7-segment display
    display.setBacklight(100); 
    
    master = config.readConfiguration();

    display.print(F("INIT"));

    // initialize the transceiver on the SPI bus
    if (!radio.begin()) 
    {
        Serial.println(F("radio hardware is not responding!!"));
        while (true) {} // hold in infinite loop
    }
    
    Serial.println(F("radio hardware initialized!!"));
    if(master)
    {
        //Config like transmiter node.
        display.clear();
        display.print(F("MSTR"));
        Serial.println(F("Configured like Master Node"));
    }  
    else
    {
        //Config like a receiver node.
        display.clear();
        display.print(F("SLV"));
        Serial.println(F("Configured like Slave Node"));
    }
    display.blink();
      
}

void loop()
{

}