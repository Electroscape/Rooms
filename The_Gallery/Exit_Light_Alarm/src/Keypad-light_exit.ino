/*=============================================*/
/**		2CP - TeamEscape - Engineering
 *		by Martin Pek & Abdullah Saei
 *
 *		based on HH  keypad-light-exit v 1.5
 *		- use stb namespace
 *		- solve all warnings
 *		-
 */
/*=============================================*/

#include "header_s.h"
using namespace stb_namespace;

// #define RFID_DISABLE 1
// #define OLED_DISABLE 1

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

#ifndef RFID_DISABLE
#include <Adafruit_PN532.h>
#endif
// OLED
// #include "SSD1306Ascii.h"             /* https://github.com/greiman/SSD1306Ascii                            */
#ifndef OLED_DISABLE
#include "SSD1306AsciiWire.h" /* https://github.com/greiman/SSD1306Ascii                            */
#endif

#ifndef RFID_DISABLE
#define RFID_1_SS_PIN 8 /* Per Konvention ist dies RFID-Port 1  */
#define RFID_2_SS_PIN 7 /* Per Konvention ist dies RFID-Port 2  */
#define RFID_3_SS_PIN 4 /* Per Konvention ist dies RFID-Port 3  */
#define RFID_4_SS_PIN 2 /* Per Konvention ist dies RFID-Port 4  */
// If using the breakout with SPI, define the pins for SPI communication.
#define PN532_SCK 13
#define PN532_MOSI 11
#define PN532_MISO 12

const byte RFID_SSPins[] = {RFID_1_SS_PIN};

Adafruit_PN532 RFID_0(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_SSPins[0]);

#define RFID_AMOUNT 1
Adafruit_PN532 RFID_READERS[1] = {RFID_0};  //

int rfid_ticks = 0;
int rfid_last_scan = millis();
const int rfid_scan_delay = 500;
const int rfid_ticks_required = 3;
#endif

// Keypad Addresses
#define LIGHT_KEYPAD_ADD 0x38 /* möglich sind 0x38, 39, 3A, 3B, 3D                         */
#define EXIT_KEYPAD_ADD 0x39  /* möglich sind 0x38, 39, 3A, 3B, 3D                         */

/*==OLED====================================================================================================*/

#ifndef OLED_DISABLE
SSD1306AsciiWire light_oled;
SSD1306AsciiWire exit_oled;
SSD1306AsciiWire oleds[] = {light_oled, exit_oled};
const int keypad_reset_after = 2000;
#endif

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

static unsigned long update_timers[] = {millis(), millis()};

Keypad_I2C LightKeypad(makeKeymap(KeypadKeys), KeypadRowPins, KeypadColPins, KEYPAD_ROWS, KEYPAD_COLS, LIGHT_KEYPAD_ADD, PCF8574);
Keypad_I2C ExitKeypad(makeKeymap(KeypadKeys), KeypadRowPins, KeypadColPins, KEYPAD_ROWS, KEYPAD_COLS, EXIT_KEYPAD_ADD, PCF8574);
Keypad_I2C keypads[] = {LightKeypad, ExitKeypad};
static int usedkeypad = -1;

// Passwort
Password passLight = Password(lightPass);    // Schaltet das Licht im Büro an
Password passExit = Password(exitPass);  // Öffnet die Ausgangstür
Password passwords[] = {passLight, passExit};

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

bool relay_init() {
    Serial.println("initializing relay");
    relay.begin(RELAY_I2C_ADD);
    for (int i = 0; i < REL_AMOUNT; i++) {
        relay.pinMode(relayPinArray[i], OUTPUT);
        relay.digitalWrite(relayPinArray[i], relayInitArray[i]);
    }

    Serial.print(F("\n successfully initialized relay\n"));
    return true;
}

/*============================================================================================================
//===KEYPAD===================================================================================================
//==========================================================================================================*/

