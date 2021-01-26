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

String title = "Light & Exit & RFID S v0.1";
String versionDate = "02.07.2020";
String version = "version 1.0";

// #include <Arduino.h>
//Watchdog timer
#include <avr/wdt.h>
/*==INCLUDE=================================================================================================*/
// I2C Port Expander
	#include "PCF8574.h"                  /* https://github.com/skywodd/pcf8574_arduino_library - modifiziert!  */
                                      /* Der Klassenname wurde geaendert, da es sonst Namenskonflikte gibt! */
// RFID
	#include <Adafruit_PN532.h>
// OLED
// #include "SSD1306Ascii.h"             /* https://github.com/greiman/SSD1306Ascii                            */
	#include "SSD1306AsciiWire.h"         /* https://github.com/greiman/SSD1306Ascii                            */

// Keypad
	#include <Wire.h>                     /* Standardbibliothek                                                 */
	#include <Keypad.h>                   /* Standardbibliothek                                                 */
	#include <Keypad_I2C.h>               /*                                                                    */
	#include <Password.h>                 /* http://www.arduino.cc/playground/uploads/Code/Password.zip
	                                         Muss modifiziert werden:
	                                         Password.h -> char guess[ MAX_PASSWORD_LENGTH ];
	                                         und byte currentIndex; muessen PUBLIC sein                         */

#define DEBUG		1

// Standards der Adressierung (Konvention)
	// Relayboard und OLED
		#define RELAY_I2C_ADD     	 0x3F     /* Relay Expander																							*/
		#define LIGHT_OLED_ADD       0x3C     /* Ist durch Hardware des OLEDs 0x3C									*/
		#define OLED_EXIT_ADD        0x3D     /* Ist durch Hardware des OLEDs 									*/

// ______________________EINSTELLUNGEN______________________
	// RFID
		#define RFID_1_SS_PIN           8     /* Per Konvention ist dies RFID-Port 1                                */
		#define RFID_2_SS_PIN           7     /* Per Konvention ist dies RFID-Port 2                                */
		#define RFID_3_SS_PIN           4     /* Per Konvention ist dies RFID-Port 3                                */
		#define RFID_4_SS_PIN           2     /* Per Konvention ist dies RFID-Port 4                                */

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


	// Keypad Adresses
		#define LIGHT_KEYPAD_ADD       	0x38     /* moeglich sind 0x38, 39, 3A, 3B, 3D                         */
		#define EXIT_KEYPAD_ADD			0x39     /* moeglich sind 0x38, 39, 3A, 3B, 3D                         */


/*==OLED====================================================================================================*/
SSD1306AsciiWire light_oled;
SSD1306AsciiWire exit_oled;
SSD1306AsciiWire oleds[] = {light_oled, exit_oled};

const int oled_reset_after = 5000;         /* Zeit, bis Display kurz flackert als Online Signal			*/


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

static int update_timers[] = {0, 0};

Keypad_I2C LightKeypad( makeKeymap(KeypadKeys), KeypadRowPins, KeypadColPins, KEYPAD_ROWS, KEYPAD_COLS, LIGHT_KEYPAD_ADD, PCF8574);
Keypad_I2C ExitKeypad( makeKeymap(KeypadKeys), KeypadRowPins, KeypadColPins, KEYPAD_ROWS, KEYPAD_COLS, EXIT_KEYPAD_ADD, PCF8574);
static int usedkeypad = -1;

// Passwort
bool light_finished = false;
bool exit_finished = false;
Password passLight = Password( "1708" );          // Schaltet das Licht im Buero an
Password passExit  = Password( "2381984" );       // Oeffnet die Ausgangstuer

/*==PCF8574=================================================================================================*/
Expander_PCF8574 relay;

/*============================================================================================================
//===BASICS===================================================================================================
//==========================================================================================================*/

void dbg_println(String print_dbg) {
	#if DEBUG
		Serial.println(print_dbg);
	#endif
}

void print_logo_infos(String progTitle) {
    Serial.println(F("+-----------------------------------+"));
    Serial.println(F("|    TeamEscape HH&S ENGINEERING    |"));
    Serial.println(F("+-----------------------------------+"));
    Serial.println();
    Serial.println(progTitle);
    Serial.println();
    delay(200);
}

void print_serial_header() {
    Serial.begin(115200);
    print_logo_infos(title);

    Serial.println("!header_begin");
    Serial.println(title);
    Serial.println(versionDate);
    Serial.println(version);
}


void output_init() {
    Wire.begin();
    Serial.begin(115200);
    print_serial_header();
}



/*============================================================================================================
//===MOTHER===================================================================================================
//==========================================================================================================*/

