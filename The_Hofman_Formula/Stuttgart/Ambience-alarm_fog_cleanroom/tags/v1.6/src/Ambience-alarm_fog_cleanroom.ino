/*==========================================================================================================*/
/*		2CP - TeamEscape - Engineering
 *		by Martin Pek &Abdullah Saei & Robert Schloezer
 *
 *		v1.5
 *		- Last Changes 08.07.2019
 *		- Add WDT
 *		- Bug fixing undefined state z = 5
        v1.6
        - changes to relay configuration and trigger with i2c keypad
 */
/*==========================================================================================================*/

const String title = "reinraumsteuerung";
const String versionDate = "15.10.2019";
const String version = "version 1.6";

/*==INCLUDE=================================================================================================*/
//Watchdog timer
#include <avr/wdt.h>
// I2C Port Expander
#include "PCF8574.h"

// WS2812B LED Strip
#include <FastLED.h>

#define SVERSION        1
// #define HHVERSION    0


/*==DEFINE==================================================================================================*/
// LED
// PIN
#define PWM_1_PIN   3                       // Predefined by STB design
#define PWM_2_PIN   5                       // Predefined by STB design
#define PWM_3_PIN   6                       // Predefined by STB design
#define PWM_4_PIN   9                       // Predefined by STB design
// SETTINGS

#define FR_LED_PIN      PWM_1_PIN

#define FR_LED_STRIP    WS2812B
#define FR_COLOR_ORDER  GRB
#define FR_LED_CNT      65 // 65
CRGB frleds[FR_LED_CNT];

#if SVERSION
    #define ZYL_LED_PIN     PWM_2_PIN
    // WS2812B used in HH we have 11
    #define ZYL_LED_STRIP       WS2811
    #define ZYL_COLOR_ORDER     BRG  // RGB  BRG GRB
    #define ZYL_LED_CNT         21  //21
    CRGB zylleds[ZYL_LED_CNT];
#endif

// I2C ADRESSES
#define RELAY_I2C_ADD     	    0x3F         // Relay Expander
#define DETECTOR_I2C_ADD        0x39//0x38         // door opening detector Expander
#define OLED_I2C_ADD            0x3C         // Predefined by hardware
#define LCD_I2C_ADD			    0x27 // Predefined by hardware

// RELAY
// PIN
#if SVERSION

    #define REL_KEYPAD_PIN          0
    #define REL_PNDOOR_PIN          1        // Pneumatic Valve from the door
    #define REL_FOG_PIN             2        // fog machine
    #define REL_FANS_SMALL_PIN      3        // small fan -> fog out
    #define REL_FOG_HEATING_PIN     4        // 230V for the fog machine to heat up
    #define REL_ROOM_LIGHT_PIN      5        // Roomlight
    #define REL_FAN_OUT_BIG_PIN     6        // big, yellow fan -> fog out
    #define REL_FR_LIGHTS_PIN       7        // three red lights on the wall -> blinks when procedure starts

    // INIT
    const byte relayPinArray[] = {
        REL_FR_LIGHTS_PIN,
        REL_FAN_OUT_BIG_PIN,
        REL_FANS_SMALL_PIN,
        REL_FOG_HEATING_PIN,
        REL_FOG_PIN,
        REL_ROOM_LIGHT_PIN,
        REL_PNDOOR_PIN,
        REL_KEYPAD_PIN
    };

    #define REL_LIGHTS_ON         0
    #define ON                    0

    #define REL_FR_LIGHTS_INIT       REL_LIGHTS_ON   // DESCRIPTION OF THE RELAY WIRING
    #define REL_FAN_OUT_BIG_INIT     1                  // DESCRIPTION OF THE RELAY WIRING
    #define REL_FANS_SMALL_INIT      1                  // DESCRIPTION OF THE RELAY WIRING
    #define REL_FOG_HEATING_INIT     0                  // DESCRIPTION OF THE RELAY WIRING
    #define REL_FOG_INIT             1                  // DESCRIPTION OF THE RELAY WIRING
    #define REL_ROOM_LIGHT_INIT      REL_LIGHTS_ON                  // DESCRIPTION OF THE RELAY WIRING
    #define REL_PNDOOR_OPEN          1
    #define REL_KEYPAD_INIT          1

    const byte relayInitArray[] = {
        REL_FR_LIGHTS_INIT,
        REL_FAN_OUT_BIG_INIT,
        REL_FANS_SMALL_INIT,
        REL_FOG_HEATING_INIT,
        REL_FOG_INIT,
        REL_ROOM_LIGHT_INIT,
        REL_PNDOOR_OPEN,
        REL_KEYPAD_INIT
    };

    // INPUT
    #define REED_LABDOOR_CLOSED         0                   // alarm light at the entrance/exit
    #define RESET_BUTTON                1
    #define CLEANROOM_KEYPAD_OPEN       4                   // trigger for starting the procedure
    const int inputs[] = {REED_LABDOOR_CLOSED, RESET_BUTTON, CLEANROOM_KEYPAD_OPEN};

    // OUTPUTS
    #define RPI_VIDEO_PIN               5
    const int outputs[] = {RPI_VIDEO_PIN};
    #define RPI_VIDEO_PIN_INIT          0
    const int outputInit[] = {RPI_VIDEO_PIN_INIT};

    // door needs 25s
    #define    ARMTIME              25000
    // opening of the Pneumatic door
    #define    DOOR_OPENTIME        18 // seconds

    #define TICKTIME                50
    #define KEYPADCHECKFOR          5 * TICKTIME
    static int keypadCheckTime      = 0;

