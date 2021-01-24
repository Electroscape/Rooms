
/*==========================================================================================================*/
/*		2CP - TeamEscape - Engineering
 *		by Martin Pek & Abdullah Saei & Robert Schloezer
 *
 *		v1.51s
 *		- Last Changes 08.07.2019
 *		- Add WDT
 *		- Bug fixing undefined state z = 5

        differences to hamburg, additional LED stripe, different ledcounts
        Keypad as input to mother control pneumatic Door
        Keypad used to reset the room and close the door with keycode 20162016 and longer opening singal
 */
/*==========================================================================================================*/

// const String title = String("REINRAUMSTEUERUNG v1.1");

const String title = "reinraumsteuerung";
const String versionDate = "15.10.2019";
const String version = "version 1.51s";

#define DEBUG_MODE 					1

/*==INCLUDE=================================================================================================*/
//Watchdog timer
#include <avr/wdt.h>
// I2C Port Expander
#include "PCF8574.h"

// WS2812B LED Strip
#include <FastLED.h>

/*==DEFINE==================================================================================================*/
// LED
// PIN
#define PWM_1_PIN 3                       // Predefined by STB design
#define PWM_2_PIN 5                       // Predefined by STB design
#define PWM_3_PIN 6                       // Predefined by STB design
#define PWM_4_PIN 9                       // Predefined by STB design
// SETTINGS
#define LED_STRIP WS2812B                                                           // Type of LED Strip

// I2C ADRESSES
#define RELAY_I2C_ADD     	0x3F         // Relay Expander
#define INPUT_I2C_ADD       0x38 //        // door opening detector Expander
#define OLED_I2C_ADD        0x3C         // Predefined by hardware
#define LCD_I2C_ADD					 0x27 // Predefined by hardware


// Relay Pin configuration
enum relayPins {
    REL_ALARM_PIN,                   // three red lights on the wall -> blinks when procedure starts
    REL_FAN_OUT_BIG_PIN,            // big, yellow fan -> fog out
    REL_FAN_OUT_SMALL_PIN,          // small fan -> fog out
    REL_FAN_IN_SMALL_PIN,           // small fan -> fresh air in
    REL_FOG_PIN,                    // fog machine
    REL_KEYPAD_PIN,                 // keypad Powersupply, we may turn it on or off
    REL_VALVE_PIN,                  // magnet at the clean room door (needs to be on during the procedure, so the door is closed)
    REL_ENTRY_ALARM_PIN             // alarm light at the room entrance
};

/*==CONSTANT VARIABLES======================================================================================*/
const enum relayPins relayPinArray[] = {
    REL_ALARM_PIN,
    REL_FAN_OUT_BIG_PIN,
    REL_FAN_OUT_SMALL_PIN,
    REL_FAN_IN_SMALL_PIN,
    REL_FOG_PIN,
    REL_KEYPAD_PIN,
    REL_VALVE_PIN,
    REL_ENTRY_ALARM_PIN
};

#define REL_AMOUNT              8

// initstates
#define REL_ALARM_INIT           1        // DESCRIPTION OF THE RELAY WIRING
#define REL_FAN_OUT_BIG_INIT     1        // DESCRIPTION OF THE RELAY WIRING
#define REL_FAN_OUT_SMALL_INIT   1        // DESCRIPTION OF THE RELAY WIRING
#define REL_FAN_IN_SMALL_INIT    1        // DESCRIPTION OF THE RELAY WIRING
#define REL_FOG_INIT             1        // DESCRIPTION OF THE RELAY WIRING
#define REL_KEYPAD_INIT          1        // DESCRIPTION OF THE RELAY WIRING
#define REL_VALVE_INIT           1        // DESCRIPTION OF THE RELAY WIRING
#define REL_ENTRY_ALARM_INIT     1        // DESCRIPTION OF THE RELAY WIRING

const byte relayInitArray[] = {
    REL_ALARM_INIT,
    REL_FAN_OUT_BIG_INIT,
    REL_FAN_OUT_SMALL_INIT,
    REL_FAN_IN_SMALL_INIT,
    REL_FOG_INIT,
    REL_KEYPAD_INIT,
    REL_VALVE_INIT,
    REL_ENTRY_ALARM_INIT
};

// inputs
enum inputPin {
  REED_PIN,                             //  0 alarm light at the entrance/exit
  KEYPAD_INPUT_PIN,                             //  1 trigger for starting the procedure
};

const enum inputPin inputPinArray[] = {
    REED_PIN,
    KEYPAD_INPUT_PIN,
};