bool i2c_scanner() {
	Serial.println();
    Serial.println (F("I2C scanner:"));
    Serial.println (F("Scanning..."));
	Serial.println();
    byte wire_device_count = 0;

    for (byte i = 8; i < 120; i++) {

        Wire.beginTransmission (i);
        if (Wire.endTransmission () == 0) {
            Serial.print   (F("Found address: "));
            Serial.print   (i, DEC);
            Serial.print   (F(" (0x"));
            Serial.print   (i, HEX);
            Serial.print (F(")"));
			switch (i) {
				case 39: Serial.print(F(" -> LCD")); break;
				case 56: Serial.print(F(" -> I2C-Board")); break;
				case 60: Serial.print(F(" -> Display")); break;
				case 63: Serial.print(F(" -> Relay")); break;
				case 22: Serial.print(F(" -> Servo-I2C-Board")); break;
				default: Serial.print(F(" -> Unkown")); break;
			}

            Serial.println();
            wire_device_count++;
            delay (5);
        }
    }
    Serial.print   (F("Found "));
    Serial.print   (wire_device_count, DEC);
    Serial.println (F(" device(s)."));
    Serial.println("successfully scanned I2C");
    Serial.println();

    return true;
}

bool relay_init() {
    Serial.println("initializing relay");
    relay.begin(RELAY_I2C_ADD);
    delay(100);
    for (int i=0; i<REL_AMOUNT; i++) {
        relay.pinMode(i, OUTPUT);
        relay.digitalWrite(i, HIGH);
        delay(100);
    }

    delay(500);

    for (int i=0; i<REL_AMOUNT; i++) {
	    relay.digitalWrite(relayPinArray[i], relayInitArray[i]);
	    Serial.print("     ");
	    Serial.print("Relay ["); Serial.print(relayPinArray[i]); Serial.print("] set to "); Serial.println(relayInitArray[i]);
	    delay(20);
    }

    Serial.println();

    Serial.println("successfully initialized relay");
    return true;
}


/*============================================================================================================
//===KEYPAD===================================================================================================
//==========================================================================================================*/