#endif

#if HHVERSION
    #define REL_FR_LIGHTS_PIN       7        // three red lights on the wall -> blinks when procedure starts
    #define REL_FAN_OUT_BIG_PIN     6        // big, yellow fan -> fog out
    #define REL_FANS_SMALL_PIN      3        // small fan -> fog out
    #define REL_RPI_VIDEO_PIN       4        // triggers the RPI video sequence
    #define REL_FOG_PIN             2        // fog machine
    #define REL_ROOM_LIGHT_PIN      5        // Roomlight
    #define REL_PNDOOR_PIN          1        // Pneumatic Valve from the door
    #define REL_KEYPAD_PIN          0

    const byte relayPinArray[] = {
        REL_FR_LIGHTS_PIN,
        REL_FAN_OUT_BIG_PIN,
        REL_FANS_SMALL_PIN,
        REL_RPI_VIDEO_PIN,
        REL_FOG_PIN,
        REL_ROOM_LIGHT_PIN,
        REL_PNDOOR_PIN,
        REL_KEYPAD_PIN
    };
#endif

/*==FLAGS===================================================================================================*/
bool firstRun     = false;
bool entranceOpen = false;
static bool roomArmed = false;
static int stage = 0;
int resetTimer = 0;

long timenow = 0;
/*==PCF8574=================================================================================================*/
Expander_PCF8574 relay;
Expander_PCF8574 iTrigger;

/*============================================================================================================
//===LED======================================================================================================
//==========================================================================================================*/

void led_set_clrs(struct CRGB * stripe, CRGB clr, int led_cnt) {
    for(size_t i = 0; i < led_cnt; i++) {
        stripe[i] = clr;
    }
    FastLED.show();
}

bool led_Init() {
    dbg_println("LED init...");
    LEDS.addLeds<FR_LED_STRIP,FR_LED_PIN,FR_COLOR_ORDER>(frleds, FR_LED_CNT);
    delay(100);
    LEDS.addLeds<ZYL_LED_STRIP, ZYL_LED_PIN, ZYL_COLOR_ORDER>(zylleds, ZYL_LED_CNT);
    LEDS.setBrightness(128);
    delay(2000);

    led_set_clrs(zylleds, CRGB::Red, ZYL_LED_CNT);
    delay(2000);
    led_set_clrs(frleds, CRGB::Black, FR_LED_CNT);
    delay(1500);

    return true;
}

/*============================================================================================================
//===SETUP====================================================================================================
//==========================================================================================================*/
void setup() {
    wdt_disable();
    dbg_init();

    if (led_Init())	{Serial.println("LED:     ok"); };
    if (input_Init()) {Serial.println("Inputs:  ok"); }
    if (Relay_Init())	{Serial.println("Relay:   ok");	}

    delay(2000);
    wdt_enable(WDTO_8S);
    Serial.println(); Serial.println("===================date 08.07.2019====================="); Serial.println();
    print_setup_end();
}

/*============================================================================================================
//===LOOP=====================================================================================================
//==========================================================================================================*/
void loop() {

    wdt_reset();

    if (roomArmed) {
        //dbg_println("armed");
        stage_check();
    } else {
        arm_check();
    }
    delay(TICKTIME);
}


void arm_room() {
    relay.digitalWrite(REL_ROOM_LIGHT_PIN, !REL_LIGHTS_ON);
    relay.digitalWrite(REL_FR_LIGHTS_PIN, REL_LIGHTS_ON);
    relay.digitalWrite(REL_FOG_HEATING_PIN, REL_FOG_HEATING_INIT);
    iTrigger.digitalWrite(RPI_VIDEO_PIN, RPI_VIDEO_PIN_INIT);
    led_set_clrs(frleds, CRGB::Black, FR_LED_CNT);
    delay(1000);
    led_set_clrs(zylleds, CRGB::Red, ZYL_LED_CNT);
    delay(1000);
}


