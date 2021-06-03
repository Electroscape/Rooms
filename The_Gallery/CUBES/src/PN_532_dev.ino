/*
        2CP - TeamEscape - Engineering
        Author Martin Pek & Abdullah Saei

        - Modified Serial outputs
        - Optimize initialization delay to smooth restarts.
        - Running version.
        - Locking after correct solution.

        Todo:
        - Nothing! working and stable.
*/

/**************************************************************************/
// Setting Configurations
#include "header_s.h"

// I2C Port Expander
#include <SPI.h>
#include <Wire.h>

#include "PCF8574.h"

// Watchdog timer
#include <avr/wdt.h>

// uncomment to use
#define DEBUGMODE 0

#define REL_AMOUNT 8

// == LEDS ================================================//

#define RFID_1_LED_PIN 9  // Per Konvention ist dies RFID-Port 1
#define RFID_2_LED_PIN 6  // Per Konvention ist dies RFID-Port 2
#define RFID_3_LED_PIN 5  // Per Konvention ist dies RFID-Port 3
#define RFID_4_LED_PIN 3  // Per Konvention ist dies RFID-Port 4

// NeoPixel
#include <Adafruit_NeoPixel.h>   // Ueber Bibliotheksverwalter
                                 // NeoPixel
#define NEOPIXEL_NR_OF_PIXELS 1  // Anzahl der Pixel auf einem Strang (Test 1 Pixel)
#define STRIPE_CNT 4

Adafruit_NeoPixel LED_Stripe_1 = Adafruit_NeoPixel(
    NEOPIXEL_NR_OF_PIXELS, RFID_1_LED_PIN, CLR_ORDER + NEO_KHZ800);
Adafruit_NeoPixel LED_Stripe_2 = Adafruit_NeoPixel(
    NEOPIXEL_NR_OF_PIXELS, RFID_2_LED_PIN, CLR_ORDER + NEO_KHZ800);
Adafruit_NeoPixel LED_Stripe_3 = Adafruit_NeoPixel(
    NEOPIXEL_NR_OF_PIXELS, RFID_3_LED_PIN, CLR_ORDER + NEO_KHZ800);
Adafruit_NeoPixel LED_Stripe_4 = Adafruit_NeoPixel(
    NEOPIXEL_NR_OF_PIXELS, RFID_4_LED_PIN, CLR_ORDER + NEO_KHZ800);

static uint32_t clr_black = LED_Stripe_1.Color(0,0,0);
static uint32_t clr_green = LED_Stripe_1.Color(0,255,0);
static uint32_t clr_yellow = LED_Stripe_1.Color(255,255,0);
static uint32_t clr_red = LED_Stripe_1.Color(255,0,0);

uint8_t gBrightness = 128;

// == PN532 imports and setup
#include <Adafruit_PN532.h>

// If using the breakout with SPI, define the pins for SPI communication.
#define PN532_SCK 13
#define PN532_MOSI 11
#define PN532_MISO 12

#define RFID_1_SS_PIN 8  // Per Konvention ist dies RFID-Port 1
#define RFID_2_SS_PIN 7  // Per Konvention ist dies RFID-Port 2
#define RFID_3_SS_PIN 4  // Per Konvention ist dies RFID-Port 3
#define RFID_4_SS_PIN 2  // Per Konvention ist dies RFID-Port 4

const byte RFID_SSPins[] = {
    RFID_1_SS_PIN,
    RFID_2_SS_PIN,
    RFID_3_SS_PIN,
    RFID_4_SS_PIN};

// very manual but ... its C its gonna be bitching when it doesn't know during compile time
const Adafruit_PN532 RFID_0(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_SSPins[0]);
const Adafruit_PN532 RFID_1(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_SSPins[1]);
const Adafruit_PN532 RFID_2(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_SSPins[2]);
const Adafruit_PN532 RFID_3(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_SSPins[3]);

Adafruit_PN532 RFID_READERS[4] = {
    RFID_0, RFID_1, RFID_2, RFID_3
};  //Maximum number for reader supported by STB

