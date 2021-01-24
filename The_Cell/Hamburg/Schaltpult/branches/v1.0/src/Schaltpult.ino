/*==========================================================================================================*/
/*		2CP - TeamEscape - Engineering
 *		by Robert Schloezer
 *
 *		v1.0 beta
 *
 *		- Last update 10.12.2018 Abdullah
 *		-
 *		-
 */
/*==========================================================================================================*/

const String title = String("The Cell keypads v1.0 beta");


/*==INCLUDE=================================================================================================*/
// I2C Port Expander
#include "PCF8574.h"                  /* https://github.com/skywodd/pcf8574_arduino_library - modifiziert!  */
                                      /* Der Klassenname wurde geaendert, da es sonst Namenskonflikte gibt! */

// OLED
// #include "SSD1306Ascii.h"             /* https://github.com/greiman/SSD1306Ascii                            */
#include "SSD1306AsciiWire.h"         /* https://github.com/greiman/SSD1306Ascii                            */

// LCD
#include "LiquidCrystal_I2C.h"
// Keypad
#include <Wire.h>                     /* Standardbibliothek                                                 */
#include <Keypad_I2C.h>               /*                                                                    */
#include <Keypad.h>                   /* Standardbibliothek                                                 */
#include <Password.h>                 /* http://www.arduino.cc/playground/uploads/Code/Password.zip
                                         Muss modifiziert werden:
                                         Password.h -> char guess[ MAX_PASSWORD_LENGTH ];
                                         und byte currentIndex; muessen PUBLIC sein                         */

/*==DEFINE==================================================================================================*/
// Standards der Adressierung (Konvention)
	// Relayboard und OLED
		#define RELAY_I2C_ADD     	 0x3F     /* Relay Expander																							*/
		#define OLED_I2C_ADD         0x3C     /* Ist durch Hardware des OLEDs vorgegeben										*/

// ______________________EINSTELLUNGEN______________________
	// NeoPixel
		#define NEOPIXEL_NR_OF_PIXELS   1     /* Anzahl der Pixel auf einem Strip				            				*/
	// Pinbelegung
		#define EXP_LICHT_PIN           A3//17//0     /* Relay: Deckenlicht  																				*/
		#define EXP_LICHT_INIT					0
		#define EXP_TUER_PIN            A2//16//1     /* Relay: Ausgangstuer																				*/
		#define EXP_TUER_INIT						0
		#define EXP_SCHACHT_PIN         6     /* Relay: Ausgangstuer																				*/

	// Keypad
		#define KEYPAD_I2C_ADD       0x38     /* moeglich sind 0x38, 39, 3A, 3B, 3D                         */
	// LCD
		#define LCD_I2C_ADD					 0x27
 // Input button
		#define INPUT_I2C_ADD         0x39         // Fuses
		// RELAY
		// PIN
		enum REL_PIN{
		  REL_1_PIN ,                              // 0 Control box Locker
		  REL_2_PIN ,                              // 1 Cell Door Lock
		  REL_3_PIN ,                              // 2 Buzzer
		  REL_4_PIN ,                              // 3
		  REL_5_PIN ,                              // 4
		  REL_6_PIN ,                              // 5
		  REL_7_PIN ,                              // 6
		  REL_8_PIN ,                              // 7
		  REL_9_PIN ,                              // 8
		  REL_10_PIN,                              // 9
		  REL_11_PIN,                              // 10
		  REL_12_PIN,                              // 11
		  REL_13_PIN,                              // 12
		  REL_14_PIN,                              // 13
		  REL_15_PIN,                              // 14
		  REL_16_PIN                               // 15
		};
		// AMOUNT
		#define REL_AMOUNT               3

		// INIT
		enum REL_INIT{
		  REL_1_INIT   =                1,        // COM-12V_IN, NO-2*12V-OUT-lockers, NC-/
		  REL_2_INIT   =                1,        // COM-12V_IN, NO-12V_OUT_DOOR, NC-SWITCH_LIGHT/
		  REL_3_INIT   =                1,        // COM-9V_IN, NO-9V_OUT_BUZZER, NC-/
		  REL_4_INIT   =                1,        // DESCRIPTION OF THE RELAY WIRING
		  REL_5_INIT   =                1,        // DESCRIPTION OF THE RELAY WIRING
		  REL_6_INIT   =                1,        // DESCRIPTION OF THE RELAY WIRING
		  REL_7_INIT   =                1,        // DESCRIPTION OF THE RELAY WIRING
		  REL_8_INIT   =                1,        // DESCRIPTION OF THE RELAY WIRING
		  REL_9_INIT   =                1,        // DESCRIPTION OF THE RELAY WIRING
		  REL_10_INIT  =                1,        // DESCRIPTION OF THE RELAY WIRING
		  REL_11_INIT  =                1,        // DESCRIPTION OF THE RELAY WIRING
		  REL_12_INIT  =                1,        // DESCRIPTION OF THE RELAY WIRING
		  REL_13_INIT  =                1,        // DESCRIPTION OF THE RELAY WIRING
		  REL_14_INIT  =                1,        // DESCRIPTION OF THE RELAY WIRING
		  REL_15_INIT  =                1,        // DESCRIPTION OF THE RELAY WIRING
		  REL_16_INIT  =                1         // DESCRIPTION OF THE RELAY WIRING
		};
		// INPUT
