/*
        2CP - TeamEscape - Engineering
        Author Martin Pek

        changelog
        v0.2 
		- Added LED clrs

        Todo:
        
*/

/**************************************************************************/
// I2C Port Expander
#include "PCF8574.h"
#include <Wire.h>
#include <SPI.h>

//Watchdog timer
#include <avr/wdt.h>

String title = "PN532 Example";
String versionDate = "22.01.2020";
String version = "version 0.2 wip oleds";

#include <FastLED.h>

#define RELAY_I2C_ADD     	0x3F         // Relay Expander
#define OLED_I2C_ADD        0x3C         // Predefined by hardware
#define LCD_I2C_ADD         0x27         // Predefined by hardware

// uncomment to use
#define DEBUGMODE           1

// RELAY
// PIN
enum REL_PIN{
    REL_1_PIN ,                              // 0 Door Opener
    REL_2_PIN ,                              // 1 Buzzer
    REL_3_PIN ,                              // 2
    REL_4_PIN ,                              // 3
    REL_5_PIN ,                              // 4   Schwarzlicht
    REL_6_PIN ,                              // 5   ROOM_LI
    REL_7_PIN ,                              // 6
    REL_8_PIN                                // 7
};

#define REL_SCHW_LI_PIN         4
#define REL_ROOM_LI_PIN         5

#define LIGHT_ON                0
#define LIGHT_OFF               1


enum REL_INIT{
  REL_1_INIT   =                1,        // COM-12V_IN, NO-12V_OUT, NC-/  set to 1 for magnet, 0 for mechanical
  REL_2_INIT   =                1,        // COM-12V_IN, NO-12V_OUT_DOOR, NC-12V_OUT_ALARM
  REL_3_INIT   =                1,        // NC-12V_OUT_ALARM
  REL_4_INIT   =                1,        // DESCRIPTION OF THE RELAY WIRING
  REL_5_INIT   =                LIGHT_ON,        // DESCRIPTION OF THE RELAY WIRING
  REL_6_INIT   =                LIGHT_ON,        // DESCRIPTION OF THE RELAY WIRING
  REL_7_INIT   =                1,        // DESCRIPTION OF THE RELAY WIRING
  REL_8_INIT   =                1        // COM AC_volt, NO 12_PS+, NC-/
};

// == constants

const enum REL_PIN relayPinArray[]  = {REL_1_PIN, REL_2_PIN, REL_3_PIN, REL_4_PIN, REL_5_PIN, REL_6_PIN, REL_7_PIN, REL_8_PIN};
const byte relayInitArray[] = {REL_1_INIT, REL_2_INIT, REL_3_INIT, REL_4_INIT, REL_5_INIT, REL_6_INIT, REL_7_INIT, REL_8_INIT};

#define REL_AMOUNT      8

#define NR_OF_LEDS              1     /* Anzahl der Pixel auf einem Strang (Test 1 Pixel)                   */
// BE CAREFUL WHEN SETTING ALL LEDS to a colour use the socket count for this
#define STRIPE_CNT              3
#define LED_STRIP_TYPE          WS2811
#define COLOR_ORDER             GRB // BRG

#define RFID_1_LED_PIN          9     /* Per Konvention ist dies RFID-Port 1                                */
#define RFID_2_LED_PIN          6     /* Per Konvention ist dies RFID-Port 2                                */
#define RFID_3_LED_PIN          5     /* Per Konvention ist dies RFID-Port 3                                */
#define RFID_4_LED_PIN          3     /* Per Konvention ist dies RFID-Port 4                                */

// only a max of 3 PWMS possible hence 3 stripes max, any further LEDs need to be addressed trhough the 3rd stipe
// here we go: 'LED_PINS' is not usable in a constant expression,
// cannot use arrays bec thats too newschool.
// MODIFIY IN LED_INIT ASWELL
CRGB LED_STRIPE_1[NR_OF_LEDS];
CRGB LED_STRIPE_2[NR_OF_LEDS];
CRGB LED_STRIPE_3[NR_OF_LEDS];

static CRGB LED_STRIPES[STRIPE_CNT] = {LED_STRIPE_1, LED_STRIPE_2, LED_STRIPE_3}; //

// == PN532 imports and setup

#include <Adafruit_PN532.h>

// If using the breakout with SPI, define the pins for SPI communication.
#define PN532_SCK               13
#define PN532_MOSI              11
#define PN532_MISO              12

