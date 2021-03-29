/*==========================================================================================================*/
/**		2CP - TeamEscape - Engineering
 *		by Martin Pek & Abdullah Saei
 *
 *		based on HH  keypad-light-exit v 1.5
 *		- use stb namespace
 *		- solve all warnings
 *		-
 */
/*==========================================================================================================*/

#include "header_s.h"
using namespace stb_namespace;

//Watchdog timer
#include <avr/wdt.h>
// RFID

// Keypad
#include <Keypad.h> /* Standardbibliothek                                                 */
#include <Keypad_I2C.h>
#include <Password.h>
#include <Wire.h> /* Standardbibliothek                                                 */

// I2C Port Expander
#include <PCF8574.h> /* https://github.com/skywodd/pcf8574_arduino_library - modifiziert!  */

#ifndef OLED_DISABLE
    #include "SSD1306AsciiWire.h" /* https://github.com/greiman/SSD1306Ascii                            */
    SSD1306AsciiWire oled;
#endif

// relay BASICS
#define REL_AMOUNT 8
enum REL_PIN {
    REL_1_PIN,  // 0 Door Opener
    REL_2_PIN,  // 1 Buzzer
    REL_3_PIN,  // 2
    REL_4_PIN,  // 3
    REL_5_PIN,  // 4
    REL_6_PIN,  // 5
    REL_7_PIN,  // 6
    REL_8_PIN   // 7
};


#define REL_2_INIT 0
#define REL_3_INIT 0
#define REL_4_INIT 0
#define REL_5_INIT 0
#define REL_6_INIT 0
#define REL_7_INIT 0
#define REL_8_INIT 0

const enum REL_PIN relayPinArray[] = {REL_1_PIN, REL_2_PIN, REL_3_PIN, REL_4_PIN, REL_5_PIN, REL_6_PIN, REL_7_PIN, REL_8_PIN};
const byte relayInitArray[] = {REL_DOOR_INIT, REL_2_INIT, REL_3_INIT, REL_4_INIT, REL_5_INIT, REL_6_INIT, REL_7_INIT, REL_8_INIT};


/*==OLED====================================================================================================*/


const int keypad_reset_after = 2000;

/*==KEYPAD I2C==============================================================================================*/
const byte KEYPAD_ROWS = 4;  // Zeilen
const byte KEYPAD_COLS = 3;  // Spalten
const byte KEYPAD_CODE_LENGTH = 4;
const byte KEYPAD_CODE_LENGTH_MAX = 7;

char KeypadKeys[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};

byte KeypadRowPins[KEYPAD_ROWS] = {1, 6, 5, 3};  // Zeilen  - Messleitungen
byte KeypadColPins[KEYPAD_COLS] = {2, 0, 4};     // Spalten - Steuerleitungen (abwechselnd HIGH)

static unsigned long update_timer = millis();

Keypad_I2C Keypad(makeKeymap(KeypadKeys), KeypadRowPins, KeypadColPins, KEYPAD_ROWS, KEYPAD_COLS, KEYPAD_ADD, PCF8574);

// Passwort
Password passKeypad = Password(makeKeymap("5314"));

/*==PCF8574=================================================================================================*/
Expander_PCF8574 relay;

/*============================================================================================================
//===BASICS===================================================================================================
//==========================================================================================================*/

void print_serial_header() {
    printWithHeader("!header_begin", "SYS");
    printWithHeader(title, "SYS");
    printWithHeader(versionDate, "SYS");
    printWithHeader(version, "SYS");
}

void output_init() {
    Serial.begin(115200);
    Wire.begin();
    print_serial_header();
}

/*============================================================================================================
//===MOTHER===================================================================================================
//==========================================================================================================*/

bool i2c_scanner() {
    Serial.println();
    Serial.println(F("I2C scanner:"));
    Serial.println(F("Scanning..."));
    byte count = 0;
    for (byte i = 8; i < 120; i++) {
        Wire.beginTransmission(i);
        if (Wire.endTransmission() == 0) {
            Serial.print("Found address: ");
            Serial.print(i, DEC);
            Serial.print(" (0x");
            Serial.print(i, HEX);
            Serial.println(")");
            count++;
            delay(1);  // maybe unneeded?
        }              // end of good response
    }                  // end of for loop
    Serial.println("Done.");
    Serial.print("Found ");
    Serial.print(count, DEC);
    Serial.println(" device(s).");

    return true;
}