#define RFID_DATABLOCK 1
uint8_t keya[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

//==Variables==============================/
int cards_solution[RFID_AMOUNT] = {0};  //0 no card, 1 there is card, 2 correct card
bool game_active = false;
// used to detect a change in cards
uint8_t old_cards_present = 0;

//=====Timer=============================/
// first evaluation of periodic updates is in main loop,
// updating this value on the end of setup to delay it
unsigned long update_timer = 0;  // responsible for the periodic updates for the interface

//==PCF8574==============================/
Expander_PCF8574 relay;

//==Serial Printing======================/
const int ctrlPin = A0;  // the control pin of max485 rs485 LOW read, HIGH write

/*======================================
//===SETUP==============================
//====================================*/
void setup() {
    serial_init();
    Serial.println("WDT endabled");
    wdt_enable(WDTO_8S);

    Serial.println();
    Serial.println("LED: ... ");
    if (LED_init()) {
        Serial.println("LED: OK!");
    } else {
        Serial.println("LED: FAILED!");
    };

    wdt_reset();

    Serial.println();
    Serial.println("I2C: ... ");
    if (i2c_scanner()) {
        Serial.println("I2C: OK!");
    } else {
        Serial.println("I2C: FAILED!");
    };

    wdt_reset();

    Serial.println();
    Serial.println("RFID: ... ");
    if (RFID_init()) {
        Serial.println("RFID: OK!");
    } else {
        Serial.println("RFID: FAILED!");
    };

    wdt_reset();

    Serial.println();
    Serial.println("Relay: ... ");
    if (relay_init()) {
        Serial.println("Relay: OK!");
    } else {
        Serial.println("Relay: FAILED!");
    };

    wdt_reset();
    update_timer = millis();  // start delay
    printWithHeader("Setup Complete", "SYS");
}

void end_game() {
    wdt_reset();
    relay.digitalWrite(REL_ROOM_LI_PIN, LIGHT_OFF);
    relay.digitalWrite(REL_SCHW_LI_PIN, LIGHT_ON);

    delay(500);

    relay.digitalWrite(REL_ROOM_LI_PIN, REL_ROOM_LI_INIT);
    relay.digitalWrite(REL_SCHW_LI_PIN, REL_SCHW_LI_INIT);
    printWithHeader("Game Complete", "SYS");
}

/*======================================
//===LOOP==============================
//====================================*/
void loop() {
    //send refresh signal every interval of time
    wdt_reset();

    if (game_active) {
        if (RFID_loop()) {
            end_game();
        }
    } else {
        // periodically updating the LEDs in case there is noise
        neopixel_set_all_clr(clr_green);
    }

    wdt_reset();
    delay(500);
}


// RFID functions
/*
 * RFID framework read, check and update
 *
 * @param void
 * @return void
 */
bool RFID_loop() {
    uint8_t success;
    uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0};  // Buffer to store the returned UID
    uint8_t uidLength;
    uint8_t data[16];
    uint8_t cards_present = 0;
    uint8_t cards_correct = 0;
    String frontendMsg = "";

    for (uint8_t reader_nr = 0; reader_nr < RFID_AMOUNT; reader_nr++) {
        wdt_reset();
        //Serial.print("reader ");Serial.print(reader_nr);Serial.print(":");

        success = RFID_READERS[reader_nr].readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
        if (success) {
            cards_present = cards_present | (1 << reader_nr) ;
            if (uidLength != 4) {  //Card is not Mifare classic, discarding card
                Serial.println("Invalid Card type!");
                continue;
            }

            if (!read_PN532(reader_nr, data, uid, uidLength)) {
                Serial.println("read failed");
                frontendMsg += "Read_failed ";
                continue;
            }

            if (data_correct(reader_nr, data, frontendMsg)) {
                cards_correct++;
            }

        } else {
            // no card present
            frontendMsg += "__";
        }

    }

    // update of the interface
    if ( millis() - update_timer >= UpdateSignalAfterDelay ||
        cards_present != old_cards_present )
    {
        update_timer = millis();
        old_cards_present = cards_present;
        printWithHeader(frontendMsg, "SYS");
    }

    // if all reader have cards check if the riddle is correct or false
    if ( cards_present >= (1 << RFID_AMOUNT -1) ) {
        if (cards_correct >= RFID_AMOUNT) {
            neopixel_set_all_clr(clr_green);
            printWithHeader("!Correct", relayCode);
            return true;
        } else {
            neopixel_set_all_clr(clr_red);
            printWithHeader("!Wrong", relayCode);
        }
    } else {
        // if not all slots are present we light up the readers with cards
        for (uint8_t reader_nr = 0; reader_nr < RFID_AMOUNT; reader_nr++) {
            if ( (1<<reader_nr) & cards_present > 0 ) {
                neopixel_setClr(reader_nr, clr_yellow);
            } else {
                neopixel_setClr(reader_nr, clr_black);
            }
        }
    }

    return false;
}