void stage_check() {

    switch (stage) {
        case 0: {
            if (!iTrigger.digitalRead(REED_LABDOOR_CLOSED)) {
                delay(100);
                if (!iTrigger.digitalRead(REED_LABDOOR_CLOSED)) {
                    delay(3000);
                    iTrigger.digitalWrite(RPI_VIDEO_PIN, !RPI_VIDEO_PIN_INIT);
                    delay(1000);
                    iTrigger.digitalWrite(RPI_VIDEO_PIN, RPI_VIDEO_PIN_INIT);
                    decontamination();
                    stage++;
                }
            }
        } break;

        case 1: {
            if (!iTrigger.digitalRead(CLEANROOM_KEYPAD_OPEN)) {
                keypadCheckTime += TICKTIME;
                if (keypadCheckTime >= KEYPADCHECKFOR) {
                    open_cleanroom();
                    keypadCheckTime = 0;
                    stage++;
                }
            } else {
                keypadCheckTime = 0;
            }
        } break;
        case 2: {
            roomArmed = false;
            stage = 0;
        }
        default: break;
    }
}


void arm_check() {
    if (iTrigger.digitalRead(RESET_BUTTON)) {

        // in case the SL forgets to cloose the labdoor we give him a hint
        if (!iTrigger.digitalRead(REED_LABDOOR_CLOSED)) {
            relay.digitalWrite(REL_KEYPAD_PIN, ON);
            delay(1000);
            relay.digitalWrite(REL_KEYPAD_PIN, !ON);
            delay(1000);
            relay.digitalWrite(REL_KEYPAD_PIN, ON);
            delay(1000);
            relay.digitalWrite(REL_KEYPAD_PIN, !ON);
        } else {
            dbg_println("reset pressed");
            Serial.println(resetTimer);
            relay.digitalWrite(REL_PNDOOR_OPEN, !REL_PNDOOR_OPEN);

            if (resetTimer >= ARMTIME) {
                dbg_println("!parse room has been reset");
                arm_room();
                roomArmed = true;
                resetTimer = 0;
            }
            resetTimer += 50;
        }

    } else {

        if (resetTimer > 0) {
            dbg_println("!parse Room reset not held long enough opening door");
            resetTimer = 0;
            reset_relays_init();
        }
    }
}


void open_cleanroom() {
    CRGB zyl_clrs_alternation[] = {CRGB::Black, CRGB::Red};
    dbg_println("!Parse opening the cleanroom");
    // led_set_clrs(zylleds, CRGB::Red, ZYL_LED_CNT);

    delay(1000);
    relay.digitalWrite(REL_PNDOOR_PIN, REL_PNDOOR_OPEN);
    relay.digitalWrite(REL_KEYPAD_PIN, !ON);

    // settings for the alarm light leds
    int beamWidth = 10;
    int instancesIntervall = 500;
    int instanceDuration = 300;
    int tickrate = instanceDuration / (ZYL_LED_CNT + beamWidth);
    int beamPos = 0;

    wdt_reset();

    for (int i=0; i<DOOR_OPENTIME * 1000; i++) {
        for (int beamPos = 0; beamPos < (beamWidth + ZYL_LED_CNT); beamPos++) {
            // Serial.println(beamPos);
            if (beamPos < ZYL_LED_CNT) {
                zylleds[beamPos] = zyl_clrs_alternation[1];
            }
            if (beamPos - beamWidth >= 0) {
                zylleds[beamPos - beamWidth] = zyl_clrs_alternation[0];
            }
            FastLED.show();
            delay(tickrate);
        }

        i = i + instancesIntervall;
        delay(instancesIntervall - instanceDuration);
        wdt_reset();
    }
    led_set_clrs(zylleds, CRGB::Green, ZYL_LED_CNT);
    delay(1000);
}


// pulls all relays to initstate used when room arming sequence is aborted
void reset_relays_init() {
    roomArmed = false;
    for (int i=0; i<sizeof(relayPinArray); i++) {
        relay.digitalWrite(relayPinArray[i], relayInitArray[i]);
        Serial.print("     ");
        Serial.print("Relay ["); Serial.print(relayPinArray[i]); Serial.print("] set to "); Serial.println(relayInitArray[i]);
        delay(20);
    }
}

/*============================================================================================================
//===DECONTAMINATION==========================================================================================
//==========================================================================================================*/

