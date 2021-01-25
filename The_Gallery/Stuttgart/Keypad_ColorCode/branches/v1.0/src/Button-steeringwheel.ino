/*==========================================================================================================*/
/*		2CP - TeamEscape - Engineering
 *		by Stephan Christoph & Robert Schloezer
 *
 *		v1.1 beta
 *		- Ein Keypad und ein RFID Reader
 *		- OLED Display ueber I2C
 *		- Schaltet Licht an, aktiviert Alarmanlage und oeffnet Ausgangstuer
 */
/*==========================================================================================================*/

const String title = String("KOMPASS- UND TUER-KEYPAD-RAETSEL v1.1 beta");


/*==INCLUDE=================================================================================================*/
// I2C Port Expander
#include "PCF8574.h"                  /* https://github.com/skywodd/pcf8574_arduino_library - modifiziert!  */
                                      /* Der Klassenname wurde geaendert, da es sonst Namenskonflikte gibt! */

// OLED
#include "SSD1306Ascii.h"             /* https://github.com/greiman/SSD1306Ascii                            */
#include "SSD1306AsciiWire.h"         /* https://github.com/greiman/SSD1306Ascii                            */

// NeoPixel
#include <Adafruit_NeoPixel.h>        /* Ueber Bibliotheksverwalter                                         */

// RFID
#include <SPI.h>                      /* Standardbibliothek                                                 */
#include <MFRC522.h>                  /* https://github.com/miguelbalboa/rfid                               */

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
		#define EXP_LICHT_PIN           2     /* Relay: Deckenlicht  																				*/
		#define EXP_TUER_PIN            3     /* Relay: Ausgangstuer																				*/
	// Keypad
		#define KEYPAD_I2C_ADD       0x38     /* moeglich sind 0x38, 39, 3A, 3B, 3D                         */


/*==OLED====================================================================================================*/
SSD1306AsciiWire oled;
bool UpdateOLED = true;
unsigned long UpdateOLEDAfterDelayTimer = 0;
const int UpdateOLEDAfterDelay = 5000;         /* Zeit, bis Display kurz flackert als Online Signal			*/
String lightString = String("OFF");
String doorString  = String("LOCKED");

/*==RFID====================================================================================================*/
const String RFID_KeywordCode[]            = {"SD",           "AH",           "GF",           "PA"           };

byte   RFID_SSPins[]                       = {RFID_1_SS_PIN,  RFID_2_SS_PIN,  RFID_3_SS_PIN,  RFID_4_SS_PIN  };
byte   RFID_LEDPins[]                      = {RFID_1_LED_PIN, RFID_2_LED_PIN, RFID_3_LED_PIN, RFID_4_LED_PIN };
bool   RFID_KeywordIsRight[]               = {false,          false,          false,          false          };
bool   RFID_CardIsOnReader[]               = {false,          false,          false,          false          };
bool   RFID_CardLeavedReader[]             = {false,          false,          false,          false          };
bool   RFID_Finished                       =  false;
bool   cardPresent                         =  true;

MFRC522 RFID_Reader[RFID_NR_OF_READERS];
MFRC522::MIFARE_Key RFID_Key;
MFRC522::StatusCode RFID_status;

const int RFID_NoCardDelay = 400;              /* warten, bis erkannt wird, dass die Karte weg ist          */
unsigned long RFID_NoCardTimer = 0;