/*============================================================================================================
//===LED======================================================================================================
//==========================================================================================================*/

#define LED_STRIP WS2812B                                                           // Type of LED Strip
#define NUM_LEDS_FR 65
#define NUM_LEDS_CYL 18

CRGB leds_fr[NUM_LEDS_FR];
CRGB leds_cyl[NUM_LEDS_CYL];


/*==FLAGS===================================================================================================*/
bool firstRun     = false;
bool entranceOpen = false;
long timenow = 0;
/*==PCF8574=================================================================================================*/
Expander_PCF8574 relay;

/*============================================================================================================
//===DECONTAMINATION==========================================================================================
//==========================================================================================================*/


bool decontamination( bool cleanroomLocked ) {

    if( cleanroomLocked ) {
        relay.digitalWrite(REL_VALVE_PIN, !REL_VALVE_INIT);
        Serial.println("Clean room: locked");
        delay(500);
        relay.digitalWrite(REL_KEYPAD_PIN, !REL_KEYPAD_INIT);
        Serial.println("Clean room: Keypad off");
        delay(2500);
        wdt_reset();
    }

    for(int i = 0; i < NUM_LEDS_FR; i++) {
        leds_fr[i] = CHSV(0,255,0);              // LED "off"
    }

    FastLED.show();

    wdt_reset();
    relay.digitalWrite(REL_ALARM_PIN, !REL_ALARM_INIT); Serial.println("Light: off");
    delay(1000);
    relay.digitalWrite(REL_FAN_OUT_SMALL_PIN, !REL_FAN_OUT_SMALL_INIT); Serial.println("Fan OUT small: on");
    relay.digitalWrite(REL_FAN_IN_SMALL_PIN, !REL_FAN_IN_SMALL_INIT); Serial.println("Fan IN small: on");
    delay(4000);

    // Fog and blinking lights
    for (int z=0; z<30; z++) {

        Serial.print("z: "); Serial.println(z);
        if (z==0) {relay.digitalWrite(REL_FOG_PIN, !REL_FOG_INIT); Serial.println("Fog: on");}
        else if (z==5) {relay.digitalWrite(REL_FOG_PIN, REL_FOG_INIT); Serial.println("Fog: off");}
        else if (z==6) {relay.digitalWrite(REL_FAN_OUT_BIG_PIN, !REL_FAN_OUT_BIG_INIT); Serial.println("Fan OUT big: on");}
        else{}//Serial.print("ez: "); Serial.println(z);}

        for(int i = 0; i < kMatrixWidth; i++) {
            for(int j = 0; j < kMatrixHeight; j++) {
                leds[XY(i,j)] = CHSV(170,95,255);
            }
        }

        FastLED.show();
        relay.digitalWrite(REL_ALARM_PIN, !REL_ALARM_INIT);
        wdt_reset();
        delay(1000);

        for(int i = 0; i < kMatrixWidth; i++) {
            for(int j = 0; j < kMatrixHeight; j++) {
                leds[XY(i,j)] = CHSV(0,255,255);
            }
        }

        FastLED.show();

        relay.digitalWrite(REL_ALARM_PIN, REL_ALARM_INIT);
        wdt_reset();
        delay(1000);
    }

    // Green light -----------------------------------------------
    for(int i = 0; i < kMatrixWidth; i++) {
        for(int j = 0; j < kMatrixHeight; j++) {
            leds[XY(i,j)] = CHSV(96,255,255);
        }
    }
    FastLED.show();
    Serial.println("LED: green");

    wdt_reset();
    delay(5000);

    relay.digitalWrite(REL_FAN_OUT_SMALL_PIN, REL_FAN_OUT_SMALL_INIT); Serial.println("Fan OUT small: off");
    relay.digitalWrite(REL_FAN_IN_SMALL_PIN, REL_FAN_IN_SMALL_INIT); Serial.println("Fan IN small: off");
    relay.digitalWrite(REL_ALARM_PIN, REL_ALARM_INIT); Serial.println("Light: on");
    if ( cleanroomLocked ) {
        relay.digitalWrite(REL_KEYPAD_PIN, REL_KEYPAD_INIT); Serial.println("Clean room: Keypad on");
    }

    // White light -----------------------------------------------
    for(int i = 0; i < kMatrixWidth; i++) {
        for(int j = 0; j < kMatrixHeight; j++) {
            leds[XY(i,j)] = CHSV(170,95,255);
        }
    }
    FastLED.show();
    Serial.println("LED: white");

    wdt_reset();
    delay(6000);

    if (cleanroomLocked) {
        relay.digitalWrite(REL_VALVE_PIN, REL_VALVE_INIT);
        Serial.println("Clean room: unlocked");
    }

    wdt_reset();
    delay(7000);
    wdt_reset();

    relay.digitalWrite(REL_FAN_OUT_BIG_PIN, REL_FAN_OUT_BIG_INIT);
    Serial.println("Fan OUT big: off");

    return true;
}