bool decontamination() {

    led_set_clrs(frleds, CRGB::Red, FR_LED_CNT);
    relay.digitalWrite(REL_FR_LIGHTS_PIN, REL_LIGHTS_ON); Serial.println("Light: off");
    wdt_reset();
    delay(5000);
    wdt_reset();
    delay(5000);
    wdt_reset();
    relay.digitalWrite(REL_FANS_SMALL_PIN, !REL_FANS_SMALL_INIT); Serial.println("Fan small: on");
    delay(3500);

    // Fog and blinking lights
    // used to run for z < 30, but due to the video in S being used being 85s
    // increased to 34
    for (int z=0; z<34; z++) {

        Serial.print("z: "); Serial.println(z);
        if (z==0) {relay.digitalWrite(REL_FOG_PIN, !REL_FOG_INIT); Serial.println("Fog: on");}
        else if (z==5) {relay.digitalWrite(REL_FOG_PIN, REL_FOG_INIT); Serial.println("Fog: off");}
        else if (z==6) {relay.digitalWrite(REL_FAN_OUT_BIG_PIN, !REL_FAN_OUT_BIG_INIT); Serial.println("Fan OUT big: on");}
        else{}//Serial.print("ez: "); Serial.println(z);}


        for(int j = 0; j < FR_LED_CNT; j++) {
            frleds[j] = CHSV(170,95,255);
        }
        FastLED.show();

        relay.digitalWrite(REL_FR_LIGHTS_PIN, !REL_LIGHTS_ON);
        wdt_reset();
        delay(1000);


        for(int j = 0; j < FR_LED_CNT; j++) {
            frleds[j] = CHSV(0,255,255);
        }
        FastLED.show();

        relay.digitalWrite(REL_FR_LIGHTS_PIN, REL_FR_LIGHTS_INIT);
        wdt_reset();
        delay(1000);
    }

    relay.digitalWrite(REL_FOG_HEATING_PIN, !REL_FOG_HEATING_INIT);
    relay.digitalWrite(REL_FR_LIGHTS_PIN, !REL_LIGHTS_ON);
    // Green light -----------------------------------------------
    led_set_clrs(frleds, CRGB::Green, FR_LED_CNT);
    Serial.println("LED: green");
    relay.digitalWrite(REL_FANS_SMALL_PIN, REL_FANS_SMALL_INIT); Serial.println("Fan small: off");
    wdt_reset();
    delay(3000);
    relay.digitalWrite(REL_ROOM_LIGHT_PIN, REL_LIGHTS_ON);
    relay.digitalWrite(REL_FR_LIGHTS_PIN, REL_LIGHTS_ON); Serial.println("Light: on");


    // White light -----------------------------------------------
    led_set_clrs(frleds, CRGB::MediumVioletRed, FR_LED_CNT);
    Serial.println("LED: white");

    // moved it forward 4s bec it really seems abrupt shutdown
    delay(1000);
    relay.digitalWrite(REL_FAN_OUT_BIG_PIN, REL_FAN_OUT_BIG_INIT);

    wdt_reset();
    // was 4s but i moved delay of 1s to the main fan
    delay(3000);

    relay.digitalWrite(REL_KEYPAD_PIN, ON);
    led_set_clrs(zylleds, CRGB::Green, ZYL_LED_CNT);

    return true;
}
/*============================================================================================================
//===INPUTS===================================================================================================
//==========================================================================================================*/
bool input_Init() {
    dbg_println("input_init...");
    iTrigger.begin(DETECTOR_I2C_ADD);
    for (int i = 0; i < sizeof(inputs); i++) {
        iTrigger.pinMode(inputs[i], INPUT);
        delay(20);
    }
    for (int i = 0; i < sizeof(outputs); i++) {
        iTrigger.pinMode(outputs[i], OUTPUT);
        iTrigger.digitalWrite(outputs[i], outputInit[i]);
        delay(20);
    }
    return true;
}


/*============================================================================================================
//===MOTHER===================================================================================================
//==========================================================================================================*/
bool Relay_Init() {
    dbg_println("relay init...");
    relay.begin(RELAY_I2C_ADD);
    for (int i=0; i<=8; i++) {
        relay.pinMode(i, OUTPUT);
        relay.digitalWrite(i, HIGH);
    }
    delay(50);

    for (int i=0; i<sizeof(relayPinArray); i++) {
        relay.digitalWrite(relayPinArray[i], relayInitArray[i]);
        Serial.print("     ");
        Serial.print("Relay ["); Serial.print(relayPinArray[i]); Serial.print("] set to "); Serial.println(relayInitArray[i]);
        delay(20);
    }

	return true;
}

void software_Reset() {
    Serial.println(F("Restarting in"));
    delay(250);
    for (byte i = 3; i>0; i--) {
        Serial.println(i);
        delay(1000);
    }
    asm volatile ("  jmp 0");
}

// OLED
#include "SSD1306Ascii.h"             /* https://github.com/greiman/SSD1306Ascii                            */
#include "SSD1306AsciiWire.h"
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
    dbg_println("!header_end");
}

void OLED_Init() {
    dbg_println("OLED init...");
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
