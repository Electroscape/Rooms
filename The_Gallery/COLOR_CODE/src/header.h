#pragma once

String title = "GL_ST_COLOR_CODE";
String versionDate = "02.02.2021";
String version = "version 1.0ST";
String brainName = String("BrCOLOR");
String relayCode = String("HID");

//==KEYPAD I2C================================/
const byte KEYPAD_ROWS = 1; 		// Zeilen
const byte KEYPAD_COLS = 4; 		// Spalten
const byte KEYPAD_CODE_LENGTH = 4;  //! create and use guess.len() better practice 
const char KeypadKeys[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'R','G','B','W'}
};

// Passwort
char* sercet_password = (char*)"RGBW";          // Simple sample password

unsigned long KeypadCodeResetSpan = 5000;     // Zeit bis Codereset
unsigned int KeypadDebounceTime = 50;     // Time for debouncing resolution

// Standards der Adressierung (Konvention)
	// Relayboard und OLED
	#define RELAY_I2C_ADD     	 0x3F     // Relay Expander																							*/
	#define OLED_I2C_ADD         0x3C     // Ist durch Hardware des OLEDs vorgegeben

// ______________________EINSTELLUNGEN______________________
// Pinbelegung
	#define EXP_MAGNET_PIN          3     // Relay: Magnet 			 					
// Keypad
	#define KEYPAD_I2C_ADD       0x39     // moeglich sind 0x38, 39, 3A, 3B, 3D

const int UpdateOLEDAfterDelay = 5000;    // Zeit, bis Display kurz flackert als Online Signal	