#define RFID_1_SS_PIN           8     /* Per Konvention ist dies RFID-Port 1                                */
#define RFID_2_SS_PIN           7     /* Per Konvention ist dies RFID-Port 2                                */
#define RFID_3_SS_PIN           4     /* Per Konvention ist dies RFID-Port 3                                */
#define RFID_4_SS_PIN           2     /* Per Konvention ist dies RFID-Port 4                                */

const byte RFID_SSPins[]  = {RFID_1_SS_PIN,  RFID_2_SS_PIN,  RFID_3_SS_PIN,  RFID_4_SS_PIN};

// very manual but ... its C its gonna be bitching when it doesnt know during compilte time
// uncomment as needed
const Adafruit_PN532 RFID_0(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_SSPins[0]);
const Adafruit_PN532 RFID_1(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_SSPins[1]);
const Adafruit_PN532 RFID_2(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_SSPins[2]);
const Adafruit_PN532 RFID_3(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_SSPins[3]);
#define RFID_AMOUNT         4
#define RFID_SOLUTION_SIZE  2
const Adafruit_PN532 RFID_READERS[4] = {RFID_0, RFID_1, RFID_2, RFID_3}; //

const uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static char RFID_solutions[RFID_AMOUNT][RFID_SOLUTION_SIZE] = {};
const char RFID_cards[4][RFID_SOLUTION_SIZE] = {"AH", "SD", "GF", "AB"}; //

#define RFID_DATABLOCK      1

static bool game_finished = false;


/*==PCF8574=================================================================================================*/
Expander_PCF8574 relay;

void setup() {

    Serial.println("WDT endabled");
    wdt_enable(WDTO_8S);

    communication_init();

    Serial.println();
    Serial.println("LED: ... ");
    if (LED_init()) {Serial.println("LED: OK!");} else {Serial.println("LED: FAILED!");};

    wdt_reset();

    Serial.println();
    Serial.println("I2C: ... ");
    if (i2c_scanner()) {Serial.println("I2C: OK!");} else {Serial.println("I2C: FAILED!");};

    wdt_reset();

    Serial.println();
    Serial.println("RFID: ... ");
    if (RFID_Init()) {Serial.println("RFID: OK!");} else {Serial.println("RFID: FAILED!");};

    wdt_reset();

    Serial.println();
    Serial.println("Relay: ... ");
    if (relay_Init()) {Serial.println("Relay: OK!");} else {Serial.println("Relay: FAILED!");};

    Serial.println();
    print_setup_end();

}

void loop() {

    wdt_reset();

    if (game_finished) {
        Serial.println("Waiting for new Game!");
        delay(50);
        relay.digitalWrite(REL_ROOM_LI_PIN, LIGHT_ON);
        delay(1000);
    } else {

        delay(50);

        if (RFID_Gate_locked()) {
            Serial.println("\n\n GATE is LOCKED \n\n");
            delay(50);
            relay.digitalWrite(REL_SCHW_LI_PIN, LIGHT_OFF);
            delay(50);
            relay.digitalWrite(REL_ROOM_LI_PIN, LIGHT_ON);
        } else {
            wdt_reset();

            Serial.println("\n\n GATE is OPEN \n\n");
            relay.digitalWrite(REL_SCHW_LI_PIN, LIGHT_ON);
            delay(50);
            relay.digitalWrite(REL_ROOM_LI_PIN, LIGHT_OFF);
            delay(4000);
            game_finished = true;
        }

    }

    wdt_reset();
}

// RFID functions

