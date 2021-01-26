/*==========================================================================================================*/
/*		2CP - TeamEscape - Engineering
 *		by Martin Pek & Robert Schloezer
 *
 *		based on HH  keypad-light-exit v 1.5
 *		-
 *		-
 *		-
 */
/*==========================================================================================================*/

String title = "Light & Exit & RFID S v0.2";
String versionDate = "02.07.2020";
String version = "version 1.13";

// #define RFID_DISABLE 1
// #define OLED_DISABLE 1

//Watchdog timer
#include <avr/wdt.h>
// RFID

// Keypad
#include <Wire.h>                     /* Standardbibliothek                                                 */
#include <Keypad.h>                   /* Standardbibliothek                                                 */
#include <Keypad_I2C.h>               /*                                                                    */
#include <Password.h>                 /* http://www.arduino.cc/playground/uploads/Code/Password.zip
                                 Muss modifiziert werden:
                                 Password.h -> char guess[ MAX_PASSWORD_LENGTH ];
                                 und byte currentIndex; muessen PUBLIC sein                         */
// I2C Port Expander
#include <PCF8574.h>                  /* https://github.com/skywodd/pcf8574_arduino_library - modifiziert!  */

#ifndef RFID_DISABLE
	#include <Adafruit_PN532.h>
#endif
// OLED
// #include "SSD1306Ascii.h"             /* https://github.com/greiman/SSD1306Ascii                            */
#ifndef OLED_DISABLE
	#include "SSD1306AsciiWire.h"         /* https://github.com/greiman/SSD1306Ascii                           */
#endif


// Standards der Adressierung (Konvention)
	// Relayboard und OLED
		#define RELAY_I2C_ADD     	 	0x3F     /* Relay Expander												*/
		#define LIGHT_OLED_ADD       	0x3C     /* Ist durch Hardware des OLEDs 0x3C							*/
		#define EXIT_OLED_ADD        	0x3D     /* Ist durch Hardware des OLEDs 								*/
	// Keypad Adresses
		#define LIGHT_KEYPAD_ADD       	0x38     /* moeglich sind 0x38, 39, 3A, 3B, 3D                          */
		#define EXIT_KEYPAD_ADD			0x39     /* moeglich sind 0x38, 39, 3A, 3B, 3D                          */

// ______________________EINSTELLUNGEN______________________
	// RFID
	#ifndef RFID_DISABLE
		#define RFID_1_SS_PIN           8     /* Per Konvention ist dies RFID-Port 1                             */
		#define RFID_2_SS_PIN           7     /* Per Konvention ist dies RFID-Port 2                             */
		#define RFID_3_SS_PIN           4     /* Per Konvention ist dies RFID-Port 3                             */
		#define RFID_4_SS_PIN           2     /* Per Konvention ist dies RFID-Port 4                             */

		const byte RFID_SSPins[]  = {RFID_1_SS_PIN};

		// If using the breakout with SPI, define the pins for SPI communication.
		#define PN532_SCK               13
		#define PN532_MOSI              11
		#define PN532_MISO              12

		const Adafruit_PN532 RFID_0(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_SSPins[0]);

		#define RFID_AMOUNT         1
		Adafruit_PN532 RFID_READERS[1] = {RFID_0}; //

		int rfid_ticks = 0;
		int rfid_last_scan = millis();
		const int rfid_scan_delay = 500;
		const int rfid_ticks_required = 3;
	#endif
		// no key or pass atm, we check only for presence

	// relay BASICS
		#define REL_AMOUNT      8
		enum REL_PIN{
		    REL_1_PIN ,                              // 0 Door Opener
		    REL_2_PIN ,                              // 1 Buzzer
		    REL_3_PIN ,                              // 2
		    REL_4_PIN ,                              // 3
		    REL_5_PIN ,                              // 4
		    REL_6_PIN ,                              // 5
		    REL_7_PIN ,                              // 6
		    REL_8_PIN                                // 7
		};

		#define REL_EXIT_INIT       0
		#define REL_ALARM_INIT      1
		#define REL_LICHT_INIT      1
		#define REL_4_INIT      	0
		#define REL_5_INIT      	0
		#define REL_6_INIT      	0
		#define REL_7_INIT      	0
		#define REL_8_INIT      	0

		const enum REL_PIN relayPinArray[]  = {REL_1_PIN, REL_2_PIN, REL_3_PIN, REL_4_PIN, REL_5_PIN, REL_6_PIN, REL_7_PIN, REL_8_PIN};
		const byte relayInitArray[] = {REL_EXIT_INIT, REL_ALARM_INIT, REL_LICHT_INIT, REL_4_INIT, REL_5_INIT, REL_6_INIT, REL_7_INIT, REL_8_INIT};

	// relay config
		#define REL_EXIT_PIN 	0
		#define REL_ALARM_PIN	1
		#define REL_LICHT_PIN 	2


