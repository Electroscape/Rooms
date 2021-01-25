/*==========================================================================================================*/
/*		2CP - TeamEscape - Engineering
 *		by Robert Schloezer
 *
 *		v1.0 beta
 *		-
 *		-
 *		-
 */
/*==========================================================================================================*/

const String title = String("KOMPASS- UND TUER-KEYPAD-RAETSEL v1.3 beta");


/*==INCLUDE=================================================================================================*/
// I2C Port Expander
#include "PCF8574.h"                  /* https://github.com/skywodd/pcf8574_arduino_library - modifiziert!  */
                                      /* Der Klassenname wurde geaendert, da es sonst Namenskonflikte gibt! */

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

/*==DEFINE==================================================================================================*/
// Standards der Pinbelegung (Konvention)
	// RFID
		#define RFID_RST_PIN           10     /* Per Konvention auf 10 festgelegt                           */
		#define RFID_1_SS_PIN           8     /* Per Konvention ist dies RFID-Port 1                        */
		#define RFID_2_SS_PIN           7     /* Per Konvention ist dies RFID-Port 2                        */
		#define RFID_3_SS_PIN           4     /* Per Konvention ist dies RFID-Port 3                        */
		#define RFID_4_SS_PIN           2     /* Per Konvention ist dies RFID-Port 4                        */
	// NeoPixel (PWM)
		#define RFID_1_LED_PIN          9     /* Per Konvention ist dies RFID-Port 1                        */
		#define RFID_2_LED_PIN          6     /* Per Konvention ist dies RFID-Port 2                        */
		#define RFID_3_LED_PIN          5     /* Per Konvention ist dies RFID-Port 3                        */
		#define RFID_4_LED_PIN          3     /* Per Konvention ist dies RFID-Port 4                        */

// Standards der Adressierung (Konvention)
	// Relayboard und OLED
		#define RELAY_I2C_ADD     	 0x3F     /* Relay Expander																							*/
		#define OLED_I2C_ADD         0x3C     /* Ist durch Hardware des OLEDs vorgegeben										*/

// ______________________EINSTELLUNGEN______________________
	// RFID
		#define RFID_NR_OF_READERS      1     /* Anzahl der angeschlossenen Reader                          */
	// NeoPixel
		#define NEOPIXEL_NR_OF_PIXELS   1     /* Anzahl der Pixel auf einem Strip				            				*/
	// Pinbelegung
		#define EXP_LICHT_PIN           0     /* Relay: Deckenlicht  																				*/
		#define EXP_LICHT_INIT					0
		#define EXP_TUER_PIN            1     /* Relay: Ausgangstuer																				*/
		#define EXP_TUER_INIT						0
		#define EXP_SCHACHT_PIN         6     /* Relay: Ausgangstuer																				*/
	// Keypad
		#define KEYPAD_I2C_ADD       0x38     /* moeglich sind 0x38, 39, 3A, 3B, 3D                         */


/*==OLED====================================================================================================*/
SSD1306AsciiWire oled;
bool UpdateOLED = true;
unsigned long UpdateOLEDAfterDelayTimer = 0;
const int UpdateOLEDAfterDelay = 5000;         /* Zeit, bis Display kurz flackert als Online Signal			*/
String lightString = String("OFF");
String doorString  = String("LOCKED");

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

const byte KeypadRowPins[KEYPAD_ROWS] = {1, 6, 5, 3}; 	// Zeilen  - Messleitungen
const byte KeypadColPins[KEYPAD_COLS] = {2, 0, 4};    	// Spalten - Steuerleitungen (abwechselnd HIGH)

bool KeypadTyping = false;
bool KeypadKeyPressed = false;
bool KeypadCodeCorrect  = false;
bool KeypadCodeWrong = false;
bool LightOffice = false;				// Wenn true, Deckenlicht an -> L�ngere Codeeingabe moeglich (Exit)
unsigned long KeypadCodeWrongTimer = 0;
unsigned long KeypadWaitAfterCodeInputTimer = 0;
const int  KeypadCodeWrongDelay = 1500;          //warten, wie lang "Falsch" angezeigt wird - DIESE ZEIT BEEINFLUSST RFID_NoCardDelay!!! RFID_NoCardDelay muss größer oder gleich sein!
const int  KeypadWaitAfterCodeInput = 500;       //warten, wie lang der Code noch angezeigt wird, bis er ausgewertet wird

Keypad_I2C MyKeypad( makeKeymap(KeypadKeys), KeypadRowPins, KeypadColPins, KEYPAD_ROWS, KEYPAD_COLS, KEYPAD_I2C_ADD, PCF8574);

