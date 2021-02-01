/*==========================================================================================================*/
/*		2CP - TeamEscape - Engineering
 *		by Stephan Christoph & Robert Schloezer
 *
 *		v2.0 beta
 *		- Use A0 control pin for MAX485
 *		- Modified Serial prints
 *		- Restarts automatically after correct code
 *		- Overcome PFC inputs bug
 */
/*==========================================================================================================*/

const String title = String("PFEILRAETSEL v2.0 beta");
const String brainName = String("BrArrows");


/*==INCLUDE=================================================================================================*/
// I2C Port Expander
#include "PCF8574.h"                  /* https://github.com/skywodd/pcf8574_arduino_library - modifiziert!  */
                                      /* Der Klassenname wurde geaendert, da es sonst Namenskonflikte gibt! */

// OLED
#include "SSD1306Ascii.h"             /* https://github.com/greiman/SSD1306Ascii                            */
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
// Standards der Adressierung (Konvention)
	// Relayboard und OLED
		#define RELAY_I2C_ADD     	 0x3F     /* Relay Expander																							*/
		#define OLED_I2C_ADD         0x3C     /* Ist durch Hardware des OLEDs vorgegeben				*/

// ______________________EINSTELLUNGEN______________________
// Pinbelegung
	#define EXP_MAGNET_PIN          3     /* Relay: Magnet 			 									*/
// Keypad
	#define KEYPAD_I2C_ADD       0x39     /* moeglich sind 0x38, 39, 3A, 3B, 3D                         */


/*==OLED====================================================================================================*/
SSD1306AsciiWire oled;
bool UpdateOLED = true;
unsigned long UpdateOLEDAfterDelayTimer = 0;
const int UpdateOLEDAfterDelay = 5000;         /* Zeit, bis Display kurz flackert als Online Signal			*/
String magnetString = String("ON");

/*==KEYPAD I2C==============================================================================================*/
const byte KEYPAD_ROWS = 1; 		// Zeilen
const byte KEYPAD_COLS = 4; 		// Spalten
const byte KEYPAD_CODE_LENGTH = 7;

const char KeypadKeys[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'O','U','L','R'}
};

//Konfiguration fuer Sparkfun-Keypads
// Keypad 1 2 3 4 5 6 7
// Chip   0 1 2 3 4 5 6
byte KeypadRowPins[KEYPAD_ROWS] = {4}; 							// Zeilen  - Messleitungen
byte KeypadColPins[KEYPAD_COLS] = {0, 1, 2, 3};    	// Spalten - Steuerleitungen (abwechselnd HIGH)

bool KeypadTyping = false;
bool KeypadKeyPressed = false;
bool KeypadCodeCorrect  = false;
bool KeypadCodeWrong = false;
bool LightOffice = false;				// Wenn true, Deckenlicht an -> Laengere Codeeingabe moeglich (Exit)
unsigned long KeypadCodeResetTimer = 0;       // ResetTimer
unsigned long KeypadCodeResetSpan = 5000;     // Zeit bis Codereset
unsigned long KeypadCodeWrongTimer = 0;
const int  KeypadCodeWrongDelay = 1500;          //warten, wie lang "Falsch" angezeigt wird - DIESE ZEIT BEEINFLUSST RFID_NoCardDelay!!! RFID_NoCardDelay muss größer oder gleich sein!
const int  KeypadWaitAfterCodeInput = 500;       //warten, wie lang der Code noch angezeigt wird, bis er ausgewertet wird

Keypad_I2C MyKeypad( makeKeymap(KeypadKeys), KeypadRowPins, KeypadColPins, KEYPAD_ROWS, KEYPAD_COLS, KEYPAD_I2C_ADD, PCF8574);

// Passwort
Password passLight = Password( (char*)"OUOLRRO" );          // Schaltet das Licht im Buero an

/*==PCF8574=================================================================================================*/
Expander_PCF8574 relay, LetKeypadWork;

/*==Serial Printing=================================================================================================*/
typedef enum{
	SYSTEM,
	UPDATE,
	RESULT,
}actions;
const int readPin =  A0;      // the control pin of max485 rs485 LOW read, HIGH write
char act[][10] = { "system",  "update",  "result"};


/*============================================================================================================
//===SETUP====================================================================================================
//==========================================================================================================*/

void setup() {

  Serial_Init();
  OLED_Init();
  Keypad_Init();

  oled.print(F("Expander "));
  Serial.println("Expander begin: start");
  relay.begin(RELAY_I2C_ADD);
  Serial.println("Expander begin: ok");
  for (int i=0; i<=7; i++) {
     relay.pinMode(i, OUTPUT);
     relay.digitalWrite(i, HIGH);
  }
  Serial.println("Pins auf HIGH gezogen");
	delay(2000);
  relay.digitalWrite(EXP_MAGNET_PIN, LOW);      /* Licht-Relais an, damit Licht initial aus.              NC -> Licht an   */
  Serial.println("Magnet Pin auf LOW gezogen");
  oled.println(F("ok."));

  delay(2000);
  printWithHeader("Setup Complete", act[SYSTEM]);
}

/*============================================================================================================
//===LOOP=====================================================================================================
//==========================================================================================================*/

