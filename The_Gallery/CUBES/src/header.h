#pragma once

String title = "The Gallery";
String versionDate = "09.09.2020";
String version = "version 0.2HH";
String brainName = String("BrRFID");
String relayCode = String("UVL");

// RELAY
// PIN
enum REL_PIN{
    REL_1_PIN ,                              // 0 Door Opener
    REL_2_PIN ,                              // 1 Buzzer
    REL_3_PIN ,                              // 2
    REL_4_PIN ,                              // 3
    REL_SCHW_LI_PIN ,                        // 4   Schwarzlicht
    REL_ROOM_LI_PIN ,                        // 5   ROOM_LI
    REL_7_PIN ,                              // 6
    REL_8_PIN                                // 7
};

#define LIGHT_ON                1
#define LIGHT_OFF               0


enum REL_INIT{
  REL_1_INIT   =                1,        // COM-12V_IN, NO-12V_OUT, NC-/  set to 1 for magnet, 0 for mechanical
  REL_2_INIT   =                1,        // COM-12V_IN, NO-12V_OUT_DOOR, NC-12V_OUT_ALARM
  REL_3_INIT   =                1,        // NC-12V_OUT_ALARM
  REL_4_INIT   =                1,        // DESCRIPTION OF THE RELAY WIRING
  REL_SCHW_LI_INIT   =          LIGHT_OFF,        // DESCRIPTION OF THE RELAY WIRING
  REL_ROOM_LI_INIT   =          LIGHT_ON,        // DESCRIPTION OF THE RELAY WIRING
  REL_7_INIT   =                1,        // DESCRIPTION OF THE RELAY WIRING
  REL_8_INIT   =                1        // COM AC_volt, NO 12_PS+, NC-/
};

#define RFID_AMOUNT         3
#define RFID_SOLUTION_SIZE  3
static char RFID_solutions[4][RFID_SOLUTION_SIZE]  = {"AH", "SD", "GF", "PA"}; //


const uint16_t UpdateSignalAfterDelay = 5000;         /* Zeit, bis Serial print als Online Signal			*/