/*==NEOPIXEL================================================================================================*/
Adafruit_NeoPixel LED_Stripe_1 = Adafruit_NeoPixel(NEOPIXEL_NR_OF_PIXELS, RFID_1_LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel LED_Stripe_2 = Adafruit_NeoPixel(NEOPIXEL_NR_OF_PIXELS, RFID_2_LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel LED_Stripe_3 = Adafruit_NeoPixel(NEOPIXEL_NR_OF_PIXELS, RFID_3_LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel LED_Stripe_4 = Adafruit_NeoPixel(NEOPIXEL_NR_OF_PIXELS, RFID_4_LED_PIN, NEO_GRB + NEO_KHZ800);

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

//Konfiguration fuer Sparkfun-Keypads
// Keypad 1 2 3 4 5 6 7
// Chip   0 1 2 3 4 5 6
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

  OLED_Init();

  Keypad_Init();

  //RFID_Init();

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
  relay.digitalWrite(EXP_TUER_PIN, LOW);       /* Tür-Relais an, damit Ruhestromtür verschlossen bleibt. NC -> Tuer offen */
  relay.digitalWrite(EXP_LICHT_PIN, LOW);      /* Licht-Relais an, damit Licht initial aus.              NC -> Licht an   */
  Serial.println("Tuer Pin auf LOW gezogen");
  oled.println(F("ok."));

  delay(2000);
}

/*============================================================================================================
//===LOOP=====================================================================================================
//==========================================================================================================*/

void loop() {

  Keypad_Update();

  //RFID_update();

  OLED_Update();

}

/*============================================================================================================
//===RFID=====================================================================================================
//==========================================================================================================*/
void RFID_Init() {

  // SS Pins auf HIGH ziehen, um SPI und alle Reader initialisieren zu k�nnen
  for (uint8_t reader_nr = 0; reader_nr < RFID_NR_OF_READERS; reader_nr++) {
    pinMode(RFID_SSPins[reader_nr], OUTPUT);
    digitalWrite(RFID_SSPins[reader_nr], HIGH);
    pinMode(RFID_LEDPins[reader_nr], OUTPUT);
    digitalWrite(RFID_LEDPins[reader_nr], HIGH);
  }

  SPI.begin();

  for (uint8_t reader_nr = 0; reader_nr < RFID_NR_OF_READERS; reader_nr++) {
		RFID_Reader[reader_nr].PCD_Init(RFID_SSPins[reader_nr], RFID_RST_PIN);
    Serial.print(F("Reader["));    oled.print(F("["));
    Serial.print(reader_nr + 1);   oled.print(reader_nr + 1);
    Serial.println(F("] init."));  oled.println(F("] init."));
    NeoPixel_Init(reader_nr);
  }

  Serial.println(); oled.println();

  for (uint8_t reader_nr = 0; reader_nr < RFID_NR_OF_READERS; reader_nr++) {
    Serial.print(F("Reader[")); Serial.print(reader_nr + 1); Serial.print(F("] "));
    RFID_Reader[reader_nr].PCD_DumpVersionToSerial();
    //Here we need a function which controls our Dump, because of errors while initialisation!!!!!!
    NeoPixel_StripeOn(reader_nr, RFID_KeywordCode[reader_nr]);
    delay(500);
    NeoPixel_StripeOff(reader_nr);
  }
  Serial.println();

  for (byte i = 0; i < 6; i++) {RFID_Key.keyByte[i] = 0xFF;}
}

void RFID_update() {
  if ( RFID_Reader[0].PICC_IsNewCardPresent() ) {
    RFID_NoCardTimer = millis();
    if ( !cardPresent ) {
      Serial.println("Karte aufgelegt");
      LED_Stripe_1.setPixelColor(0, LED_Stripe_1.Color(0, 255, 0)); /* GRÜN */
      LED_Stripe_1.show();
      cardPresent = true;
      // relay.digitalWrite(EXP_ALARM_PIN, HIGH); /* Alarm-Licht ausschalten */
    }
  }

  if ( ((millis() - RFID_NoCardTimer) > RFID_NoCardDelay) && cardPresent) {
    //Karte laenger weg als Delay-Time
    Serial.println("Karte weg");
    LED_Stripe_1.setPixelColor(0, LED_Stripe_1.Color(255, 0, 0)); /* ROT */
    LED_Stripe_1.show();
    cardPresent = false;
//    relay.digitalWrite(EXP_ALARM_PIN, LOW); /* Alarm-Licht einschalten */
  }
  /*else {
    //Karte ist da
    Serial.println("Karte aufgelegt");
    LED_Stripe_1.setPixelColor(0, LED_Stripe_1.Color(0, 255, 0)); // GRÜN
    LED_Stripe_1.show();
//    relay.digitalWrite(EXP_ALARM_PIN, HIGH); // Alarm-Licht ausschalten
  }*/

}


/*============================================================================================================
//===NEOPIXEL=================================================================================================
//==========================================================================================================*/
void NeoPixel_Init(byte i) {
   if (i == 0) {
    LED_Stripe_1.begin();
    NeoPixel_StripeOff(0);
  }
  if (i == 1) {
    LED_Stripe_2.begin();
    NeoPixel_StripeOff(1);
  }
  if (i == 2) {
    LED_Stripe_3.begin();
    NeoPixel_StripeOff(2);
  }
  if (i == 3) {
    LED_Stripe_4.begin();
    NeoPixel_StripeOff(3);
  }
}

void NeoPixel_StripeOn(byte i, String color_str) {
  uint32_t color;

  if (color_str == "blau") {
    color = LED_Stripe_1.Color(  0,   0, 255); //blau
  } else
  if (color_str == "rot") {
    color = LED_Stripe_1.Color(255,   0,   0); //rot
  } else
  if (color_str == "gruen") {
    color = LED_Stripe_1.Color(  0, 255,   0); //gruen
  } else
  if (color_str == "gelb") {
    color = LED_Stripe_1.Color(255, 150,   0); //gelb
  } else
  if (color_str == "gold") {
    color = LED_Stripe_1.Color(255, 70,   0); //gold
  } else
  if (color_str == "SD") {
    color = LED_Stripe_1.Color(  0,   0, 255); //blau
  } else
  if (color_str == "AH") {
    color = LED_Stripe_1.Color(255,   0,   0); //rot
  } else
  if (color_str == "GF") {
    color = LED_Stripe_1.Color(  0, 255,   0); //gruen
  } else
  if (color_str == "PA") {
    color = LED_Stripe_1.Color(255, 150,   0); //gelb
  } else
  {
    color = LED_Stripe_1.Color(  0,   0,   0); //schwarz
  }

  if (i == 0) {
    LED_Stripe_1.setPixelColor(0, color);
    LED_Stripe_1.show();
  }
  if (i == 1) {
    LED_Stripe_2.setPixelColor(0, color);
    LED_Stripe_2.show();
  }
  if (i == 2) {
    LED_Stripe_3.setPixelColor(0, color);
    LED_Stripe_3.show();
  }
  if (i == 3) {
    LED_Stripe_4.setPixelColor(0, color);
    LED_Stripe_4.show();
  }
}

void NeoPixel_StripeOff(byte i) {
  long int color_black = LED_Stripe_1.Color(  0,   0,   0);

  if (i == 0) {
    LED_Stripe_1.setPixelColor(0, color_black);
    LED_Stripe_1.show();
  }
  if (i == 1) {
    LED_Stripe_2.setPixelColor(0, color_black);
    LED_Stripe_2.show();
  }
  if (i == 2) {
    LED_Stripe_3.setPixelColor(0, color_black);
    LED_Stripe_3.show();
  }
  if (i == 3) {
    LED_Stripe_4.setPixelColor(0, color_black);
    LED_Stripe_4.show();
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
	if ( (( (millis() - UpdateOLEDAfterDelayTimer) > UpdateOLEDAfterDelay)) && !KeypadTyping) { UpdateOLED = true; }

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
	oled.print(F("Keypad "));
	MyKeypad.addEventListener(keypadEvent);    // Event Listener erstellen
	MyKeypad.begin( makeKeymap(KeypadKeys) );
	MyKeypad.setHoldTime(5000);
	MyKeypad.setDebounceTime(20);
	oled.println(F("ok."));
}

void Keypad_Update() {
  MyKeypad.getKey();

  if(strlen((passLight.guess))== 4 && !LightOffice){
	  Serial.println("4 Zeichen eingebeben - ueberpruefe Passwort");
	  UpdateOLED = true;
	  OLED_Update();
	  delay(KeypadWaitAfterCodeInput);
	  checkPassword();
  }
  else if((strlen(passExit.guess) == 7) && LightOffice){
	  Serial.println("7 Zeichen eingebeben - ueberpruefe Passwort");
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
 		 Serial.println(passLight.guess);
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
 		Serial.print("HOLD: ");
 		Serial.println(eKey);
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

		Serial.println( "Korrektes Passwort: Licht an" );
		relay.digitalWrite(EXP_LICHT_PIN, HIGH);
		lightString = String( "ON" );
		passwordReset();
	}
	else if( passExit.evaluate() ) {
		KeypadCodeCorrect = true;
		KeypadTyping = false;
		UpdateOLED = true;

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
