#include "Arduino.h"
#include <SPI.h>
#include "printf.h"
#include "RF24.h"
#include "SevenSegmentTM1637.h"
#include "SevenSegmentExtended.h"
#include "configuration_app.h"
#include "pins_app.h"

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
    display.print(F("INIT"));
    master = config.readConfiguration();
    delay(1000);
    
    // initialize the transceiver on the SPI bus
    if (!radio.begin()) 
    {
        Serial.println(F("radio hardware is not responding!!"));
        display.clear(); //
        display.print(F("ERROR"));
        display.blink(500,30);
        while (true) {} // hold in infinite loop
    }
    // Set the PA Level low 
    radio.setPALevel(RF24_PA_MAX);  // RF24_PA_MAX is default.
    // save on transmission time by setting the radio to only transmit the
    // number of bytes we need 
    radio.setPayloadSize(sizeof(payload)); // float datatype occupies 4 bytes
    // set the TX address of the RX node into the TX pipe
    radio.openWritingPipe(address[radioNumber]);     // always uses pipe 0
    // set the RX address of the TX node into a RX pipe
    radio.openReadingPipe(1, address[!radioNumber]); // using pipe 1
    
    Serial.println(F("radio hardware initialized!!"));
    if(master)
    {
        //Config like transmiter node.
        display.clear();
        display.print(F("MSTR"));
        Serial.println(F("Node Configured in Master Mode"));
        radio.stopListening();  // put radio in TX mode
    }  
    else
    {
        //Config like a receiver node.
        display.clear();
        display.print(F("SLV"));
        Serial.println(F("Node Configured in Slave Mode"));
        radio.startListening(); // put radio in RX mode
    }
    display.blink(500);

    // For debugging info
    printf_begin();             // needed only once for printing details
    radio.printDetails();       // (smaller) function that prints raw register values
    radio.printPrettyDetails(); // (larger) function that prints human readable data
      
}

void loop()
{

}