enum INPUT_PINs{
			INPUT_1_PIN,        // Switch Button
			INPUT_2_PIN,
			INPUT_3_PIN,
			INPUT_4_PIN,
			INPUT_5_PIN,
			INPUT_6_PIN,
			INPUT_7_PIN,
			INPUT_8_PIN
	};
enum INPUT_PINs inputPinArray[]   = {INPUT_1_PIN, INPUT_2_PIN, INPUT_3_PIN, INPUT_4_PIN, INPUT_5_PIN, INPUT_6_PIN, INPUT_7_PIN};
		// AMOUNT
		#define INPUT_AMOUNT             1

/*==OLED====================================================================================================*/
SSD1306AsciiWire oled;
bool UpdateOLED = true;
unsigned long UpdateOLEDAfterDelayTimer = 0;
const int UpdateOLEDAfterDelay = 5000;         /* Zeit, bis Display kurz flackert als Online Signal			*/

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
bool KeypadCodeCorrect  = false;
bool KeypadCodeWrong = false;
const int  KeypadWaitAfterCodeInput = 500;       //warten, wie lang der Code noch angezeigt wird, bis er ausgewertet wird

Keypad_I2C MyKeypad( makeKeymap(KeypadKeys), KeypadRowPins, KeypadColPins, KEYPAD_ROWS, KEYPAD_COLS, KEYPAD_I2C_ADD, PCF8574);

// Passwort
bool passwordCheckup = false;
Password passDoor = Password( "4176" );          // To unlock the Terminal button
Password progReset = Password( "20162016" );      // Resettet den Arduino

/*==PCF8574=================================================================================================*/
Expander_PCF8574 relay;
Expander_PCF8574 iButton;

byte buttonState = 0;

