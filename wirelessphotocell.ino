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
//bool role = false;  // true = TX role, false = RX role

// For this example, we'll be using a payload containing
// a single float number that will be incremented
// on every successful transmission
uint8_t payload = 0;

// Used for determining what operation mode use: Master or Slave
Configuration config = Configuration(OPERATION_MODE_PIN);

bool master = false; // true = master, false = slave node. Master is the starting node, and slave are partial timers

unsigned long start_time = 0; //Counter to display time in 7-segments

uint8_t seconds = 0;
uint8_t hundrethseconds = 0;
bool running = false;

const int timeThreshold = 50;
long startTime = 0;
bool photoCellState = false;

//ISR function to detect photoCell cut
void photoCellActivated() 
{
    if (millis() - startTime > timeThreshold)
    {
        if(digitalRead(PHOTOCELL_DATA_PIN) == 1)
        {
            photoCellState = true;
        }
        else
        {
            photoCellState = false;   
        }
        startTime = millis();
    }
}


void setup()
{
    Serial.begin(115200);
    //while (!Serial) {} // some boards need to wait to ensure access to serial over USB
    
    // initializes the 7-segment display
    display.begin();            
    display.setBacklight(100); 
    display.print(F("INIT"));
    //Initialize Configuration Object to determining master or slave functionality
    master = config.readConfiguration();
    //Initialize Photo Data Cell Pin
    pinMode(PHOTOCELL_DATA_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(PHOTOCELL_DATA_PIN), photoCellActivated, CHANGE);
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

    // Min speed for better range
    radio.setDataRate(RF24_250KBPS);
    // save on transmission time by setting the radio to only transmit the
    // number of bytes we need 
    radio.setPayloadSize(sizeof(payload)); // float datatype occupies 4 bytes

    
    Serial.println(F("radio hardware initialized!!"));
    if(master)
    {
        //Config like transmiter node.
        display.clear();
        display.print(F("MSTR"));
        Serial.println(F("Node Configured in Master Mode"));
        // set the TX address of the RX node into the TX pipe
        radio.openWritingPipe(address[radioNumber]);     // always uses pipe 0
        // set the RX address of the TX node into a RX pipe
        radio.openReadingPipe(1, address[!radioNumber]); // using pipe 1
        radio.stopListening();  // put radio in TX mode
    }  
    else
    {
        //Config like a receiver node.
        display.clear();
        display.print(F("SLV"));
        Serial.println(F("Node Configured in Slave Mode"));
        // set the TX address of the RX node into the TX pipe
        radio.openWritingPipe(address[!radioNumber]);     // always uses pipe 0
        // set the RX address of the TX node into a RX pipe
        radio.openReadingPipe(1, address[radioNumber]); // using pipe 1
        radio.startListening(); // put radio in RX mode
    }
    display.blink(500);

    // For debugging info
    printf_begin();             // needed only once for printing details
    radio.printDetails();       // (smaller) function that prints raw register values
    radio.printPrettyDetails(); // (larger) function that prints human readable data

    //Send initial frame to all nodes
    if(master) 
    {
        delay(2000);
        payload = 0x33;
        bool report = radio.write(&payload, sizeof(payload));
        if (!report) 
        {
            Serial.println(F("Transmission failed or timed out")); // payload was not delivered
        }
    }
      
}

void loop()
{

    if (master) {
        
        if(digitalRead(PHOTOCELL_DATA_PIN))
        {
            payload = 0xAA;
            // This device is a TX node
            unsigned long start_timer = micros();                    // start the timer
            bool report = radio.write(&payload, sizeof(payload));      // transmit & save the report
            unsigned long end_timer = micros();                      // end the timer

            if (report) 
            {
                Serial.print(F("Transmission successful! "));          // payload was delivered
                Serial.print(F("Time to transmit = "));
                Serial.print(end_timer - start_timer);                 // print the timer result
                Serial.print(F(" us. Sent: "));
                Serial.println(payload);                               // print payload sent
            } 
            else 
            {
                Serial.println(F("Transmission failed or timed out")); // payload was not delivered
            }
        }

    } 
    else 
    {   // This device is a RX node                    
        uint8_t pipe;
        if (radio.available(&pipe)) 
        {             // is there a payload? get the pipe number that recieved it
            uint8_t bytes = radio.getPayloadSize(); // get the size of the payload
            radio.read(&payload, bytes);            // fetch payload from FIFO
            
            if(payload == 0xAA && !running)
            {
                //Frame from master Node Start chrono time from zero.
                start_time = millis(); //the beggining time to use to calculate chrono running time
                seconds = 0;
                hundrethseconds = 0;
                running = true;
            }
            else if (payload == 0x33)
            {
                //Frame from master Node to reset the counter and put in initial Mode
                running = false;
                seconds = 0;
                hundrethseconds = 0;
            } 
            else if(payload == 0xDB)
            {
                //Frame from Slave Node, keep incresing time, and send signal to output device

            }       
            Serial.print(F("Received "));
            Serial.print(bytes);                    // print the size of the payload
            Serial.print(F(" bytes on pipe "));
            Serial.print(pipe);                     // print the pipe number
            Serial.print(F(": "));
            Serial.println(payload);                // print the payload's value
        }

        if(running)
        {
            unsigned long current_time = millis();
            Serial.println(current_time);
            seconds = (current_time - start_time) / 1000;
            hundrethseconds = (uint8_t)(((current_time - start_time) % 1000) / 10);

            if(photoCellState) {
                running = false;
            }           
        }
        
        display.printTime(seconds, hundrethseconds, true, 10); // Refresh Display time
    } // role
}