/*
 * Reads specified reader, writes data into passed variables
 * @return true if success
 */
bool read_PN532(int reader_nr, uint8_t* data, uint8_t* uid, uint8_t uidLength) {
    bool success;

    // authentication may be shifted to another function if we need to expand
    success = RFID_READERS[reader_nr].mifareclassic_AuthenticateBlock(uid, uidLength, RFID_DATABLOCK, 0, keya);
    //dbg_println("Trying to authenticate block 4 with default KEYA value");
    delay(1);  //was 100!!
    if (!success) {
        //dbg_println("Authentication failed, card may already be authenticated");
        return false;
    }

    success = RFID_READERS[reader_nr].mifareclassic_ReadDataBlock(RFID_DATABLOCK, data);
    if (!success) {
        Serial.println("Reading failed, discarding card");
    }
    return success;
}

/* Checks if the solution for the given reader is correct aswell as creates
the frontend message
 */
bool data_correct(int current_reader, uint8_t *data, String &msg) {
    bool valid_card = false;
    bool correct_card = false;

    for (int solution_nr = 0; solution_nr < RFID_AMOUNT; solution_nr++) {

        if (RFID_solutions[solution_nr] == data) {
            correct_card = (solution_nr == current_reader);
            valid_card = true;
            msg = msg + "C";
            msg.concat(solution_nr + 1);
        }
        if (!valid_card) {
            msg = msg + "Unknown";
        }
        msg = msg + " ";
    }

    return correct_card;
}

/*
 * used for printing the buffer
 */
void RFID_dump_byte_array(byte* buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}

/*
 * Set LED to specific color
 *
 * @param i byte of LED index
 *        color_str string color name (red, green, white, gold, black)
 * @return void
 */
void neopixel_setClr(byte i, uint32_t color) {
    switch (i) {
        case 0:
            LED_Stripe_1.setPixelColor(0, color); LED_Stripe_1.show(); break;
        case 1:
            LED_Stripe_2.setPixelColor(0, color); LED_Stripe_2.show(); break;
        case 2:
            LED_Stripe_3.setPixelColor(0, color); LED_Stripe_3.show(); break;
        case 3:
            LED_Stripe_4.setPixelColor(0, color); LED_Stripe_4.show(); break;
    }
}

/*
 * Initialise LEDs library
 *
 * @param i byte LED index
 * @return void
 */
void neopixel_init(byte i) {
    switch (i) {
        case 0:
            LED_Stripe_1.begin();
            neopixel_setClr(i, clr_red);
            break;
        case 1:
            LED_Stripe_2.begin();
            neopixel_setClr(i, clr_red);
            break;
        case 2:
            LED_Stripe_3.begin();
            neopixel_setClr(i, clr_red);
            break;
        case 3:
            LED_Stripe_4.begin();
            neopixel_setClr(i, clr_red);
            break;
        default:
            break;
    }
    delay(100);
}

void neopixel_set_all_clr(uint32_t color) {
    for (size_t i = 0; i < STRIPE_CNT; i++) {
        neopixel_setClr(i, color);
    }
}

bool LED_init() {
    for (size_t i = 0; i < STRIPE_CNT; i++) {
        neopixel_init(i);
    }
    neopixel_set_all_clr(clr_black);
    return true;
}
/*
 * Initialise RFID
 *
 * @param void
 * @return true on success
 * @note When stuck WTD cause arduino to restart
 */
