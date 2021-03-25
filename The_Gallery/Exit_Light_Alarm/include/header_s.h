#include "stb_namespace.h"

String title = "Light & Exit & RFID S v0.3";
String versionDate = "15.03.2021";
String version = "version 1.65";

String brainName = String("BrRFID");
String relayCode = String("UVL");

// I2C Addresses
// Relayboard und OLED
#define RELAY_I2C_ADD 0x3F  /* Relay Expander */
#define LIGHT_OLED_ADD 0x3C /* Ist durch Hardware des OLEDs 0x3C */
#define EXIT_OLED_ADD 0x3D  /* Ist durch Hardware des OLEDs */

// Keypad Addresses
#define LIGHT_KEYPAD_ADD 0x38 /* möglich sind 0x38, 39, 3A, 3B, 3D                         */
#define EXIT_KEYPAD_ADD 0x39  /* möglich sind 0x38, 39, 3A, 3B, 3D                         */
// ______________________EINSTELLUNGEN______________________
// RFID

// Relay config
#define REL_EXIT_PIN 0
#define REL_ALARM_PIN 1
#define REL_LICHT_PIN 2
