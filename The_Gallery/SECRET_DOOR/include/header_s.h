#include "stb_namespace.h"

String title = "Secret Door";
String versionDate = "12.04.2021";
String version = "version 1.0";

String brainName = String("BrSEC");
String relayCode = String("FPK");

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