/*==OLED====================================================================================================*/

#ifndef OLED_DISABLE
	SSD1306AsciiWire light_oled;
	SSD1306AsciiWire exit_oled;
	const int oled_reset_after = 2000;
#endif





/*==KEYPAD I2C==============================================================================================*/
const byte KEYPAD_ROWS = 4; 		// Zeilen
const byte KEYPAD_COLS = 3; 		// Spalten
const byte KEYPAD_CODE_LENGTH = 4;
const byte KEYPAD_CODE_LENGTH_MAX = 7;

const char KeypadKeys[KEYPAD_ROWS][KEYPAD_COLS] = {
	{'1','2','3'},
	{'4','5','6'},
	{'7','8','9'},
	{'*','0','#'}
};

byte KeypadRowPins[KEYPAD_ROWS] = {1, 6, 5, 3}; 	// Zeilen  - Messleitungen
byte KeypadColPins[KEYPAD_COLS] = {2, 0, 4};    	// Spalten - Steuerleitungen (abwechselnd HIGH)

static unsigned long update_timers[] = {millis(), millis()};

Keypad_I2C LightKeypad( makeKeymap(KeypadKeys), KeypadRowPins, KeypadColPins, KEYPAD_ROWS, KEYPAD_COLS, LIGHT_KEYPAD_ADD, PCF8574);
Keypad_I2C ExitKeypad( makeKeymap(KeypadKeys), KeypadRowPins, KeypadColPins, KEYPAD_ROWS, KEYPAD_COLS, EXIT_KEYPAD_ADD, PCF8574);
static int usedkeypad = -1;

// Passwort
Password passLight = Password((char *) "1708" );          // Schaltet das Licht im Buero an
Password passExit  = Password((char *) "2381984" );       // Oeffnet die Ausgangstuer

/*==PCF8574=================================================================================================*/
Expander_PCF8574 relay;

/*============================================================================================================
//===BASICS===================================================================================================
//==========================================================================================================*/


void print_serial_header() {
	Serial.println(title);

    Serial.print(F("!header_begin\n"));
    Serial.println(title);
    Serial.println(versionDate);
    Serial.println(version);
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
    Serial.println (F("I2C scanner:"));
    Serial.println (F("Scanning..."));
	byte count = 0;
	for (byte i = 8; i < 120; i++) {
		Wire.beginTransmission (i);
		if (Wire.endTransmission () == 0) {
			Serial.print ("Found address: ");
			Serial.print (i, DEC);
			Serial.print (" (0x");
			Serial.print (i, HEX);
			Serial.println (")");
			count++;
			delay (1);  // maybe unneeded?
		} // end of good response
	} // end of for loop
	Serial.println ("Done.");
	Serial.print ("Found ");
	Serial.print (count, DEC);
	Serial.println (" device(s).");

    return true;
}


bool relay_init() {
    Serial.println("initializing relay");
    relay.begin(RELAY_I2C_ADD);

    for (int i=0; i<REL_AMOUNT; i++) {
        relay.pinMode(i, OUTPUT);
        relay.digitalWrite(i, HIGH);
    }

    for (int i=0; i<REL_AMOUNT; i++) {
	    relay.digitalWrite(relayPinArray[i], relayInitArray[i]);
    }

    Serial.print(F("\n successfully initialized relay\n"));
    return true;
}


/*============================================================================================================
//===KEYPAD===================================================================================================
//==========================================================================================================*/