void keypadEvent(KeypadEvent eKey) {
    KeyState state = keypads[usedkeypad].getState();

    // Check which keypad
    switch (state) {
        case PRESSED:
            update_timers[usedkeypad] = millis();
            // Check with button is pressed
            switch (eKey) {
                case '#':
                    checkPassword();
                    break;
                case '*':
                    passwordReset();
#ifndef OLED_DISABLE
                    oleds[usedkeypad].clear();
#endif
                    break;
                default:
                    passwords[usedkeypad].append(eKey);
#ifndef OLED_DISABLE
                    oleds[usedkeypad].clear();
                    oleds[usedkeypad].setFont(Adafruit5x7);
                    oleds[usedkeypad].print("\n\n\n");
                    oleds[usedkeypad].setFont(Verdana12_bold);
                    oleds[usedkeypad].print("         ");
                    oleds[usedkeypad].println(passwords[usedkeypad].guess);
#endif
                    printWithHeader(passwords[usedkeypad].guess, relayCodes[usedkeypad]);
                    break;
            }
            break;
        default:
            break;
    }
}

void LightKeypadEvent(KeypadEvent eKey) {
    usedkeypad = 0;
    keypadEvent(eKey);
}

void ExitKeypadEvent(KeypadEvent eKey) {
    usedkeypad = 1;
    keypadEvent(eKey);
}

bool keypad_init() {
    keypads[0].addEventListener(LightKeypadEvent);  // Event Listener erstellen
    keypads[1].addEventListener(ExitKeypadEvent);  // Event Listener erstellen
    for (Keypad_I2C &keypad : keypads) {
        keypad.begin();
        keypad.setHoldTime(5000);
        keypad.setDebounceTime(20);
    }

    return true;
}

void keypad_update() {
    usedkeypad = -1;
    keypads[0].getKey();
    keypads[1].getKey();
}

void passwordReset() {
    switch (usedkeypad) {
        case 0:
            passLight.reset();
            printWithHeader("!Reset", "LIT");
            break;
        case 1:
            passExit.reset();
            printWithHeader("!Reset", "EXT");
            break;
    }
}

void checkPassword() {
    // don't check if there is no password entered
    if (strlen(passwords[usedkeypad].guess) < 1) return;
    if (passwords[usedkeypad].evaluate()) {
        printWithHeader("!Correct", relayCodes[usedkeypad]);
#ifndef OLED_DISABLE
        oleds[usedkeypad].clear();
        oleds[usedkeypad].setFont(Adafruit5x7);
        oleds[usedkeypad].print("\n\n\n");
        oleds[usedkeypad].setFont(Verdana12_bold);
        oleds[usedkeypad].println("   ACCESS GRANTED!");
#endif
        relay.digitalWrite(relayPinArray[usedkeypad], !relayInitArray[usedkeypad]);
        delay(3000);
    } else {
        printWithHeader("!Wrong", relayCodes[usedkeypad]);
#ifndef OLED_DISABLE
        oleds[usedkeypad].println("    ACCESS DENIED!");
#endif
    }
    passwordReset();
}

/*============================================================================================================
//===RFID====================================================================================================
//==========================================================================================================*/

