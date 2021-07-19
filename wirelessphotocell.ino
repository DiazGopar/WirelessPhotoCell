#include "Arduino.h"
#include <SPI.h>
#include "printf.h"
#include "RF24.h"
#include "SevenSegmentTM1637.h"
#include "SevenSegmentExtended.h"
#include "configuration_app.h"
#include "pins_app.h"
#include "battery.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"

#define SCREEN_WIDTH    128 // OLED display width, in pixels
#define SCREEN_HEIGHT   32 // OLED display height, in pixels
#define OLED_RESET      -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS  0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

#define SECONDS_OVERFLOW    99
#define HUNDRETHSSECONDS_OVERFLOW 50

#define OLED 1
#define SEVENSEGMENT 1

#ifdef OLED
Adafruit_SSD1306 oled_display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#endif

// instantiate an object for the nRF24L01 transceiver
RF24 radio(CE_PIN, CSN_PIN); 

// initialize global TM1637 Display object
#ifdef SEVENSEGMENT
SevenSegmentExtended      display(TM1637_CLK_PIN, TM1637_DIO_PIN);
#endif

// Let these addresses be used for the pair
uint8_t address[][6] = {"1Node", "2Node"};
// It is very helpful to think of an address as a path instead of as
// an identifying device destination

// to use different addresses on a pair of radios, we need a variable to
// uniquely identify which address this radio will use to transmit
bool radioNumber = 1; // 0 uses address[0] to transmit, 1 uses address[1] to transmit

// Used to control whether this node is sending or receiving
//bool role = false;  // true = TX role, false = RX role

//Command to send the other modules
uint8_t payload = 0;

// Used for determining what operation mode use: Master or Slave
Configuration config = Configuration(OPERATION_MODE_PIN);

//Used to read the battery level
Battery battery = Battery(LIPO_LEVEL_PIN,ANALOG_ENABLE_PIN);

bool master = false; // true = master, false = slave node. Master is the starting node, and slave are partial timers

unsigned long start_time = 0; //Counter to display time in 7-segments

uint8_t seconds = 0;
uint8_t hundrethseconds = 0;
bool running = false;

const int timeThreshold = 25;
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
            digitalWrite(LED1_PIN, HIGH);
        }
        else
        {
            photoCellState = false;
            digitalWrite(LED1_PIN, LOW);   
        }
        startTime = millis();
    }
}


void setup()
{
    //Initialize Onboard LED
    pinMode(LED1_PIN,OUTPUT); //Barrier Cut ON
    digitalWrite(LED1_PIN, LOW);
    pinMode(LED2_PIN,OUTPUT); // Mode LED (ON SLAVE - OFF MASTER)
    digitalWrite(LED2_PIN, LOW);

    //Initialize output PIN
    pinMode(OUTPUT_SIGNAL_PIN, OUTPUT);
    digitalWrite(OUTPUT_SIGNAL_PIN, HIGH);

    Serial.begin(115200);
    //while (!Serial) {} // some boards need to wait to ensure access to serial over USB
    // initializes the 7-segment display
#ifdef SEVENSEGMENT
    display.begin();
    display.setPrintDelay(500);            
    display.setBacklight(100); 
    display.print(F("INIT"));
#endif
#ifdef OLED
    //Initialize LCD
    if(!oled_display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) 
    {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;); // Don't proceed, loop forever
    }
    // Show initial display buffer contents on the screen --
    // the library initializes this with an Adafruit splash screen.
    oled_display.display();
    //oled_display.clearDisplay();
    delay(2000); // Pause for 2 seconds
    // Clear the buffer
    oled_display.clearDisplay();