void keypadEvent(KeypadEvent eKey){

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
			Serial.print("Taste: "); Serial.print(eKey);
			Serial.print(" -> Code: "); Serial.print(passLight.guess); Serial.println(eKey);
			update_timers[usedkeypad] = millis();

			switch (eKey) {
				case '#':
					checkPassword();
				break;
				case '*':
					passwordReset();
				break;

				default:
					switch (usedkeypad) {
						case 0:
							passLight.append(eKey);
							// OLED_simple_bold_text(light_oled, passLight.guess);
						break;
						case 1:
							passExit.append(eKey);
							// OLED_simple_bold_text(exit_oled, passExit.guess);
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
	ExitKeypad.getKey();
	delay(2);
	LightKeypad.getKey();
}

void passwordReset() {
	Serial.print("Resetting Password for: ");
	switch (usedkeypad) {
		case 0:
			passLight.reset(); Serial.println("Light");
		break;
		case 1:
			passExit.reset(); Serial.println("Exit");
		break;
	}
}

void checkPassword() {
	switch (usedkeypad) {
		Serial.print("Checking password entry for: ");
		case 0:
			Serial.print("Light");
			if (passLight.evaluate()) {
				Serial.println( "Korrektes Passwort: Licht an" );
				// OLED_simple_bold_text(light_oled, "ACCESS GRANTED!");
				relay.digitalWrite(REL_LICHT_PIN, !REL_LICHT_INIT);
			} else {
				// OLED_simple_bold_text(light_oled, "ACCESS DENIED!");
				passwordReset();
			}

		break;
		case 1:
			Serial.print("Exit");
			if (passExit.evaluate()) {
				Serial.println( "Korrektes Passwort: Exit opening" );
				relay.digitalWrite(REL_EXIT_PIN, !REL_EXIT_INIT);
				delay(3000);
				relay.digitalWrite(REL_ALARM_PIN, REL_ALARM_INIT);
				wdt_reset();
				delay(1500);
				software_Reset();
				// OLED_simple_bold_text(exit_oled, "ACCESS GRANTED!");
			} else {
				// OLED_simple_bold_text(exit_oled, "ACCESS DENIED!");
				passwordReset();
			}
		break;
		default: Serial.println("error, no keypad active"); break;
	}
}


/*============================================================================================================
//===RFID====================================================================================================
//==========================================================================================================*/

bool RFID_init() {

    for (int i=0; i<RFID_AMOUNT; i++) {

        delay(20);

        Serial.print("initializing reader: "); Serial.println(i);
        RFID_READERS[i].begin();
        RFID_READERS[i].setPassiveActivationRetries(10);

        int retries = 0;
        while (true) {
            uint32_t versiondata = RFID_READERS[i].getFirmwareVersion();
            if (!versiondata) {
                Serial.println("Didn't find PN53x board");
                if (retries > 5) {
                    Serial.println("PN532 startup timed out, restarting");
                    software_Reset();
                }
            } else {
                Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
                Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
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
	Serial.print("RFID ticks: "); Serial.println(rfid_ticks);
	if (success) {
		if (rfid_ticks > 0) {
			Serial.println("resetting alarm");
			relay.digitalWrite(REL_ALARM_PIN, REL_ALARM_INIT);
		}
		rfid_ticks = 0;
	} else {
		if (rfid_ticks == rfid_ticks_required) {
			Serial.println("compass removed, activating alarm");
			relay.digitalWrite(REL_ALARM_PIN, !REL_ALARM_INIT);
			rfid_ticks++;
		} else if (rfid_ticks < rfid_ticks_required + 1) {
			rfid_ticks++;
		}
	}
}


/*============================================================================================================
//===OLED=====================================================================================================
//==========================================================================================================*/

bool oled_init(SSD1306AsciiWire oled, uint8_t oled_add) {
	Serial.println();
	Serial.print("tryint to init oled with add:");
	Serial.println(oled_add);
	oled.begin(&Adafruit128x64, oled_add);
	oled.set400kHz();
	oled.setScroll(true);
	oled.setFont(System5x7);

	oled.clear();
	oled.setFont(Arial_bold_14);
	oled.println();
	oled.print("HELLO!"); oled.println(oled_add);
	return true;
}

// ToDo: automatisch zentrieren
void OLED_keypadscreen(SSD1306AsciiWire oled, char str[]) {
	oled.clear();
	oled.setFont(Adafruit5x7);
	oled.println();
	oled.setFont(Arial_bold_14);
	oled.println();
	oled.print(F("  "));
	oled.print(F("  "));
	for ( int i=0; i<sizeof(str); i++) {
		oled.print(str[i]);
		oled.print(F(" "));
	}
}

void OLED_simple_bold_text(SSD1306AsciiWire oled, char str[]) {
	oled.clear();
	oled.setFont(Adafruit5x7);
	oled.println();
	oled.println();
	oled.setFont(Arial_bold_14);
	oled.println(str);
}

void OLED_reset() {
	for (int i=0; i<sizeof(update_timers); i++) {
		if (millis() - update_timers[i] >= oled_reset_after) {
			oleds[i].clear();
			oleds[i].setFont(Adafruit5x7);
			oleds[i].println();
			oleds[i].setFont(Arial_bold_14);
			oleds[i].println("# to confirm");
			oleds[i].println("* to reset");
			update_timers[i] = millis();
		}
	}

}


void setup() {

	wdt_disable();  //disable previous watchdog
	Serial.println("!header_begin");
	Serial.println("gal_A_stresstest");
	Serial.println("!header_end");
	Serial.println("WDT endabled");
	wdt_enable(WDTO_8S);
	Wire.begin();
	Serial.begin(115200);
	wdt_reset();

	Serial.println("!setup_begin");
	i2c_scanner();
	Serial.print("Relay: ...");
	if (relay_init()) { Serial.println(" ok"); }
	wdt_reset();

	Serial.print("Keypad: ...");
	if (keypad_init()) 	{Serial.println(" ok");	}
	wdt_reset();

	Serial.print("Oleds: ...");
	if (oled_init(light_oled, LIGHT_OLED_ADD))	{Serial.println(" light OLED ok...");	}
	delay(2000);
	if (oled_init(exit_oled, OLED_EXIT_ADD))	{Serial.println(" exit OLED ok...");	}
	delay(2000);
	
	wdt_reset();
	Serial.print("RFID: ...");
	if (RFID_init() ) {Serial.println(" ok");	}
	wdt_reset();

	Serial.println(); Serial.println("===================START====================="); Serial.println();

	Serial.println("!setup_end");
	delay(2000);
}

/*============================================================================================================
//===LOOP=====================================================================================================
//==========================================================================================================*/

void stress() {
	Serial.println("stressing relays");
	for (int i=0; i<REL_AMOUNT; i++) {
		if (rand() && i != REL_ALARM_PIN) {
			relay.digitalWrite(i, rand());
		}
	}
}


void loop() {
	wdt_reset();
	//Serial.println("keypad_update");
	keypad_update();
	//Serial.println("oled");
	//OLED_reset();
	//Serial.println("RFID");
	//RFID_alarm_check();
	stress();

	if (millis() - rfid_last_scan > rfid_scan_delay) {
		rfid_last_scan = millis();
		RFID_alarm_check();
		//Serial.println("checkup");
	}
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