/*==LCD=================================================================================================*/
LiquidCrystal_I2C lcd(LCD_I2C_ADD, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
int  countMiddle = 8;
int  keyCount    = countMiddle;
bool UpdateLCD = true;
unsigned long UpdateLCDAfterDelayTimer = 0;
const int UpdateLCDAfterDelay = 5000;         /* Zeit, bis Display kurz flackert als Online Signal			*/

/*==Flags=================================================================================================*/
bool  doorClosed    = true;
bool 	gameEnd				= false;

/*============================================================================================================
//===SETUP====================================================================================================
//==========================================================================================================*/

void setup() {

  Serial.begin(115200);
  Serial.println(); Serial.println("===========Schaltpult 03.01.2019=============");
	Serial.println(); Serial.println("===================SETUP====================="); Serial.println();


	if( OLED_Init() 	)	{Serial.println("OLED  :   ok");	}
	if( LCD_Init() 		)	{Serial.println("LCD	 :  ok");	}
	if( Keypad_Init() ) {Serial.println("Keypad: ok");	}
	if( Relay_Init() 	)	{Serial.println("Relay :  ok");	}
	if( input_Init()	)	{Serial.println("Inputs:  ok");	}

	delay(500);
	i2c_scanner();
	delay(500);

	Serial.println(); Serial.println("===================START====================="); Serial.println();


}

/*============================================================================================================
//===LOOP=====================================================================================================
//==========================================================================================================*/

void loop() {
		if (!doorClosed && !gameEnd){
			relay.digitalWrite( REL_1_PIN, !REL_1_INIT );
			buttonState = iButton.digitalRead(inputPinArray[0]);
			Serial.print(buttonState); Serial.println(" Control box open");
			if (buttonState && !gameEnd){
				Serial.println("Door open");
				relay.digitalWrite( REL_2_PIN, !REL_2_INIT );
				relay.digitalWrite( REL_3_PIN, !REL_3_INIT );
				LCD_openDoor();
				delay(3000);
				relay.digitalWrite( REL_3_PIN, REL_3_INIT );
				relay.digitalWrite( REL_1_PIN, REL_1_INIT );
				gameEnd = true;
			}
			delay(1000);
		}
	//	delay(200);
		Keypad_Update();
		OLED_Update();
		LCD_Update();
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
			if (i == 39) {Serial.print(F(" -> LCD")); 	oled.print(" -> LCD");}
			if (i == 56) {Serial.print(F(" -> LCD-I2C-Board")); oled.print(" -> I2C-Board");}
      if (i == 60) {Serial.print(F(" -> Display")); oled.print(" -> Display");}
      if (i == 63) {Serial.print(F(" -> Relay")); 	oled.print(" -> Relay");}
			if (i == 57) {Serial.print(F(" -> Input-I2C-board")); 	oled.print(" -> Inputs");}
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
//===LCD======================================================================================================
//==========================================================================================================*/

bool LCD_Init() {
	lcd.begin(20,4);  // 20*4 New LiquidCrystal
	lcd.setCursor(0,1);
	lcd.print("Starting...");

	for (int i =0; i<2; i++){
		delay(500);
		lcd.noBacklight();
		delay(500);
		lcd.backlight();
	}

	LCD_homescreen();

	return true;
}

void LCD_Update(){
	static unsigned int counterLCD = 0;

		if ( (( (millis() - UpdateLCDAfterDelayTimer) > UpdateLCDAfterDelay)) && !KeypadTyping) { UpdateLCD = true; Serial.println("LCD refresh"); }

		if (UpdateLCD && doorClosed) {
			LCD_homescreen();
			UpdateLCDAfterDelayTimer = millis();
			UpdateLCD = false;
	//		Serial.println(KeypadTyping);
	//		Serial.println(KeypadCodeCorrect);
	//		Serial.println(KeypadCodeWrong);
			Serial.print("UpdateLCD "); Serial.println(counterLCD);

			if (KeypadTyping) {
				Serial.println("LCD Keypadscreen start");
				LCD_keypadscreen();
				Serial.println("LCD Keypadscreen ende");
			}
			else if (KeypadCodeCorrect) {
				Serial.println("LCD KeypadCodeCorrect start");
				LCD_correct();
				KeypadCodeCorrect = false;
				Serial.println("LCD KeypadCodeCorrect ende");
			}
			else if (KeypadCodeWrong) {
				Serial.println("LCD KeypadCodeWrong start");
				LCD_wrong();
				UpdateLCD = true;
				KeypadCodeWrong = false;
				Serial.println("LCD KeypadCodeCorrect ende");
			}
			else { LCD_homescreen(); }
			counterLCD++;
		}
}

void LCD_homescreen() {
	lcd.clear();
	lcd.home();
	lcd.setCursor(4,0);
	lcd.print("Enter  Code");
	lcd.setCursor(0,3);
	lcd.print("* - Clear");
}

void LCD_correct(){
	lcd.clear();
	lcd.setCursor(7,1);
	lcd.print("ACCESS");
	lcd.setCursor(6,2);
	lcd.print("GRANTED");
	delay(1000);
	doorClosed = false;
}

void LCD_openDoor(){
	lcd.clear();
	lcd.setCursor(5,1);
	lcd.print("Cell Door");
	lcd.setCursor(7,2);
	lcd.print("OPEN");
	delay(1000);
}

void LCD_wrong(){
	lcd.clear();
	lcd.setCursor(7,1);
	lcd.print("ACCESS");
	lcd.setCursor(7,2);
	lcd.print("DENIED");
	delay(1000);
}

void LCD_keypadscreen(){
 if(doorClosed) {
	 for ( int i=0; i<strlen((passDoor.guess)); i++) {
		 	lcd.setCursor(8+i,1); lcd.print(passDoor.guess[i]);
			if (i == 11) {break;}
		}
	}
}

/*==INPUTS==================================================================================================*/
bool input_Init() {
  iButton.begin(INPUT_I2C_ADD);
  iButton.pinMode(inputPinArray[0], INPUT);
	delay(500);
	buttonState = iButton.digitalRead(inputPinArray[0]);
  return true;
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

	return true;
}

void OLED_Update() {
	static unsigned int counter = 0;

	if ( (( (millis() - UpdateOLEDAfterDelayTimer) > UpdateOLEDAfterDelay)) && !KeypadTyping) { UpdateOLED = true; Serial.println("OLED refresh"); }

	if (UpdateOLED) {
		UpdateOLEDAfterDelayTimer = millis();
		UpdateOLED = false;
//		Serial.println(KeypadTyping);
//		Serial.println(KeypadCodeCorrect);
//		Serial.println(KeypadCodeWrong);
		Serial.print("UpdateOLED "); Serial.println(counter);

		if (KeypadTyping) {
			//Serial.println("Keypadscreen start");
			OLED_keypadscreen();
			//Serial.println("Keypadscreen ende");
		}
		else if (KeypadCodeCorrect) {
			//Serial.println("KeypadCodeCorrect start");
			//Serial.println("OLED_smileySmile start");
			OLED_smileySmile();
			LCD_correct();
			//Serial.println("OLED_smileySmile ende");
			UpdateOLED = true;
			KeypadCodeCorrect = false;
			//Serial.println("KeypadCodeCorrect ende");
		}
		else if (KeypadCodeWrong) {
			//Serial.println("KeypadCodeWrong start");
			//Serial.println("OLED_smileySad start");
			OLED_smileySad();
			LCD_wrong();
			UpdateOLED = true;
			KeypadCodeWrong = false;
			//Serial.println("KeypadCodeCorrect ende");
		}
		else { OLED_homescreen();}
		counter++;
	}
}

void OLED_homescreen() {
	oled.clear();
	oled.setFont(Adafruit5x7);
	oled.println();
	oled.setFont(Arial_bold_14);
	oled.println();
	oled.print(F(" PIN: "));
}

void OLED_keypadscreen() {		// ToDo: automatisch zentrieren
	oled.clear();
	oled.setFont(Adafruit5x7);
	oled.println();
	oled.setFont(Arial_bold_14);
	oled.println();
	oled.print(F(" PIN: "));
	for ( int i=0; i<strlen((passDoor.guess))+1; i++) {
		oled.print(passDoor.guess[i]);
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
	// relay.digitalWrite(EXP_LICHT_PIN, EXP_LICHT_INIT);      /* Licht-Relais an, damit Licht initial aus.              NC -> Licht an   */
	// //pinMode(EXP_LICHT_PIN, OUTPUT);
	// //pinMode(EXP_TUER_PIN, OUTPUT);
	// //digitalWrite(EXP_LICHT_PIN, EXP_LICHT_INIT);
	// Serial.print("     ");
	// Serial.print("Licht Pin ["); Serial.print(EXP_LICHT_PIN); Serial.print("] auf "); Serial.print(EXP_LICHT_INIT); Serial.println(" gezogen");
	// delay(20);
	// relay.digitalWrite(EXP_TUER_PIN, EXP_TUER_INIT);       /* Tür-Relais an, damit Ruhestromtür verschlossen bleibt. NC -> Tuer offen */
	// //digitalWrite(EXP_TUER_PIN, EXP_TUER_INIT);
	// Serial.print("     ");
	// Serial.print("Tuer  Pin ["); Serial.print(EXP_TUER_PIN); Serial.print("] auf "); Serial.print(EXP_TUER_INIT); Serial.println(" gezogen");
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

	if(strlen((passDoor.guess))== 4 && doorClosed){
		Serial.println("4 Zeichen eingebeben - ueberpruefe Passwort");
    keyCount = countMiddle;
		UpdateOLED = true;
		UpdateLCD = true;
		passwordCheckup = true;
		OLED_Update();
		LCD_Update();
		checkPassword();
		Serial.println("Check password Done");
	} else if (strlen((passDoor.guess))== 8){
		passwordCheckup = true;
	}

  if(passwordCheckup){
		checkPassword();}
}

void keypadEvent(KeypadEvent eKey){
	Serial.println("Event Happened");
 switch( MyKeypad.getState() ) {
 	 case PRESSED:
 		 Serial.print("Taste: "); Serial.print(eKey); Serial.print(" -> Code: "); Serial.print(passDoor.guess); Serial.println(eKey);
 		 KeypadTyping = true;
 		 UpdateOLED = true;
		 UpdateLCD = true;

 		 switch (eKey){
			 case '#': Serial.println("Hash Not Used");
					  break;
			 case '*': Serial.println("* - cleared");
			 			passwordReset();
						if (doorClosed){
							keyCount = countMiddle;
							lcd.clear();
	 						lcd.setCursor(6,1);
	 						lcd.print("CLEARED");
	 						delay(750);
	 						LCD_homescreen();
						}
					  break;
			 default:  passDoor.append(eKey);
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
	if ( passDoor.evaluate() ) {
		KeypadCodeCorrect = true;
		KeypadTyping = false;
		UpdateOLED = true;
		UpdateLCD = true;
		passwordCheckup = false;

		Serial.println( "Korrektes Passwort: Door Open" );
//	relay.digitalWrite(EXP_LICHT_PIN, !EXP_LICHT_INIT);

		passwordReset();
	}
	else if( progReset.evaluate() ) {
		resetCode();
	}
	else {
		KeypadCodeWrong = true;
		KeypadTyping = false;
		UpdateOLED = true;
		UpdateLCD = true;
		passwordCheckup = false;

		Serial.println( "Falsches Passwort" );
		passwordReset();
	}
}

void passwordReset() {
	KeypadTyping = false;
	UpdateOLED = true;
	UpdateLCD = true;

	passDoor.reset();
	progReset.reset();
	Serial.println( "Passwort zurueckgesetzt" );
}

void resetCode() {
	Serial.println("Restart Code eingegeben, starte neu");
	software_Reset();
}