// Passwort
bool passwordCheckup = false;
Password passLight = Password( "1708" );          // Schaltet das Licht im Buero an
Password passExit  = Password( "8198423" );       // Oeffnet die Ausgangstuer
Password progReset = Password( "20162016" );      // Resettet den Arduino

/*==PCF8574=================================================================================================*/
Expander_PCF8574 relay;

/*============================================================================================================
//===SETUP====================================================================================================
//==========================================================================================================*/

void setup() {

  Serial.begin(115200);

	if( OLED_Init() 	)	{Serial.println("OLED:   ok");	}
	if( Keypad_Init() ) {Serial.println("Keypad: ok");	}
	if( Relay_Init() 	)	{Serial.println("Relay:  ok");	}

	Serial.println(); Serial.println("===================START====================="); Serial.println();

  delay(2000);
}

/*============================================================================================================
//===LOOP=====================================================================================================
//==========================================================================================================*/

void loop() {

  Keypad_Update();

  OLED_Update();

	if (Serial.available()) {
        byte nr = Serial.read();
        Serial.println(nr, DEC);

        if(nr == 49){
          Serial.println("Kommando: Lichtraetsel geloest");
					KeypadCodeCorrect = true;
					KeypadTyping = false;
					UpdateOLED = true;
					LightOffice = true;

					Serial.println( "Korrektes Passwort: Licht an" );
					relay.digitalWrite(EXP_LICHT_PIN, HIGH);
					lightString = String( "ON" );
					passwordReset();
        }
        else if(nr == 50){
          Serial.println("Kommando: Exitraetsel geloest");
					KeypadCodeCorrect = true;
					KeypadTyping = false;
					UpdateOLED = true;

					Serial.println( "Korrektes Passwort: Exit auf" );
					relay.digitalWrite(EXP_TUER_PIN, HIGH);
					doorString = String( "OPEN" );
					passwordReset();
        }
				else if(nr == 51){
          Serial.println("Kommando: Relay Reset");
					relay.digitalWrite(EXP_TUER_PIN, LOW);
					relay.digitalWrite(EXP_LICHT_PIN, LOW);
				}
				else if(nr == 52){
          software_Reset();
        }
			}

}

/*============================================================================================================
//===BASICS===================================================================================================
//==========================================================================================================*/
void print_logo_infos(String progTitle) {
  Serial.println(F("+-----------------------------------+"));
  Serial.println(F("|    TeamEscape HH&S ENGINEERING    |"));
  Serial.println(F("+-----------------------------------+"));
  Serial.println();
  Serial.println(progTitle);
  Serial.println();
  oled.clear();
  oled.println();
  oled.println();
  oled.setFont(Arial_bold_14);
  oled.print(F("     T"));
  oled.setFont(Callibri11_bold);
  oled.print(F("EAM "));
  oled.setFont(Arial_bold_14);
  oled.print(F("E"));
  oled.setFont(Callibri11_bold);
  oled.println(F("SCAPE"));
  oled.setFont(Callibri11_italic);
  oled.println(F("         ENGINEERING"));
  delay(2000);
  oled.clear();
}

// I2C Scanner - scannt nach angeschlossenen I2C Geraeten
void i2c_scanner() {
  Serial.println (F("I2C scanner:"));
  Serial.println (F("Scanning..."));
  oled.setFont(System5x7);
  oled.println   (F("I2C Scan..."));
  byte wire_device_count = 0;

  for (byte i = 8; i < 120; i++) {
    Wire.beginTransmission (i);
    if (Wire.endTransmission () == 0) {
      Serial.print   (F("Found address: "));
      Serial.print   (i, DEC);
      Serial.print   (F(" (0x"));
      oled.print     (F(" (0x"));
      Serial.print   (i, HEX);
      oled.print     (i, HEX);
      Serial.print (F(")"));
      oled.print   (F(")"));
      if (i == 60) {Serial.print(F(" -> Display")); oled.print(" -> Display");}
      if (i == 63) {Serial.print(F(" -> Relay")); 	oled.print(" -> Relay");}
      Serial.println();
      oled.println();
      wire_device_count++;
      delay (1);
    }
  }
  Serial.print   (F("Found "));
  oled.print     (F("Found "));
  Serial.print   (wire_device_count, DEC);
  oled.print     (wire_device_count, DEC);
  Serial.println (F(" device(s)."));
  oled.println   (F(" device(s)."));

  Serial.println();
  oled.println();

  delay(2000);
}