bool RFID_init() {
    bool success;
    for (int i = 0; i < RFID_AMOUNT; i++) {
        uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0};  // Buffer to store the returned UID
        uint8_t uidLength;                      // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

        wdt_reset();

        Serial.print("initializing reader: ");
        Serial.println(i);
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
                Serial.print("Found chip PN5");
                Serial.println((versiondata >> 24) & 0xFF, HEX);
                Serial.print("Firmware ver. ");
                Serial.print((versiondata >> 16) & 0xFF, DEC);
                Serial.print('.');
                Serial.println((versiondata >> 8) & 0xFF, DEC);
                break;
            }
            retries++;
        }
        // configure board to read RFID tags
        RFID_READERS[i].SAMConfig();
        delay(1);  //was 20!!!
        success = RFID_READERS[i].readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
        //cards_solution[i]= int(success);
    }

    return success;
}
/*
 * Initialise Relays on I2C
 *
 * @param void
 * @return true when done
 */
bool relay_init() {
    Serial.println("initializing relay");
    relay.begin(RELAY_I2C_ADD);

    for (int i = 0; i < REL_AMOUNT; i++) {
        relay.pinMode(relayPinArray[i], OUTPUT);
        relay.digitalWrite(relayPinArray[i], relayInitArray[i]);
        Serial.print("     ");
        Serial.print("Relay [");
        Serial.print(relayPinArray[i]);
        Serial.print("] set to ");
        Serial.println(relayInitArray[i]);
    }
    Serial.println();
    Serial.println("successfully initialized relay");
    return true;
}
/*
 * Discover devices on I2C bus
 *
 * @param void
 * @return true when done
 */
bool i2c_scanner() {
    Serial.println(F("I2C scanner:"));
    Serial.println(F("Scanning..."));
    byte wire_device_count = 0;

    for (byte i = 8; i < 120; i++) {
        Wire.beginTransmission(i);
        if (Wire.endTransmission() == 0) {
            Serial.print(F("Found address: "));
            Serial.print(i, DEC);
            Serial.print(F(" (0x"));
            Serial.print(i, HEX);
            Serial.print(F(")"));
            if (i == 39)
                Serial.print(F(" -> LCD"));
            if (i == 56)
                Serial.print(F(" -> LCD-I2C-Board"));
            if (i == 57)
                Serial.print(F(" -> Input-I2C-board"));
            if (i == 60)
                Serial.print(F(" -> Display"));
            if (i == 63)
                Serial.print(F(" -> Relay"));
            if (i == 22)
                Serial.print(F(" -> Servo-I2C-Board"));
            Serial.println();
            wire_device_count++;
            delay(1);
        }
    }
    Serial.print(F("Found "));
    Serial.print(wire_device_count, DEC);
    Serial.println(F(" device(s)."));
    Serial.println("successfully scanned I2C");
    Serial.println();

    return true;
}
/*
 * Initialize I2C, Serial and Arduino Pins
 *
 * @param void
 * @return void
 */
void serial_init() {
    Wire.begin();
    Serial.begin(115200);
    delay(2000);
    // initialize the read pin as an output:
    pinMode(ctrlPin, OUTPUT);
    // Test Serial Print
    Serial.println("\nSETUP\n");
    // Welcome Print
    printWithHeader("Setup Begin", "SYS");
}

/*
 * Prints with the correct format
 *
 * @param message string in the message field,
 *        source string the message source either relayCode or 'SYS'
 * @return void
 * @note switching pin for MAX485 control Write then Read
 */
void printWithHeader(String message, String source) {
    // turn Write mode on:

    digitalWrite(ctrlPin, MAX485_WRITE);
    Serial.println();
    Serial.print("!");
    Serial.print(brainName);
    Serial.print(",");
    Serial.print(source);
    Serial.print(",");
    Serial.print(message);
    Serial.println(",Done.");
    delay(50);
    // turn Write mode off:
    digitalWrite(ctrlPin, MAX485_READ);
}


// To restart the arduino by jumping to address 0x00
void software_Reset() {
    Serial.println(F("Restarting in"));
    delay(50);
    for (byte i = 3; i > 0; i--) {
        Serial.println(i);
        delay(100);
    }
    asm volatile("  jmp 0");
}
