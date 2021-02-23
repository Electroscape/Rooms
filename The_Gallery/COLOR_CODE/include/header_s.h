/*
* To configure a relay:
*   - rename REL_X_PIN to informative name
*   - rename and set REL_X_INIT with your init value
*/
#pragma once

String title = "GL_ST_COLOR_CODE";
String versionDate = "02.02.2021";
String version = "version 1.0ST";
String brainName = String("BrCOLOR");
String relayCode = String("HID");

//==KEYPAD I2C================================/
const byte KEYPAD_ROWS = 1;  // Zeilen
const byte KEYPAD_COLS = 4;  // Spalten
const char KeypadKeys[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'R', 'G', 'B', 'W'}};

// Passwort
char* secret_password = (char*)"RGGBWGRBWG";  // Simple sample password

unsigned long KeypadCodeResetSpan = 5000;  // Zeit bis Codereset
unsigned int KeypadDebounceTime = 50;      // Time for debouncing resolution

// CONSTANTS
#define MAGNET_LOCK 0
#define MAGNET_OPEN 1

#define MAX485_READ LOW
#define MAX485_WRITE HIGH

// Standards der Adressierung (Konvention)
#define RELAY_I2C_ADD 0x3F   // Relay Expander																							*/
#define OLED_I2C_ADD 0x3C    // Ist durch Hardware des OLEDs vorgegeben
#define KEYPAD_I2C_ADD 0x39  // möglich sind 0x38, 39, 3A, 3B, 3D

// RELAY
enum REL_PIN {
    REL_1_PIN,       // 0 First room light
    REL_2_PIN,       // 1 Exit door
    REL_3_PIN,       // 2 Secret door
    REL_MAGNET_PIN,  // 3 MAGNET holding the painting
    REL_5_PIN,       // 4 UV light
    REL_6_PIN,       // 5 Second room light
    REL_7_PIN,       // 6 Empty
    REL_8_PIN        // 7 Peripherals powersupply
};

enum REL_INIT {
    REL_1_INIT = 1,                 // DESCRIPTION OF THE RELAY WIRING
    REL_2_INIT = 1,                 // DESCRIPTION OF THE RELAY WIRING
    REL_3_INIT = 1,                 // DESCRIPTION OF THE RELAY WIRING
    REL_MAGNET_INIT = MAGNET_LOCK,  // NC = Empty | COM = Magnet +Ve | NO = 12V
    REL_5_INIT = 1,                 // DESCRIPTION OF THE RELAY WIRING
    REL_6_INIT = 1,                 // DESCRIPTION OF THE RELAY WIRING
    REL_7_INIT = 1,                 // DESCRIPTION OF THE RELAY WIRING
    REL_8_INIT = 1                  // DESCRIPTION OF THE RELAY WIRING
};

const int UpdateOLEDAfterDelay = 5000;  // Zeit, bis Display kurz flackert als Online Signal

// == constants
const enum REL_PIN relayPinArray[] = {
    REL_1_PIN,
    REL_2_PIN,
    REL_3_PIN,
    REL_MAGNET_PIN,
    REL_5_PIN,
    REL_6_PIN,
    REL_7_PIN,
    REL_8_PIN};
const byte relayInitArray[] = {
    REL_1_INIT,
    REL_2_INIT,
    REL_3_INIT,
    REL_MAGNET_INIT,
    REL_5_INIT,
    REL_6_INIT,
    REL_7_INIT,
    REL_8_INIT};