// Software Reset - Startet den Arduino neu
void software_Reset() {
  Serial.println(F("Neustart in"));
  oled.clear();
  oled.println("Neustart in");
  delay(250);
  for (byte i = 3; i>0; i--) {
    Serial.println(i);
    oled.println(i);
    delay(1000);
  }
  asm volatile ("  jmp 0");
}


/*============================================================================================================
//===OLED=====================================================================================================
//==========================================================================================================*/
bool OLED_Init() {
  Wire.begin();

  oled.begin(&Adafruit128x64, OLED_I2C_ADD);
  oled.set400kHz();
  oled.setScroll(true);
  oled.setFont(System5x7);

  print_logo_infos(title);

  i2c_scanner();

	return true;
}

void OLED_Update() {
	if ( (( (millis() - UpdateOLEDAfterDelayTimer) > UpdateOLEDAfterDelay)) && !KeypadTyping) { UpdateOLED = true; Serial.println("OLED refresh"); }

	if (UpdateOLED) {
		UpdateOLEDAfterDelayTimer = millis();
		UpdateOLED = false;
		Serial.println(KeypadTyping);
		Serial.println(KeypadCodeCorrect);
		Serial.println(KeypadCodeWrong);
		Serial.println("UpdateOLED");


		if (KeypadTyping) {
			Serial.println("Keypadscreen start");
			OLED_keypadscreen();
			Serial.println("Keypadscreen ende");
		}
		else if (KeypadCodeCorrect) {
			Serial.println("KeypadCodeCorrect start");
			Serial.println("OLED_smileySmile start");
			OLED_smileySmile();
			Serial.println("OLED_smileySmile ende");
			delay(1000);
			UpdateOLED = true;
			KeypadCodeCorrect = false;
			Serial.println("KeypadCodeCorrect ende");
		}
		else if (KeypadCodeWrong) {
			Serial.println("KeypadCodeWrong start");
			Serial.println("OLED_smileySad start");
			OLED_smileySad();
			Serial.println("OLED_smileySad start");
			delay(1000);
			UpdateOLED = true;
			KeypadCodeWrong = false;
			Serial.println("KeypadCodeCorrect ende");
		}
		else { OLED_homescreen(); }
	}
}

void OLED_homescreen() {
	oled.clear();
	oled.setFont(Adafruit5x7);
	oled.println();
	oled.setFont(Arial_bold_14);
	oled.println();
	oled.print(F("  Light: "));  oled.println(lightString);
	oled.print(F("  Door : "));  oled.println(doorString);
}

void OLED_keypadscreen() {		// ToDo: automatisch zentrieren
	oled.clear();
	oled.setFont(Adafruit5x7);
	oled.println();
	oled.setFont(Arial_bold_14);
	oled.println();
	oled.print(F("  "));
	oled.print(F("  "));
	for ( int i=0; i<strlen((passLight.guess))+1; i++) {
		oled.print(passLight.guess[i]);
		oled.print(F(" "));
	}

}

void OLED_smileySmile() {
	delay(KeypadWaitAfterCodeInput);
	oled.setFont(Adafruit5x7);
	oled.clear();
	oled.println(F("        _____     "));
	oled.println(F("      .'     '.   "));
	oled.println(F("     /  o   o  \\ "));
	oled.println(F("    |           | "));
	oled.println(F("    |  \\     /  |"));
	oled.println(F("     \\  '---'  / "));
	oled.print  (F("      '._____.'   "));
}

void OLED_smileySad() {
	delay(KeypadWaitAfterCodeInput);
	oled.setFont(Adafruit5x7);
	oled.clear();
	oled.println(F("        _____     "));
	oled.println(F("      .'     '.   "));
	oled.println(F("     /  o   o  \\ "));
	oled.println(F("    |           | "));
	oled.println(F("    |    ___    | "));
	oled.println(F("     \\  /   \\  /"));
	oled.print  (F("      '._____.'   "));
}

void OLED_textCorrect() {
	oled.clear();
	oled.setFont(Adafruit5x7);
	oled.println();
	oled.setFont(Arial_bold_14);
	oled.println();
	oled.println(F("     RICHTIG  :)"));
}

void OLED_textWrong() {
	oled.clear();
	oled.setFont(Adafruit5x7);
	oled.println();
	oled.setFont(Arial_bold_14);
	oled.println();
	oled.println(F("      FALSCH :("));
}