void keypadEvent(KeypadEvent eKey){

	Serial.print(F("keypadevent on keypad")); Serial.println(usedkeypad);
	KeyState state = IDLE;

	switch (usedkeypad) {
		case 0:
			state = LightKeypad.getState();
		break;
		case 1:
			state = ExitKeypad.getState();
		break;
	}

	if (eKey) { Serial.println(eKey);};

	switch( state ) {

		case PRESSED:
			update_timers[usedkeypad] = millis();
			Serial.print(F("Taste: ")); Serial.print(eKey);

			switch (eKey) {
				case '#':
					checkPassword();
				break;
				case '*':
					light_oled.clear();
					passwordReset();
				break;
				default:
					switch (usedkeypad) {
						case 0:
							passLight.append(eKey);
							#ifndef OLED_DISABLE
								light_oled.clear();
								light_oled.print("\n   ");
								light_oled.print(passLight.guess);
							#endif
							Serial.print(F(" -> Code: ")); Serial.println(passLight.guess);
						break;
						case 1:
							passExit.append(eKey);
							#ifndef OLED_DISABLE
								exit_oled.clear();
								exit_oled.print("\n   ");
								exit_oled.print(passExit.guess);
							#endif
							Serial.print(F(" -> Code: ")); Serial.println(passExit.guess);
						break;
					}
				break;
			}
		break;

		default: break;
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

	LightKeypad.addEventListener(LightKeypadEvent);    // Event Listener erstellen
	LightKeypad.begin( makeKeymap(KeypadKeys) );
	LightKeypad.setHoldTime(5000);
	LightKeypad.setDebounceTime(20);

	ExitKeypad.addEventListener(ExitKeypadEvent);    // Event Listener erstellen
	ExitKeypad.begin( makeKeymap(KeypadKeys) );
	ExitKeypad.setHoldTime(5000);
	ExitKeypad.setDebounceTime(20);

	return true;
}

void keypad_update() {
	usedkeypad = -1;
	LightKeypad.getKey();
	ExitKeypad.getKey();
}

void passwordReset() {
	Serial.print(F("Resetting Password for: "));
	switch (usedkeypad) {
		case 0:
			passLight.reset(); Serial.print(F("Light\n"));
		break;
		case 1:
			passExit.reset(); Serial.print(F("Exit\n"));
		break;
	}
}

void checkPassword() {
	switch (usedkeypad) {
		Serial.print(F("Checking password entry for: "));
		case 0:
			Serial.print(F("Light\n"));
			if (passLight.evaluate()) {
				Serial.print(F( "Korrektes Passwort: Licht an\n"));
				light_oled.println("\n  ACCESS GRANTED!");
				relay.digitalWrite(REL_LICHT_PIN, !REL_LICHT_INIT);
				delay(3000);
			} else {
				light_oled.println("\n  ACCESS DENIED!");
			}
			passwordReset();
		break;
		case 1:
			Serial.print(F("Exit"));
			if (passExit.evaluate()) {
				Serial.print(F( "Korrektes Passwort: Exit opening\n"));
				exit_oled.println("\n  ACCESS GRANTED!");
				relay.digitalWrite(REL_EXIT_PIN, !REL_EXIT_INIT);
				wdt_reset();
				delay(3000);
				relay.digitalWrite(REL_ALARM_PIN, REL_ALARM_INIT);
				wdt_reset();
				delay(1500);
				software_Reset();
			} else {
				exit_oled.println("\n  ACCESS DENIED!");
			}
			passwordReset();
		break;
		default: Serial.print(F("error, no keypad active\n")); break;
	}
}


/*============================================================================================================
//===RFID====================================================================================================
//==========================================================================================================*/

#ifndef RFID_DISABLE
	bool RFID_init() {

	    for (int i=0; i<RFID_AMOUNT; i++) {

	        delay(20);

	        Serial.print(F("initializing reader: ")); Serial.println(i);
	        RFID_READERS[i].begin();
	        RFID_READERS[i].setPassiveActivationRetries(5);

	        int retries = 0;
	        while (true) {
	            uint32_t versiondata = RFID_READERS[i].getFirmwareVersion();
	            if (!versiondata) {
	                Serial.print(F("Didn't find PN53x board\n"));
	                if (retries > 5) {
	                    Serial.print(F("PN532 startup timed out, restarting\n"));
	                    software_Reset();
	                }
	            } else {
	                Serial.print(F("Found chip PN5")); Serial.println((versiondata>>24) & 0xFF, HEX);
	                Serial.print(F("Firmware ver. ")); Serial.print((versiondata>>16) & 0xFF, DEC);
	                Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
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
		uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0 };
		uint8_t uidLength;
		uint8_t success = RFID_READERS[0].readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
		Serial.print(F("RFID ticks: ")); Serial.println(rfid_ticks);
		if (success) {
			if (rfid_ticks > 0) {
				Serial.print(F("resetting alarm\n"));
				relay.digitalWrite(REL_ALARM_PIN, REL_ALARM_INIT);
			}
			rfid_ticks = 0;
		} else {
			if (rfid_ticks == rfid_ticks_required) {
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
		light_oled.begin(&Adafruit128x64, LIGHT_OLED_ADD);
		light_oled.set400kHz();
		light_oled.setScroll(true);
		light_oled.setFont(System5x7);
		Serial.print(F("part1 done\n"));

		light_oled.clear();
		light_oled.setFont(Arial_bold_14);
		light_oled.println();
		light_oled.print("HELLO! Light");
		Serial.print(F("part2 done\n"));

		// &SH1106_128x64 &Adafruit128x64
		Serial.print(F("Oled init\n"));
		exit_oled.begin(&Adafruit128x64, EXIT_OLED_ADD);
		exit_oled.set400kHz();
		exit_oled.setScroll(true);
		exit_oled.setFont(System5x7);
		Serial.print(F("part1 done\n"));

		exit_oled.clear();
		exit_oled.setFont(Arial_bold_14);
		exit_oled.println();
		exit_oled.print("HELLO! Exit");
		Serial.print(F("part2 done\n"));
		return true;
	}

	void OLED_reset() {
		int outdated = 0;
		for (int keypad_no = 0; keypad_no < 2; keypad_no++) {
			if (millis() - update_timers[keypad_no] >= oled_reset_after) {
				outdated++;
			}
		}
		if (outdated >= 2) {
			for (int keypad_no = 0; keypad_no < 2; keypad_no++) {
				update_timers[keypad_no] = millis();
			}

			Serial.print(F("Reset! \n"));
			light_oled.clear();
			exit_oled.clear();
			if (strlen(passLight.guess) > 0) {
				Serial.println("checkpass Light \n\n");
				usedkeypad = 0;
				checkPassword();
			}
			if (strlen(passExit.guess) > 0) {
				Serial.println("checkpass Exit \n\n");
				usedkeypad = 1;
				checkPassword();
			}
			passwordReset();
		}
	}
#endif

/*


void OLED_simple_bold_text(SSD1306AsciiWire oled, char str[]) {
	oled.clear();
	oled.setFont(Adafruit5x7);
	oled.println();
	oled.println();
	oled.setFont(Arial_bold_14);
	oled.println(str);
}
*/


void setup() {
	output_init();
	Serial.println("WDT endabled");
	// wdt_enable(WDTO_8S);
	wdt_reset();

	Serial.println("!setup_begin");

	i2c_scanner();
	wdt_reset();
	
	#ifndef OLED_DISABLE
		Serial.print(F("Oleds: ..."));
		if (oled_init())	{Serial.println(" light OLED ok...");	}
	#endif

	Serial.print(F("Relay: ..."));
	if (relay_init()) { Serial.println(" ok"); }
	wdt_reset();


	#ifndef RFID_DISABLE
		Serial.print(F("RFID: ..."));
		if (RFID_init() ) {Serial.print(F(" ok\n"));	}
	#endif

	wdt_reset();

	delay(50);


	Serial.print(F("Keypad: ..."));
	if (keypad_init()) 	{Serial.print(F(" ok\n"));	}
	wdt_reset();
	delay(5);

	Serial.println(); Serial.println("===================START====================="); Serial.println();

	Serial.print(F("!setup_end\n\n"));
	delay(2000);
}

/*============================================================================================================
//===LOOP=====================================================================================================
//==========================================================================================================*/


void loop() {
	wdt_reset();
	// Serial.println("keypad_update");
	keypad_update();
	#ifndef OLED_DISABLE
		// Serial.println("oled");
		OLED_reset();
	#endif
	/*
	checkPassword();
	passwordReset();
	*/

	#ifndef RFID_DISABLE
		if (millis() - rfid_last_scan > rfid_scan_delay) {
			Serial.println("Attempting checkup");
			rfid_last_scan = millis();
			RFID_alarm_check();
			Serial.println("checkup complete");
		}
	#endif

}

void software_Reset() {
    Serial.println(F("Restarting in"));
    delay(50);
    for (byte i = 3; i>0; i--) {
        Serial.println(i);
        delay(100);
    }
    asm volatile ("  jmp 0");
}
