#include "stb_namespace.h"

String title = "Light & Exit & RFID S v0.3";
String versionDate = "15.03.2021";
String version = "version 1.65";

// Passwords
char *lightPass = (char*) "1708";
char *exitPass = (char*) "2381984";
// Relay codes
String relayCodeLight = String("LIT");
String relayCodeAlarm = String("ALA");
String relayCodeExit = String("EXT");
String relayCodes[] = {relayCodeLight, relayCodeExit, relayCodeAlarm};

// Standards der Adressierung (Konvention)
// Relayboard und OLED
#define RELAY_I2C_ADD 0x3F  /* Relay Expander */
#define LIGHT_OLED_ADD 0x3C /* Ist durch Hardware des OLEDs 0x3C */
#define EXIT_OLED_ADD 0x3D  /* Ist durch Hardware des OLEDs */
// ______________________EINSTELLUNGEN______________________
// RFID

// relay BASICS
#define REL_AMOUNT 3
// RELAY
enum REL_PIN {
    REL_LICHT_PIN,  // 0 First room light
    REL_1_PIN,      // 1 Light 2nd room
    REL_2_PIN,      // 2 UV Light
    REL_ALARM_PIN,  // 3 Alarm
    REL_4_PIN,      // 4 Empty
    REL_5_PIN,      // 5 Fireplace valve
    REL_6_PIN,      // 6 valve holding the painting
    REL_EXIT_PIN    // 7 Exit door lock
};
enum REL_INIT {
    REL_LICHT_INIT = 1,  // DESCRIPTION OF THE RELAY WIRING
    REL_1_INIT = 1,      // NC = Empty | COM = Light +Ve | NO = 230V
    REL_2_INIT = 1,      // NC = Empty | COM = UV +Ve    | NO = 230V
    REL_ALARM_INIT = 1,  // DESCRIPTION OF THE RELAY WIRING
    REL_4_INIT = 1,      // DESCRIPTION OF THE RELAY WIRING
    REL_5_INIT = 1,      // DESCRIPTION OF THE RELAY WIRING
    REL_6_INIT = 1,      // NC = Empty | COM = 24V  | NO = Valve
    REL_EXIT_INIT = 1    // DESCRIPTION OF THE RELAY WIRING
};

const enum REL_PIN relayPinArray[] = {REL_LICHT_PIN, REL_ALARM_PIN, REL_EXIT_PIN};
const enum REL_INIT relayInitArray[] = {REL_LICHT_INIT, REL_ALARM_INIT, REL_EXIT_INIT};