/*============================================================================================================
//===MOTHER===================================================================================================
//==========================================================================================================*/
bool Relay_Init() {
	oled.print(F("Expander "));
  relay.begin(RELAY_I2C_ADD);
  for (int i=0; i<=7; i++) {
     relay.pinMode(i, OUTPUT);
     relay.digitalWrite(i, HIGH);
  }
	delay(1000);
	relay.digitalWrite(EXP_LICHT_PIN, EXP_LICHT_INIT);      /* Licht-Relais an, damit Licht initial aus.              NC -> Licht an   */
	Serial.print("     ");
	Serial.print("Licht Pin ["); Serial.print(EXP_LICHT_PIN); Serial.print("] auf "); Serial.print(EXP_LICHT_INIT); Serial.println(" gezogen");
	delay(20);
	relay.digitalWrite(EXP_TUER_PIN, EXP_TUER_INIT);       /* Tür-Relais an, damit Ruhestromtür verschlossen bleibt. NC -> Tuer offen */
	Serial.print("     ");
	Serial.print("Tuer  Pin ["); Serial.print(EXP_TUER_PIN); Serial.print("] auf "); Serial.print(EXP_TUER_INIT); Serial.println(" gezogen");
	oled.println(F("ok."));

	return true;
}


/*============================================================================================================
//===KEYPAD===================================================================================================
//==========================================================================================================*/
bool Keypad_Init() {
	oled.print(F("Keypad "));
	MyKeypad.addEventListener(keypadEvent);    // Event Listener erstellen
	MyKeypad.begin( makeKeymap(KeypadKeys) );
	MyKeypad.setHoldTime(5000);
	MyKeypad.setDebounceTime(20);
	oled.println(F("ok."));

	return true;
}

void Keypad_Update() {
  MyKeypad.getKey();

  if(strlen((passLight.guess))== 4 && !LightOffice && !passwordCheckup){
	  Serial.println("4 Zeichen eingebeben - ueberpruefe Passwort");
	  UpdateOLED = true;
		passwordCheckup = true;
	  // OLED_Update();
	  //delay(KeypadWaitAfterCodeInput);
	  //checkPassword();
  }
  else if((strlen(passExit.guess) == 7) && LightOffice && !passwordCheckup){
	  Serial.println("7 Zeichen eingebeben - ueberpruefe Passwort");
	  UpdateOLED = true;
		passwordCheckup = true;
	  // OLED_Update();
	  //delay(KeypadWaitAfterCodeInput);
	  //checkPassword();
  }
	else if( passwordCheckup ) {
		checkPassword();
	}
}

void keypadEvent(KeypadEvent eKey){
 switch( MyKeypad.getState() ) {
 	 case PRESSED:
 		 Serial.print("Taste: "); Serial.print(eKey); Serial.print(" -> Code: "); Serial.print(passLight.guess); Serial.println(eKey);
 		 KeypadTyping = true;
 		 UpdateOLED = true;

 		 switch (eKey){
			 case '#': Serial.println("# gedrueckt - cleared");
					   passwordReset();
					   break;
			 case '*': Serial.println("* gedrueckt - cleared");
					   passwordReset();
					   break;
			 default:  passLight.append(eKey);
					   passExit.append(eKey);
					   progReset.append(eKey);
					   break;
 		 }
 		 break;

 	case HOLD:
 		Serial.print("HOLD: ");	Serial.println(eKey);
 		switch (eKey){
 			case '*': resetCode();
 			break;
    	}
    	break;
 	}
}

void checkPassword() {
	if ( passLight.evaluate() ) {
		KeypadCodeCorrect = true;
		KeypadTyping = false;
		UpdateOLED = true;
		LightOffice = true;
		passwordCheckup = false;

		Serial.println( "Korrektes Passwort: Licht an" );
		relay.digitalWrite(EXP_LICHT_PIN, HIGH);
		lightString = String( "ON" );
		passwordReset();
	}
	else if( passExit.evaluate() ) {
		KeypadCodeCorrect = true;
		KeypadTyping = false;
		UpdateOLED = true;
		passwordCheckup = false;

		Serial.println( "Korrektes Passwort: Exit auf" );
		relay.digitalWrite(EXP_TUER_PIN, HIGH);
		doorString = String( "OPEN" );
		passwordReset();
	}
	else if( progReset.evaluate() ) {
		resetCode();
	}
	else {
		KeypadCodeWrong = true;
		KeypadTyping = false;
		UpdateOLED = true;
		passwordCheckup = false;

		Serial.println( "Falsches Passwort" );
		passwordReset();
	}
}

void passwordReset() {
	KeypadTyping = false;
	UpdateOLED = true;

	passLight.reset();
	passExit.reset();
	progReset.reset();
	Serial.println( "Passwort zurueckgesetzt" );
}

void resetCode() {
	Serial.println("Restart Code eingegeben, starte neu");
	software_Reset();
}