bool relay_init() {
    Serial.println("initializing relay");
    relay.begin(RELAY_I2C_ADD);

    for (int i = 0; i < REL_AMOUNT; i++) {
        relay.pinMode(i, OUTPUT);
        relay.digitalWrite(i, HIGH);
    }

    for (int i = 0; i < REL_AMOUNT; i++) {
        relay.digitalWrite(relayPinArray[i], relayInitArray[i]);
    }

    Serial.print(F("\n successfully initialized relay\n"));
    return true;
}

/*============================================================================================================
//===KEYPAD===================================================================================================
//==========================================================================================================*/

void keypadEvent(KeypadEvent eKey) {
    Serial.print(F("keypadevent on keypad"));
    KeyState state = IDLE;

    state = Keypad.getState();

    if (eKey) {
        Serial.println(eKey);
    };

    switch (state) {
        case PRESSED:
            update_timer = millis();
            Serial.print(F("Taste: "));
            Serial.print(eKey);

            switch (eKey) {

                case '#':
                    checkPassword();
                    break;

                case '*':
                    #ifndef OLED_DISABLE
                        oled.clear();
                    #endif
                    passwordReset();
                    break;

                default:
                    passKeypad.append(eKey);
                    #ifndef OLED_DISABLE
                        oled.clear();
                        oled.setFont(Adafruit5x7);
                        oled.print("\n\n\n");
                        oled.setFont(Verdana12_bold);
                        oled.print("         ");
                        oled.println(passKeypad.guess);
                    #endif
                    printWithHeader(passKeypad.guess, "FPK");
                    break;
            } break;

        default: break;
    }
}


bool keypad_init() {
    Keypad.addEventListener(keypadEvent);  // Event Listener erstellen
    Keypad.begin(makeKeymap(KeypadKeys));
    Keypad.setHoldTime(5000);
    Keypad.setDebounceTime(20);
    return true;
}


void passwordReset() {
    if (strlen(passKeypad.guess) > 0) {
        passKeypad.reset();
        printWithHeader("!Reset", "FPK");
    }
}

void checkPassword() {
    if (strlen(passKeypad.guess) < 1) return;
    if (passKeypad.evaluate()) {
        printWithHeader("!Correct", "FPK");
        #ifndef OLED_DISABLE
            oled.clear();
            oled.setFont(Adafruit5x7);
            oled.print("\n\n\n");
            oled.setFont(Verdana12_bold);
            oled.println("   ACCESS GRANTED!");
        #endif
        relay.digitalWrite(REL_DOOR_PIN, !REL_DOOR_INIT);
        delay(3000);
    } else {
        printWithHeader("!Wrong", "FPK");
        #ifndef OLED_DISABLE
            oled.println("    ACCESS DENIED!");
        #endif
    }
    passwordReset();
}


/*============================================================================================================
//===OLED=====================================================================================================
//==========================================================================================================*/

#ifndef OLED_DISABLE
bool oled_init() {
    // &SH1106_128x64 &Adafruit128x64
    Serial.print(F("Oled init\n"));
    oled.begin(&SH1106_128x64, OLED_ADD);
    oled.set400kHz();
    oled.setScroll(true);
    oled.clear();
    oled.setFont(Verdana12_bold);  // Arial_bold_14
    delay(1000);
    return true;
}
#endif

void keypad_reset() {
    if (millis() - update_timer >= keypad_reset_after) {

        update_timer = millis();

        if (strlen(passKeypad.guess) > 0) {
            checkPassword();
        } else {
            oled.clear();
        }

        passwordReset();
    }
}

void setup() {
    output_init();
    Serial.println("WDT endabled");
    // wdt_enable(WDTO_8S);
    wdt_reset();

    Serial.println("!setup_begin");

    i2c_scanner();

#ifndef OLED_DISABLE
    Serial.print(F("Oleds: ..."));
    if (oled_init()) {
        Serial.println(" light OLED ok...");
    }
#endif

    Serial.print(F("Relay: ..."));
    if (relay_init()) {
        Serial.println(" ok");
    }
    wdt_reset();

    delay(50);

    Serial.print(F("Keypad: ..."));
    if (keypad_init()) {
        Serial.print(F(" ok\n"));
    }
    wdt_reset();
    delay(5);

    Serial.println();
    Serial.println("===================START=====================");
    Serial.println();

    Serial.print(F("!setup_end\n\n"));
    delay(2000);
}

/*============================================================================================================
//===LOOP=====================================================================================================
//==========================================================================================================*/

void loop() {
    wdt_reset();

    Keypad.getKey();
    keypad_reset();
}