bool RFID_Gate_locked() {

    uint8_t success;
    uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
    uint8_t uidLength;
    uint8_t data[16];
    memset(data, 0, 16);
    bool gate_locked = false;

    int cards_present[RFID_AMOUNT];
    memset(cards_present, 0, RFID_AMOUNT);
    int cards_present_cnt = 0;
    int test_result = 0;

    for (int reader_nr=0; reader_nr<RFID_AMOUNT; reader_nr++) {

        Serial.print("Checking presence for reader: ");Serial.println(reader_nr);

        success = RFID_READERS[reader_nr].readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
        if (success) {
            Serial.println("Card present on reader!");
#ifdef DEBUGMODE
            // PN532DEBUGPRINT needs to be enabled
            // RFID_READERS[reader_nr].PrintHex(uid, uidLength);
#endif
            cards_present[reader_nr] = 1;
            cards_present_cnt++;

            if (uidLength != 4) {
                Serial.println("Card is not Mifare classic, discarding card");
                gate_locked = true;
                continue;
            }

            if (!read_PN532(reader_nr, data, uid, uidLength)) {
                Serial.println("read failed");
                gate_locked = true;
                continue;
            }
#ifdef DEBUGMODE
            // PN532DEBUGPRINT needs to be enabled
            // RFID_READERS[reader_nr].PrintHexChar(data, 16);
#endif
        if (!data_correct(reader_nr, data)) {
            gate_locked = true;
            continue;
        } else {
            test_result += pow(2, reader_nr);
        }

        } else {
            dbg_println("No presence on the reader!");
            cards_present[reader_nr] = 0;
            gate_locked = true;
        }
        delay(50);
    }

    // adjusting of colours needs to be delayed otherwise we may get interfence
    delay(20);

    if (cards_present_cnt >= RFID_AMOUNT) {
        if (gate_locked) {
            led_set_all_clrs(CRGB::Red, NR_OF_LEDS);
            delay(500);
        } else {
            // optional could be removed
            led_set_all_clrs(CRGB::Green, NR_OF_LEDS);
            delay(500);
        }
    } else {
        for (int reader_nr=0; reader_nr<RFID_AMOUNT; reader_nr++) {
            if (cards_present[reader_nr]) {
                led_set_clrs(reader_nr, CRGB::White, NR_OF_LEDS);
            } else {
                led_set_clrs(reader_nr, CRGB::Black, NR_OF_LEDS);
            }

        }
    }
    delay(20);

    Serial.print("Result: ");Serial.println(test_result);
    Serial.print("\n");
    return gate_locked;
}

bool read_PN532(int reader_nr, uint8_t *data, uint8_t *uid, uint8_t uidLength) {

    uint8_t success;

    // authentication may be shifted to another function if we need to expand
    success = RFID_READERS[reader_nr].mifareclassic_AuthenticateBlock(uid, uidLength, RFID_DATABLOCK, 0, keya);
    dbg_println("Trying to authenticate block 4 with default KEYA value");
    delay(100);
    if (!success) {
        dbg_println("Authentication failed, card may already be authenticated");
        return false;
    }

    success = RFID_READERS[reader_nr].mifareclassic_ReadDataBlock(RFID_DATABLOCK, data);
    if (!success) {
        Serial.println("Reading failed, discarding card");
        return false;
    }
    return true;
}

bool data_correct(int current_reader, uint8_t *data) {
    uint8_t result = -1;

    for (int reader_nr=0; reader_nr<RFID_AMOUNT; reader_nr++) {

        for (int i=0; i<RFID_SOLUTION_SIZE; i++) {
            if (RFID_solutions[reader_nr][i] != data[i]) {
                // We still check for the other solutions to show the color
                // but we display it being the wrong solution of the riddle
                if (reader_nr == current_reader) {
                    Serial.print("Wrong card placed on reader: "); Serial.println(current_reader);
                }
                continue;
            } else {
                if (i >= RFID_SOLUTION_SIZE - 1) {
                    // its a valid card but not placed on the right socket
                    dbg_println("equal to result of reader");
                    dbg_println(i);
                    result = reader_nr;
                }
            }
        }

    }

    // handling of the colours of the individual cards
    if (result < 0) {
        Serial.print("undefined card placed on reader: "); Serial.println(current_reader);
    }

    if (result == current_reader) {
        Serial.print("Correct card placed on reader: "); Serial.println(current_reader);
        return true;
    } else {
        return false;
    }
}

// LED functions for Neopixel

bool LED_init() {

    // yep, exactly what it looks like...
    // we need to do it manually since we cannot define arrays easily for the preprocessor
    LEDS.addLeds<LED_STRIP_TYPE, RFID_1_LED_PIN, COLOR_ORDER>(LED_STRIPE_1, NR_OF_LEDS);
    LEDS.addLeds<LED_STRIP_TYPE, RFID_2_LED_PIN, COLOR_ORDER>(LED_STRIPE_2, NR_OF_LEDS);
    LEDS.addLeds<LED_STRIP_TYPE, RFID_3_LED_PIN, COLOR_ORDER>(LED_STRIPE_3, 2*NR_OF_LEDS);
    LEDS.setBrightness(100);
    delay(20);

    for (int stripe_nr = 0; stripe_nr < RFID_AMOUNT; stripe_nr++) {

        Serial.print("LED stripe: "); Serial.println(stripe_nr);

#ifdef DEBUGMODE
        led_set_clrs(stripe_nr, CRGB::Red, NR_OF_LEDS);
        delay(500);
        led_set_clrs(stripe_nr, CRGB::Green, NR_OF_LEDS);
        delay(500);
#endif
        led_set_clrs(stripe_nr, CRGB::Black, NR_OF_LEDS);

    }
    delay(20);

    return true;
}

