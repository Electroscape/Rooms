/*==========================================================================================================*/
/*		2CP - TeamEscape - Engineering
 *		by Robert Schloezer
 *
 *		v1.0 beta
 *
 *		- Last update 26.12.2018 Abdullah
 *		-
 *		-
 */
/*==========================================================================================================*/

const String title = String("The Cell keypads v1.0 beta");


/*==INCLUDE=================================================================================================*/
// LCD
#include "LiquidCrystal_I2C.h"
// Keypad
#include <Keypad_I2C.h>               /*                                                                    */
#include <Keypad.h>                   /* Standardbibliothek                                                 */
#include <Password.h>                 /* http://www.arduino.cc/playground/uploads/Code/Password.zip
                                         Muss modifiziert werden:
                                         Password.h -> char guess[ MAX_PASSWORD_LENGTH ];
                                         und byte currentIndex; muessen PUBLIC sein                         */
//STB Library
#include <StandardBox.h>

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

Keypad_I2C MyKeypad( makeKeymap(KeypadKeys), KeypadRowPins, KeypadColPins, KEYPAD_ROWS, KEYPAD_COLS, KEYPAD_I2C_ADD, PCF8574);

// Passwort
bool passwordCheckup = false;
Password passDoor = Password( "4176" );          // To unlock the Terminal button
Password progReset = Password( "20162016" );      // Resettet den Arduino

/*==PCF8574=================================================================================================*/
STB_Board exp1;
InputPin switchBtn;

STB_Board exp2;
OutputPin relayLocker;
OutputPin relayDoor;
OutputPin relayBuzzer;

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
bool	buttonState 	= false;
/*============================================================================================================
//===SETUP====================================================================================================
//==========================================================================================================*/

void setup() {

	Serial.begin(115200);

	Serial.println(); Serial.println("===========Schaltpult 21.12.2019=============");
	Serial.println(); Serial.println("===================SETUP====================="); Serial.println();

	if( LCD_Init() 		)	{Serial.println("LCD	 :  ok");	}
	if( Keypad_Init() ) {Serial.println("Keypad: ok");	}
	if( Relay_Init() 	)	{Serial.println("Relay :  ok");	}
	if( input_Init()	)	{Serial.println("Inputs:  ok");	}

	i2c_scanner();

	Serial.println(); Serial.println("===================START====================="); Serial.println();

	delay(2000);
}

/*============================================================================================================
//===LOOP=====================================================================================================
//==========================================================================================================*/

void loop() {
	if (!doorClosed && !gameEnd){
		relayLocker.on();
		//digitalWrite( REL_1_PIN, !REL_1_INIT );
		buttonState = switchBtn.read();
		Serial.print(buttonState); Serial.println(" Control box open");
		if (buttonState && !gameEnd){
			Serial.println("Door open");
			relayDoor.off();
			relayBuzzer.on();
//				relay.digitalWrite( REL_2_PIN, !REL_2_INIT );
//				relay.digitalWrite( REL_3_PIN, !REL_3_INIT );
			LCD_openDoor();
			delay(3000);
			relayBuzzer.off();
//				relay.digitalWrite( REL_3_PIN, REL_3_INIT );
//				relay.digitalWrite( REL_1_PIN, REL_1_INIT );
			relayLocker.off();
			gameEnd = true;
		}
		delay(1000);
	}
	delay(200);
	Keypad_Update();
	LCD_Update();
}

/*============================================================================================================
//===Functions===================================================================================================
//==========================================================================================================*/

// I2C Scanner - scannt nach angeschlossenen I2C Geraeten
void i2c_scanner() {
  Serial.println (F("I2C scanner:"));
  Serial.println (F("Scanning..."));
  byte wire_device_count = 0;

  for (byte i = 8; i < 120; i++) {
    Wire.beginTransmission (i);
    if (Wire.endTransmission () == 0) {
      Serial.print   (F("Found address: "));
      Serial.print   (i, DEC);
      Serial.print   (F(" (0x"));
      Serial.print   (i, HEX);
      Serial.print (F(")"));
			if (i == 39) {Serial.print(F(" -> LCD")); }
			if (i == 56) {Serial.print(F(" -> LCD-I2C-Board"));}
      if (i == 60) {Serial.print(F(" -> Display"));}
      if (i == 63) {Serial.print(F(" -> Relay")); }
			if (i == 57) {Serial.print(F(" -> Input-I2C-board")); }
      Serial.println();
      wire_device_count++;
      delay (1);
    }
  }
  Serial.print   (F("Found "));
  Serial.print   (wire_device_count, DEC);
  Serial.println (F(" device(s)."));

  Serial.println();

  delay(2000);
}

// Software Reset - Startet den Arduino neu
void software_Reset() {
  Serial.println(F("Neustart in"));
  delay(250);
  for (byte i = 3; i>0; i--) {
    Serial.println(i);
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
	exp1.init(INPUT_I2C_ADD);
	switchBtn.init(INPUT_1_PIN, exp1.getExpanderNumber());

	delay(500);
	buttonState = switchBtn.read();
  return true;
}

/*============================================================================================================
//===MOTHER===================================================================================================
//==========================================================================================================*/
bool Relay_Init() {

	exp2.init(RELAY_I2C_ADD);
	delay(1000);
	relayLocker.init(REL_1_PIN, exp2.getExpanderNumber(),OFF);
	relayDoor.init(REL_2_PIN, exp2.getExpanderNumber(),ON);
	relayBuzzer.init(REL_3_PIN, exp2.getExpanderNumber(),OFF);

	return true;
}


/*============================================================================================================
//===KEYPAD===================================================================================================
//==========================================================================================================*/
bool Keypad_Init() {
	MyKeypad.addEventListener(keypadEvent);    // Event Listener erstellen
	MyKeypad.begin( makeKeymap(KeypadKeys) );
	MyKeypad.setHoldTime(5000);
	MyKeypad.setDebounceTime(20);

	return true;
}

void Keypad_Update() {
  MyKeypad.getKey();

	if(strlen((passDoor.guess))== 4 && doorClosed){
		Serial.println("4 Zeichen eingebeben - ueberpruefe Passwort");
    keyCount = countMiddle;
		UpdateLCD = true;
		passwordCheckup = true;
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
//	KeypadTyping = true;
//	UpdateLCD = true;
 switch( MyKeypad.getState() ) {
 	 case PRESSED:
 		 Serial.print("Taste: "); Serial.print(eKey); Serial.print(" -> Code: "); Serial.print(passDoor.guess); Serial.println(eKey);
 		 KeypadTyping = true;
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
	 						delay(1000);
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
		UpdateLCD = true;
		passwordCheckup = false;
		Serial.println( "Korrektes Passwort: Door Open" );
		passwordReset();
	}
	else if( progReset.evaluate() ) {
		resetCode();
	}
	else {
		KeypadCodeWrong = true;
		KeypadTyping = false;
		UpdateLCD = true;
		passwordCheckup = false;

		Serial.println( "Falsches Passwort" );
		passwordReset();
	}
}

void passwordReset() {
	KeypadTyping = false;
	UpdateLCD = true;

	passDoor.reset();
	progReset.reset();
	Serial.println( "Passwort zurueckgesetzt" );
}

void resetCode() {
	Serial.println("Restart Code eingegeben, starte neu");
	software_Reset();
}