/*============================================================================================================
//===LED======================================================================================================
//==========================================================================================================*/

bool led_Init() {

    int LEDdelay = 3000;
    LEDS.addLeds<LED_STRIP,PWM_1_PIN,GRB>(leds_fr,NUM_LEDS_FR);
    LEDS.addLeds<LED_STRIP,PWM_2_PIN,GRB>(leds_cyl,NUM_LEDS_CYL);
    LEDS.setBrightness(255);

    // cycling through the colour to verfiy function
    if (DEBUG_MODE > 0) {
        for(int i = 0; i < NUM_LEDS_FR; i++) {
            leds_fr[i] = CRGB::Red;
        }
        FastLED.show();
        delay(LEDdelay);

        for(int i = 0; i < NUM_LEDS_FR; i++) {
            leds_fr[i] = CRGB::GreenYellow;
        }
        FastLED.show();
        delay(LEDdelay);

        for(int i = 0; i < NUM_LEDS_FR; i++) {
            leds_fr[i] = CRGB::Blue;
        }
        FastLED.show();
        delay(LEDdelay);

        for(int i = 0; i < NUM_LEDS_FR; i++) {
            leds_fr[i] = CRGB::Green;
        }

        FastLED.show();
        delay(LEDdelay);
        for(int i = 0; i < NUM_LEDS_FR; i++) {
            leds_fr[i] = CRGB::White;
        }
        FastLED.show();
        delay(LEDdelay);
    };

    for(int i = 0; i < NUM_LEDS_CYL; i++) {
        leds_cyl[i] = CRGB::Black;
    }
    FastLED.show();
    Serial.println("LED: red");


    return true;
}

void set_led_clrs(CRGB clr, CRGB stripe) {
    for(size_t i = 0; i < sizeof(stripe); i++) {
        stripe[i] = clr;
    };
    FastLED.show();
};


/*============================================================================================================
//===STD basics===================================================================================================
//==========================================================================================================*/

// OLED
#include "SSD1306Ascii.h"             /* https://github.com/greiman/SSD1306Ascii                            */
#include "SSD1306AsciiWire.h"         /* https://github.com/greiman/SSD1306Ascii                            */

// == Constructors
SSD1306AsciiWire oled;

void dbg_println(String print_dbg) {
    Serial.println(print_dbg);
    oled.println(print_dbg);
}


void print_logo_infos(String progTitle) {
    Serial.println(F("+-----------------------------------+"));
    Serial.println(F("|    TeamEscape HH&S ENGINEERING    |"));
    Serial.println(F("+-----------------------------------+"));
    Serial.println();
    Serial.println(progTitle);
    Serial.println();
    delay(200);
}


void print_serial_header() {
    Serial.begin(115200);
    print_logo_infos(title);

    dbg_println("!header_begin");
    dbg_println(title);
    dbg_println(versionDate);
    dbg_println(version);
}

void OLED_Init() {
    Wire.begin();

    oled.begin(&Adafruit128x64, OLED_I2C_ADD);
    oled.set400kHz();
    oled.setScroll(true);
    oled.setFont(System5x7);
}


void dbg_init() {
    Serial.begin(115200);
    OLED_Init();
    print_serial_header();
}


void print_setup_end() {
    Serial.println("!setup_end");
    Serial.println(); Serial.println("===================START====================="); Serial.println();
    // blink_onBoardled(100);
}

/*============================================================================================================
//===I2C connections===================================================================================================
//==========================================================================================================*/

void i2c_scanner() {
    Serial.println (F("I2C scanner:"));
    Serial.println (F("Scanning..."));
    byte wire_device_count = 0;

    for (byte i = 8; i < 120; i++) {
        Wire.beginTransmission (i);
        if (Wire.endTransmission () == 0) {
            Serial.print   (F("Found address: "));
            Serial.print   (i, DEC);
            Serial.print   (F(" (0x"));
            Serial.print   (i, HEX);
            Serial.print (F(")"));
            if (i == 39) Serial.print(F(" -> LCD"));
            if (i == 56) Serial.print(F(" -> LCD-I2C-Board"));
            if (i == 57) Serial.print(F(" -> Input-I2C-board"));
            if (i == 60) Serial.print(F(" -> Display"));
            if (i == 63) Serial.print(F(" -> Relay"));
            if (i == 22) Serial.print(F(" -> Servo-I2C-Board"));
            Serial.println();
            wire_device_count++;
            delay (1);
        }
    }

    Serial.print   (F("Found "));
    Serial.print   (wire_device_count, DEC);
    Serial.println (F(" device(s)."));
    Serial.println("successfully scanned I2C");
    Serial.println();
    delay(500);
}