void led_set_all_clrs(CRGB clr, int led_cnt) {
    for(int stripe_nr=0; stripe_nr<RFID_AMOUNT; stripe_nr++) {
        led_set_clrs(stripe_nr, clr, NR_OF_LEDS);
    }
}

void led_set_clrs(int stripe_nr, CRGB clr, int led_cnt) {
    for(size_t i = 0; i < led_cnt; i++) {
        switch(stripe_nr) {
            case 0:
                LED_STRIPE_1[i] = clr; break;
            case 1:
                LED_STRIPE_2[i] = clr; break;
            case 2:
                LED_STRIPE_3[i] = clr; break;
            case 3:
                LED_STRIPE_3[NR_OF_LEDS + i] = clr; break;
            default: Serial.println("wrong led selection"); break;
        }
    }
    FastLED.show();
    delay(10*led_cnt);
}


/*==FUNCTIONS==================================================================================================*/

void RFID_dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}


bool RFID_Init() {

    memcpy(RFID_solutions, RFID_cards, RFID_AMOUNT);

    for (int i=0; i<RFID_AMOUNT; i++) {

        delay(20);
        wdt_reset();

        Serial.print("initializing reader: "); Serial.println(i);
        RFID_READERS[i].begin();
        RFID_READERS[i].setPassiveActivationRetries(5);

        int retries = 0;
        while (true) {
            uint32_t versiondata = RFID_READERS[i].getFirmwareVersion();
            if (!versiondata) {
                Serial.println("Didn't find PN53x board");
                if (retries > 5) {
                    Serial.println("PN532 startup timed out, restarting");
                    software_Reset();
                }
            } else {
                Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
                Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
                Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
                break;
            }
            retries++;
        }
        // configure board to read RFID tags
        RFID_READERS[i].SAMConfig();
        delay(20);
    }
    return true;
}

void dbg_println(String print_dbg) {
 // #ifdef DEBUGMODE
    Serial.println(print_dbg);
 // #endif
}

void print_logo_infos(String progTitle) {
    Serial.println(F("+-----------------------------------+"));
    Serial.println(F("|    TeamEscape HH&S ENGINEERING    |"));
    Serial.println(F("+-----------------------------------+"));
    Serial.println();
    Serial.println(progTitle);
    Serial.println();
    delay(20);
}

void print_serial_header() {
    print_logo_infos(title);

    Serial.println("!header_begin");
    Serial.println(title);
    Serial.println(versionDate);
    Serial.println(version);
}

void communication_init() {
    Wire.begin();
    Serial.begin(115200);
    delay(20);
    print_serial_header();
}

void print_setup_end() {
    Serial.println("!setup_end");
    Serial.println(); Serial.println("===================START====================="); Serial.println();
}

bool relay_Init() {

    Serial.println("initializing relay");
    relay.begin(RELAY_I2C_ADD);
    delay(20);

    for (int i=0; i<REL_AMOUNT; i++) {
        relay.pinMode(relayPinArray[i], OUTPUT);
        relay.digitalWrite(relayPinArray[i], HIGH);
        delay(20);
    }

    delay(20);

    for (int i=0; i<REL_AMOUNT; i++) {
        relay.digitalWrite(relayPinArray[i], relayInitArray[i]);
        Serial.print("     ");
        Serial.print("Relay ["); Serial.print(relayPinArray[i]); Serial.print("] set to "); Serial.println(relayInitArray[i]);
        delay(20);
    }

    Serial.println();
    Serial.println("successfully initialized relay");
    delay(20);

    return true;
}

bool i2c_scanner() {
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
            delay (3);
        }
    }
    Serial.print   (F("Found "));
    Serial.print   (wire_device_count, DEC);
    Serial.println (F(" device(s)."));
    Serial.println("successfully scanned I2C");
    Serial.println();

    return true;
}

/*==RESET==================================================================================================*/

void software_Reset() {
    Serial.println(F("Restarting in"));
    delay(50);
    for (byte i = 3; i>0; i--) {
        Serial.println(i);
        delay(100);
    }
    asm volatile ("  jmp 0");
}
