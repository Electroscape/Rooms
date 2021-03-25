#include "stb_namespace.h"

String title = "Light & Exit & RFID S v0.3";
String versionDate = "15.03.2021";
String version = "version 1.65";

String brainName = String("BrRFID");
String relayCode = String("UVL");

// I2C Addresses
// Relayboard und OLED
#define RELAY_I2C_ADD 0x3F  /* Relay Expander */
#define OLED_ADD 0x3C /* Ist durch Hardware des OLEDs 0x3C */

// Keypad Addresses
#define KEYPAD_ADD 0x38
// ______________________EINSTELLUNGEN______________________
// RFID

// Relay config

#define REL_DOOR_INIT 0
#define REL_DOOR_PIN 0



// #define OLED_DISABLE 1