void loop() {

  Keypad_Update();

  OLED_Update();

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
      if (i == 57) {Serial.print(F(" -> Buttons")); oled.print(" -> Buttons");}
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
	
    printWithHeader("RESTART", act[SYSTEM]);
	Serial.println("Expander Ports:");
	for (int i=0; i<=7; i++) {
		relay.pinMode(i, OUTPUT);
		relay.digitalWrite(i, HIGH);
	}
	Serial.println("HIGH");
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

void Serial_Init(){
  Serial.begin(115200);
  delay(1000);
  // initialize the read pin as an output:
  pinMode(readPin, OUTPUT);
  // turn Write mode on:
  digitalWrite(readPin, HIGH);
  Serial.println("\n");
  printWithHeader("Setup Begin", act[SYSTEM]);
  // turn Write mode off:
  digitalWrite(readPin, LOW);
}

/*============================================================================================================
//===OLED=====================================================================================================
//==========================================================================================================*/
void OLED_Init() {
  Wire.begin();

  oled.begin(&Adafruit128x64, OLED_I2C_ADD);
  oled.set400kHz();
  oled.setScroll(true);
  oled.setFont(System5x7);

  print_logo_infos(title);

  i2c_scanner();
}

void OLED_Update() {
	if ( (( (millis() - UpdateOLEDAfterDelayTimer) > UpdateOLEDAfterDelay)) && !KeypadTyping) {
		printWithHeader("refresh", act[SYSTEM]);
		UpdateOLED = true; 
		}

	if (UpdateOLED) {
		UpdateOLEDAfterDelayTimer = millis();
		UpdateOLED = false;

		if (KeypadTyping) {
			OLED_keypadscreen();
		}
		else if (KeypadCodeCorrect) {
			OLED_smileySmile();
			delay(1000);
			UpdateOLED = true;
			KeypadCodeCorrect = false;
			resetCode();
		}
		else if (KeypadCodeWrong) {
			OLED_smileySad();
			delay(1000);
			UpdateOLED = true;
			KeypadCodeWrong = false;
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
	oled.print(F("  Magnet: "));  oled.println(magnetString);
}

void OLED_keypadscreen() {		// ToDo: automatisch zentrieren
	oled.clear();
	oled.setFont(Adafruit5x7);
	oled.println();
	oled.setFont(Arial_bold_14);
	oled.println();
	oled.print(F("  "));
	oled.print(F("  "));
	for ( unsigned int i=0; i<strlen((passLight.guess))+1; i++) {
		oled.print(passLight.guess[i]);
		oled.print(F(" "));
	}

}

void OLED_smileySmile() {
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
//===KEYPAD===================================================================================================
//==========================================================================================================*/
void Keypad_Init() {
	LetKeypadWork.begin(KEYPAD_I2C_ADD);
    for (int i=0; i<=7; i++) {
     LetKeypadWork.pinMode(i, INPUT);
     LetKeypadWork.digitalWrite(i, HIGH);
  	}
  	delay(100);
	oled.print(F("Keypad "));
	MyKeypad.addEventListener(keypadEvent);    // Event Listener erstellen
	MyKeypad.begin( makeKeymap(KeypadKeys) );
	MyKeypad.setHoldTime(5000);
	MyKeypad.setDebounceTime(50);
	oled.println(F("ok."));
}

void Keypad_Update() {
  MyKeypad.getKey();
	if ( (millis() - KeypadCodeResetTimer > KeypadCodeResetSpan) && KeypadTyping) {
			printWithHeader("Timeout Reset code", act[RESULT]);
			oled.clear();
			oled.println("Resette Passwort");
			passwordReset();
    }

  if(strlen((passLight.guess))== KEYPAD_CODE_LENGTH && !LightOffice){
	  UpdateOLED = true;
	  OLED_Update();
	  delay(KeypadWaitAfterCodeInput);
	  checkPassword();
  }
}

void keypadEvent(KeypadEvent eKey){
 switch( MyKeypad.getState() ) {
 	 case PRESSED:

 		 Serial.print("Pressed: ");
 		 Serial.println(eKey);
 		 KeypadTyping = true;
 		 UpdateOLED = true;
		 KeypadCodeResetTimer = millis();

 		 switch (eKey){
			default: 
				passLight.append(eKey);
				printWithHeader(passLight.guess, act[UPDATE]);
				break;
 		 }
 		 break;

 	case HOLD:
 		Serial.print("HOLD: ");
 		Serial.println(eKey);
 		switch (eKey){
 			case 'L': //resetCode();
 			break;
    	}
    	break;

	default:
		break;
 	}
}

void printWithHeader(const char* message, char* action){
	static unsigned int count = 0;
    // turn Write mode on:
    digitalWrite(readPin, HIGH);
	Serial.println();
	Serial.print("!");
	Serial.print(brainName);
	Serial.print(": , ");
	Serial.print(count);
	Serial.print(" , ");
	Serial.print(action);
	Serial.print(" , ");
	Serial.print(message);
	Serial.println(" , Done.");
	count++;
	delay(50);
	// turn Write mode off:
  	digitalWrite(readPin, LOW);
}

void checkPassword() {
	if ( passLight.evaluate() ) {
		KeypadCodeCorrect = true;
		KeypadTyping = false;
		UpdateOLED = true;
		LightOffice = true;

		printWithHeader("Correct Passowrd", act[RESULT]);
		relay.digitalWrite(EXP_MAGNET_PIN, HIGH);
		magnetString = String( "OFF" );
		passwordReset();
	}
	else {
		KeypadCodeWrong = true;
		KeypadTyping = false;
		UpdateOLED = true;

		printWithHeader("Wrong Passowrd", act[RESULT]);
		passwordReset();
	}
}

void passwordReset() {
	KeypadTyping = false;
	UpdateOLED = true;
	passLight.reset();
}

void resetCode() {
	printWithHeader("Restart Code inserted", act[SYSTEM]);
	software_Reset();
}