#endif



    //Initialize Configuration Object to determining master or slave functionality
    master = config.readConfiguration();
    //Initialize Photo Data Cell Pin
    pinMode(PHOTOCELL_DATA_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(PHOTOCELL_DATA_PIN), photoCellActivated, CHANGE);
    delay(500);
    
    //Read battery level
    Serial.println("ADC Initiating...");
    int batteryLevel = battery.readmv();
    batteryLevel = battery.readmv();
    Serial.println(battery.readmv()); 

    // initialize the transceiver on the SPI bus
    if (!radio.begin()) 
    {
        const uint8_t message_radio[] = "RADIO NOT RESPONDING!!!";
        const uint8_t message_charging[] = "BATTERY CHARGING!!!";
        Serial.println(F("radio hardware is not responding!!"));
#ifdef SEVENSEGMENT
        display.clear(); //
        display.write(message_radio,21);
        display.setBacklight(50);
        display.clear();
        display.write(message_charging,20);
        display.setBacklight(10);
#endif
#ifdef OLED
        oled_display.clearDisplay(); //
        oled_display.setTextSize(1);             // Normal 1:1 pixel scale
        oled_display.setTextColor(SSD1306_WHITE);        // Draw white text
        oled_display.setCursor(0,0); 
        oled_display.println(F("RADIO NOT RESPONDING!!!"));
        oled_display.clearDisplay();
        //oled_display.clear();
        oled_display.println(F("BATTERY CHARGING!!!"));
        oled_display.display();
#endif
        while (true) {
            //Probably the module is not Power On, Maybe in charge battery mode
            byte soc = battery.soc();
            Serial.println(battery.readmv());
            Serial.println(soc);
#ifdef SEVENSEGMENT
            display.printNumber(soc);
#endif
#ifdef OLED
            oled_display.clearDisplay(); //
            oled_display.setTextSize(4);             // Normal 1:1 pixel scale
            oled_display.setTextColor(SSD1306_WHITE);        // Draw white text
            oled_display.setCursor(0,0); 
            oled_display.print(soc);
            oled_display.display();
#endif
            delay(15000);
        } // hold in infinite loop
    }

#ifdef SEVENSEGMENT
    display.setBacklight(100);
#endif
    
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
#ifdef SEVENSEGMENT
        display.clear();
        display.print(F("MSTR"));
#endif
#ifdef OLED
        oled_display.clearDisplay(); //
        oled_display.setTextSize(4);             // Normal 1:1 pixel scale
        oled_display.setTextColor(SSD1306_WHITE);        // Draw white text
        oled_display.setCursor(0,0); 
        oled_display.print(F("MASTER"));
        oled_display.display();
#endif
        Serial.println(F("Node Configured in Master Mode"));
        digitalWrite(LED2_PIN,LOW);
        // set the TX address of the RX node into the TX pipe
        radio.openWritingPipe(address[radioNumber]);     
        // set the RX address of the TX node into a RX pipe
        radio.openReadingPipe(1, address[!radioNumber]); 
        radio.stopListening();  // put radio in TX mode
    }  
    else
    {
        //Config like a receiver node.
#ifdef SEVENSEGMENT
        display.clear();
        display.print(F("SLV"));
#endif
#ifdef OLED
        oled_display.clearDisplay(); //        
        oled_display.setTextSize(4);             // Normal 1:1 pixel scale
        oled_display.setTextColor(SSD1306_WHITE);        // Draw white text
        oled_display.setCursor(0,0); 
        oled_display.print(F("SLAVE"));
        oled_display.display();
#endif
        Serial.println(F("Node Configured in Slave Mode"));
        digitalWrite(LED2_PIN,HIGH);
        // set the TX address of the RX node into the TX pipe
        radio.openWritingPipe(address[!radioNumber]);     
        // set the RX address of the TX node into a RX pipe
        radio.openReadingPipe(1, address[radioNumber]); 
        radio.startListening(); // put radio in RX mode
    }
#ifdef SEVENSEGMENT
    display.blink(500);
#endif

    // For debugging info
    printf_begin();             // needed only once for printing details
    radio.printDetails();       // (smaller) function that prints raw register values
    radio.printPrettyDetails(); // (larger) function that prints human readable data

    //Send initial frame to all nodes
    if(master) 
    {
        delay(1000);
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
    static long pressed_time = 0;
    
    if (master) /*Master Mode*/
    {       
        if(photoCellState)
        {
            
            //if(millis() - pressed_time <= )
            payload = 0xAA;
            // This device is a TX node
            unsigned long start_timer = micros();                   // start the timer
            //digitalWrite(LED2_PIN,HIGH);                    
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
        else
        {
            //if(pressed_time > 3 segundos)
            //{
                //Send a reset 
            //}
            pressed_time = 0;
            //digitalWrite(LED1_PIN,LOW);
        }
        

        //TODO: Logic to reinitialise the whole nodes with a long signal

    } 
    else /* SLAVE Mode */
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
                //TODO: Activate the output
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

            if(photoCellState || (seconds >= SECONDS_OVERFLOW && hundrethseconds >= HUNDRETHSSECONDS_OVERFLOW)) 
            {
                running = false;
            }           
        }
#ifdef SEVENSEGMENT        
        display.printTime(seconds, hundrethseconds, true, 10); // Refresh Display time
#endif
#ifdef OLED
        oled_display.clearDisplay(); //        
        oled_display.setTextSize(4);             // Normal 1:1 pixel scale
        oled_display.setTextColor(SSD1306_WHITE);        // Draw white text
        oled_display.setCursor(0,0); 
        oled_display.print(seconds);
        oled_display.print(":");
        oled_display.print(hundrethseconds);
        oled_display.display();
#endif
    } // role
}