#ifndef RFID_DISABLE
bool RFID_init() {
    for (int i = 0; i < RFID_AMOUNT; i++) {
        delay(20);

        Serial.print(F("initializing reader: "));
        Serial.println(i);
        RFID_READERS[i].begin();
        RFID_READERS[i].setPassiveActivationRetries(5);

        int retries = 0;
        while (true) {
            uint32_t versiondata = RFID_READERS[i].getFirmwareVersion();
            if (!versiondata) {
                Serial.print(F("Didn't find PN53x board\n"));
                if (retries > 5) {
                    Serial.print(F("PN532 startup timed out, restarting\n"));
                    softwareReset();
                }
            } else {
                Serial.print(F("Found chip PN5"));
                Serial.println((versiondata >> 24) & 0xFF, HEX);
                Serial.print(F("Firmware ver. "));
                Serial.print((versiondata >> 16) & 0xFF, DEC);
                Serial.print('.');
                Serial.println((versiondata >> 8) & 0xFF, DEC);
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

void RFID_alarm_check() {
    uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0};
    uint8_t uidLength;
    uint8_t success = RFID_READERS[0].readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
    if (success) {
        if (rfid_ticks > 0) {
            printWithHeader("vorhanden", relayCodeAlarm);
            Serial.print(F("resetting alarm\n"));
            relay.digitalWrite(REL_ALARM_PIN, REL_ALARM_INIT);
        }
        rfid_ticks = 0;
    } else {
        if (rfid_ticks == rfid_ticks_required) {
            printWithHeader("entfernt", relayCodeAlarm);
            Serial.print(F("compass removed, activating alarm\n"));
            relay.digitalWrite(REL_ALARM_PIN, !REL_ALARM_INIT);
            rfid_ticks++;
        } else if (rfid_ticks < rfid_ticks_required + 1) {
            rfid_ticks++;
        }
    }
}
#endif

/*============================================================================================================
//===OLED=====================================================================================================
//==========================================================================================================*/

#ifndef OLED_DISABLE
bool oled_init() {
    // &SH1106_128x64 &Adafruit128x64
    Serial.print(F("Oled init\n"));
    oleds[0].begin(&SH1106_128x64, LIGHT_OLED_ADD);
    delay(100);
    oleds[1].begin(&SH1106_128x64, EXIT_OLED_ADD);
    Serial.print(F("Oled Before LOOP\n"));
    for (SSD1306AsciiWire &oled : oleds) {
        oled.set400kHz();
        oled.setScroll(true);
        oled.clear();
        oled.setFont(Verdana12_bold);  // Arial_bold_14
        Serial.print(F("inside\n"));
    }

    return true;
}
#endif

void keypad_reset() {
    int outdated = 0;
    for (int keypad_no = 0; keypad_no < 2; keypad_no++) {
        if (millis() - update_timers[keypad_no] >= keypad_reset_after) {
            outdated++;
        }
    }
    if (outdated >= 2) {
        for (int keypad_no = 0; keypad_no < 2; keypad_no++) {
            update_timers[keypad_no] = millis();
        }

        if (strlen(passLight.guess) > 0) {
            Serial.println("checkpass Light \n\n");
            usedkeypad = 0;
            checkPassword();
        } else {
            oleds[0].clear();
        }
        if (strlen(passExit.guess) > 0) {
            Serial.println("checkpass Exit \n\n");
            usedkeypad = 1;
            checkPassword();
        } else {
            oleds[1].clear();
        }
        passwordReset();
    }
}

void setup() {
    brainSerialInit();
    print_serial_header();
    Serial.println("WDT endabled");
    int x = 0;
    int y = 1;
    int arr[] = {x , y};
    for (int &val : arr) {
        Serial.println(val);
        val += 5;
    }
    Serial.println("arr");
    Serial.println(arr[0]);
    Serial.println(arr[1]);
    Serial.println("Vals");
    Serial.println(x);
    Serial.println(y);
    // wdt_enable(WDTO_8S);
    wdt_reset();

    Serial.println("!setup_begin");

    i2cScanner();
    wdt_reset();
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

#ifndef RFID_DISABLE
    Serial.print(F("RFID: ..."));
    if (RFID_init()) {
        Serial.print(F(" ok\n"));
    }
#endif

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
    delay(1000);
}

/*============================================================================================================
//===LOOP=====================================================================================================
//==========================================================================================================*/

void loop() {
    wdt_reset();
    // Serial.println("keypad_update");
    keypad_update();
    // Serial.println("oled");
    keypad_reset();
    /*
	checkPassword();
	passwordReset();
	*/

#ifndef RFID_DISABLE
    if (millis() - rfid_last_scan > rfid_scan_delay) {
        rfid_last_scan = millis();
        RFID_alarm_check();
    }
#endif
}