bool relay_Init() {

    Serial.println("initializing relay");
    relay.begin(RELAY_I2C_ADD);
    delay(20);

    Serial.println("added relay port");

    for (int i=0; i<REL_AMOUNT; i++) {
        relay.digitalWrite(relayPinArray[i], relayInitArray[i]);
        Serial.print("     ");
        Serial.print("Relay ["); Serial.print(relayPinArray[i]); Serial.print("] set to "); Serial.println(relayInitArray[i]);
        delay(20);
    }

    Serial.println();
    Serial.println("successfully initialized relay");

    return true;
}

/*============================================================================================================
//===INPUTS===================================================================================================
//==========================================================================================================*/

Expander_PCF8574 iTrigger;
bool input_Init() {
    dbg_println("Input Init running ...");
    delay(5);
    iTrigger.begin(INPUT_I2C_ADD);
    delay(5);
    for (int i=0; i < 2; i++) {
        Serial.println(i);
        dbg_println(str(inputPinArray[i]));
        iTrigger.pinMode(inputPinArray[i], INPUT);
        dbg_println(str(iTrigger.digitalRead())

        delay(5);
    }
  return true;
}

/*============================================================================================================
//===SETUP====================================================================================================
//==========================================================================================================*/

void setup() {
    Serial.println("Setup");
    wdt_disable();
    dbg_init();


    //if (led_Init())	{Serial.println("LED:     ok"); }
    if (relay_Init())	{Serial.println("Relay:   ok");	}
    //if( input_Init()) {Serial.println("Inputs:  ok"); }

    delay(50);
    i2c_scanner();
    delay(50);

    delay(2000);
    wdt_enable(WDTO_8S);
    Serial.println(); Serial.println("===================date 08.07.2019====================="); Serial.println();
    Serial.println("===================START====================="); Serial.println();
    print_setup_end();
}

/*============================================================================================================
//===LOOP=====================================================================================================
//==========================================================================================================*/

void loop() {

    wdt_reset();


    if(!iTrigger.digitalRead(REED_PIN) && !firstRun && !entranceOpen) {
        Serial.println("Entrance: open");
        entranceOpen = true;
        relay.digitalWrite(REL_ENTRY_ALARM_PIN, !REL_ENTRY_ALARM_INIT);
    } else if (iTrigger.digitalRead(REED_PIN) && !firstRun  && entranceOpen) {
        Serial.println("Entrance: closed");
        entranceOpen = false;
        relay.digitalWrite(REL_ENTRY_ALARM_PIN, REL_ENTRY_ALARM_INIT);
    }

    if(!iTrigger.digitalRead(REED_1_PIN) && !firstRun) {
        Serial.println("Lab door: open");
        delay(2000);
        Serial.println("Lab door: open (2nd check)");
        if(!iTrigger.digitalRead(REED_1_PIN)) {
            Serial.println("Decontamination: start");
            wdt_reset();
            if (decontamination(false)) { Serial.println("Decontamination: end"); }
            firstRun = true;
        }
    }
    if (firstRun && iTrigger.digitalRead(REED_1_PIN)) {
        Serial.println("Lab door: closed");

        timenow=millis();   //get current time and reset timer in case the door is not closed for continuous 3 mins

        while (iTrigger.digitalRead(REED_1_PIN)) {  //check if the lab door is close
            wdt_reset();
            //Check if the door is closed for 3 mins with interval of 100ms.
            if ((millis()-timenow)>180000) {
                Serial.println("Lab door: closed for 3 mins -> RESET");
                software_Reset();
            }
            delay(100);
            //Serial.println(millis()-timenow);
        }
        Serial.println("Lab door: opened before 3 mins -> timerReset");
    }
}


void software_Reset() {
    Serial.println(F("Restarting in"));
    delay(250);
    for (byte i = 3; i>0; i--) {
        Serial.println(i);
        delay(100);
    }
    asm volatile ("  jmp 0");
}
