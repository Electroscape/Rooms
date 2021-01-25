/*==========================================================================================================*/
/*		2CP - TeamEscape - Engineering
 *		by Abdullah Saei & Robert Schloezer
 *
 *		v1.0
 *		-
 *		-
 *		-
 */
/*==========================================================================================================*/

const String title = String("Telephone v1.0");

/*==INCLUDE=================================================================================================*/
#include <Wire.h>                     /* TWI / I2C                                                          */
// I2C Port Expander
#include "PCF8574.h"
// LCD
#include "LiquidCrystal_I2C.h"
// Keypad
#include <Keypad.h>                   /* Standardbibliothek                                                 */
#include <Keypad_I2C.h>               /*                                                                    */
#include <Password.h>                 /* http://www.arduino.cc/playground/uploads/Code/Password.zip
                                         Muss modifiziert werden:
                                         Password.h -> char guess[ MAX_PASSWORD_LENGTH ];
                                         und byte currentIndex; muessen PUBLIC sein                         */


// AUDIO
#include "AltSoftSerial.h"
//#include "NeoSWSerial.h"
#include "DFRobotDFPlayerMini.h"
//Servo
#include <Servo_I2C.h> //
//#include <Servo.h> //pins connected to Attiny PIN D

/*==DEFINE==================================================================================================*/
// LED
// PIN
enum PWM_PIN{
  PWM_1_PIN = 3,                           // Predefined by STB design
  PWM_2_PIN = 5,                           // Predefined by STB design
  PWM_3_PIN = 6,                           // Predefined by STB design
  PWM_4_PIN = 9,                           // Predefined by STB design
  PWM_5_PIN = 12                           // Servo
};

// SETTINGS
#define LED_STRIP WS2812B                 // Type of LED Strip, predefined by STB design
#define MAX_DIMENSION ((kMatrixWidth>kMatrixHeight) ? kMatrixWidth : kMatrixHeight) // if w > h -> MAX_DIMENSION=kMatrixWidth, else MAX_DIMENSION=kMatrixHeight
#define NUM_LEDS (kMatrixWidth * kMatrixHeight)

// I2C ADRESSES
#define RELAY_I2C_ADD     	   0x3F         // Relay Expander
//#define OLED_I2C_ADD         0x3C         // Predefined by hardware
#define LCD_I2C_ADD					  0x27         // Predefined by hardware
#define KEYPAD_I2C_ADD        0x38         // Telephone Keypad
#define INPUT_I2C_ADD         0x39         // coin detector and ?!
#define SERVO_I2C_ADD         0x16         // From Attiny code


// RELAY
// PIN
enum REL_PIN{
  REL_1_PIN ,                              // 0 Door Opener
  REL_2_PIN ,                              // 1 Buzzer
  REL_3_PIN ,                              // 2
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
#define REL_AMOUNT               2

// INIT
enum REL_INIT{
  REL_1_INIT   =                0,        // COM-12V_IN, NO-12V_OUT, NC-/
  REL_2_INIT   =                1,        // COM-12V_IN, NO-12V_OUT_DOOR, NC-12V_OUT_ALARM
  REL_3_INIT   =                1,        // DESCRIPTION OF THE RELAY WIRING
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
enum INPUT_PIN{
  INPUT_1_PIN,                             //  0 Coin Detector
  INPUT_2_PIN,                             //  1 trigger for starting the procedure
  INPUT_3_PIN,                             //  2
  INPUT_4_PIN,                             //  3
  INPUT_5_PIN,                             //  4
  INPUT_6_PIN,                             //  5
  INPUT_7_PIN,                             //  6
  INPUT_8_PIN
};
// AMOUNT
#define INPUT_AMOUNT             1

/*==CONSTANT VARIABLES======================================================================================*/
const enum REL_PIN relayPinArray[]  = {REL_1_PIN, REL_2_PIN, REL_3_PIN, REL_4_PIN, REL_5_PIN, REL_6_PIN, REL_7_PIN, REL_8_PIN, REL_9_PIN, REL_10_PIN, REL_11_PIN, REL_12_PIN, REL_13_PIN, REL_14_PIN, REL_15_PIN, REL_16_PIN};
const byte relayInitArray[] = {REL_1_INIT, REL_2_INIT, REL_3_INIT, REL_4_INIT, REL_5_INIT, REL_6_INIT, REL_7_INIT, REL_8_INIT, REL_9_INIT, REL_10_INIT, REL_11_INIT, REL_12_INIT, REL_13_INIT, REL_14_INIT, REL_15_INIT, REL_16_INIT};

const enum INPUT_PIN inputPinArray[]   = {INPUT_1_PIN, INPUT_2_PIN, INPUT_3_PIN, INPUT_4_PIN, INPUT_5_PIN, INPUT_6_PIN, INPUT_7_PIN};

/*==KEYPAD I2C==============================================================================================*/
const byte KEYPAD_ROWS            = 4;
const byte KEYPAD_COLS            = 3;
const byte KEYPAD_CODE_LENGTH     = 4;
const byte KEYPAD_CODE_LENGTH_MAX = 7;
const byte KeypadRowPins[KEYPAD_ROWS] = {1, 6, 5, 3}; 	// Measure
const byte KeypadColPins[KEYPAD_COLS] = {2, 0, 4};    	// Control wires (alternating HIGH)

const char KeypadKeys[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

bool KeypadTyping       = false;
bool KeypadCodeCorrect  = false;
bool KeypadCodeWrong    = false;

const int  KeypadWaitAfterCodeInput = 500;    // time for showing the entered code until it's checked
/*==Password================================================================================================*/
//bool passwordCheckup = false;

/*==Servo===================================================================================================*/
byte servo_correctstate = 1; // zero is execluded
byte servo_defaultstate = 90;
byte servo_wrongstate   = 170;
byte servo_currentstate;
bool UpdateServo        = false;

unsigned long coin_timerDelay      = 0;
unsigned long coin_TimeOutCoinBack = 90000;

/*==VARIABLES===============================================================================================*/
bool CoinIn      = false;     // coin inserted?
bool EndGame     = false;
/*==LCD=====================================================================================================*/
byte  countMiddle     = 6;
bool  noTaxi          = true;
bool  UpdateLCD       = true;

unsigned long UpdateLCDAfterDelayTimer = 0;
const unsigned long UpdateLCDAfterDelay = 5000;        /* Refreshing the LCD periodically */

/*==CONSTRUCTOR=============================================================================================*/
// PCF8574
Expander_PCF8574 relay;
Expander_PCF8574 inputs;
// Keypad
Keypad_I2C MyKeypad( makeKeymap(KeypadKeys), KeypadRowPins, KeypadColPins, KEYPAD_ROWS, KEYPAD_COLS, KEYPAD_I2C_ADD, PCF8574);
// Password
Password pass_tele_num = Password( "44243" );
// LCD
LiquidCrystal_I2C lcd(LCD_I2C_ADD, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
// Servo
Servo_I2C myservo;

//AUDIO
//NeoSWSerial altSerial( 8, 9 );
AltSoftSerial altSerial;
DFRobotDFPlayerMini myDFPlayer;
int tastenTon = 150;
bool UpdateDFP  = false;

/*============================================================================================================
//===SETUP====================================================================================================
//==========================================================================================================*/
void setup() {

  Serial.begin(115200);
  Serial.println(); Serial.println("==============Sweet Revange 29.04.2019=============");
	Serial.println(); Serial.println("===================SETUP====================="); Serial.println();


  if( lcd_Init() 		)	{Serial.println("LCD:     ok");	}
  if( relay_Init() 	)	{Serial.println("Relay:   ok");	}
  if( input_Init()  ) {Serial.println("Inputs:  ok"); }
  if( keypad_Init() ) {Serial.println("Keypad: ok");	}
  if( DFP_Init() )    {Serial.println("DFP:   ok");	}
  delay(500);
  if( servo_Init() )    {Serial.println("Servo:   ok");	}

  delay(500);
  i2c_scanner();
  delay(500);

  Serial.println(); Serial.println("===================START====================="); Serial.println();
}


/*============================================================================================================
//===LOOP=====================================================================================================
//==========================================================================================================*/
void loop() {

    if(!noTaxi && coinCheck() && !EndGame) {
      //relay.digitalWrite( REL_2_PIN, !REL_2_INIT );
      Serial.println("Everything correct, open door");
      EndGame = true;
      }
      else if (EndGame){
      delay(1000);
      //  relay.digitalWrite( REL_2_PIN, REL_2_INIT );
      }

    Keypad_Update();
    LCD_Update();
    DFP_Update();
    servo_Update();
}

/*============================================================================================================
//===FUNCTIONS================================================================================================
//==========================================================================================================*/

/*===Servo=================================================================================================*/
void servo_Update(){
if(coinCheck()){
  if (UpdateServo){
    UpdateServo = false;
    if (KeypadCodeCorrect) {
      Serial.print("Ser1");
      servo_coinBlock();
      Serial.println(" OK");
   } else if (KeypadCodeWrong) {
      Serial.print("Ser2");
      servo_coinBack();
      Serial.println(" OK");
   }
  }
  }
}

void servo_coinBlock(){
  myservo.write(servo_correctstate);
  delay(2500);
  myservo.write(servo_defaultstate);
  delay(20);
}

void servo_coinBack(){
  myservo.write(servo_wrongstate);
  delay(2500);
  myservo.write(servo_defaultstate);
  delay(20);
}

/*===AUDIO=================================================================================================*/
void DFP_Update(){
  if (UpdateDFP){
    UpdateDFP = false;
    if (KeypadCodeCorrect) {
      playCorrectPasswort();
    } else if (KeypadCodeWrong) {
      playWrongPassword();
    }
  }
}

void DFP_soundAufgelegt(){
  myDFPlayer.playMp3Folder(13);
  delay(500);
  myDFPlayer.playMp3Folder(13);
  delay(500);
  myDFPlayer.playMp3Folder(13);
  delay(500);
  myDFPlayer.playMp3Folder(13);
  delay(500);
  myDFPlayer.playMp3Folder(13);
  delay(500);
}

void playButtonPressed(char eKey){
  switch (eKey) {
    case '1': myDFPlayer.playMp3Folder(1);
              break;
    case '2': myDFPlayer.playMp3Folder(2);
              break;
    case '3': myDFPlayer.playMp3Folder(3);
              break;
    case '4': myDFPlayer.playMp3Folder(4);
              break;
    case '5': myDFPlayer.playMp3Folder(5);
              break;
    case '6': myDFPlayer.playMp3Folder(6);
              break;
    case '7': myDFPlayer.playMp3Folder(7);
              break;
    case '8': myDFPlayer.playMp3Folder(8);
              break;
    case '9': myDFPlayer.playMp3Folder(9);
              break;
    case '0': myDFPlayer.playMp3Folder(10);
              break;
  }
  delay(tastenTon);
}

void playCorrectPasswort(){
  Serial.println("Sound: BEGIN");
  myDFPlayer.playMp3Folder(11);
  delay(21200);
  DFP_soundAufgelegt();
}

void playWrongPassword(){
    Serial.println("Falsches Passwort eingegeben");
    myDFPlayer.pause();
    delay(50);
    myDFPlayer.playMp3Folder(12);
    delay(3300);
    myDFPlayer.playMp3Folder(13);
}

void DFP_printDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }

}

/*==Light Detector==========================================================================================*/
bool coinCheck() {
  if ( (( (millis() - coin_timerDelay) > coin_TimeOutCoinBack)) && noTaxi) {
    coinTimeOut();
    Serial.println("TimeOut Coin Back");
    coin_timerDelay = millis();
  }
  // if coin inserted return 1 else
  return true;
}

void coinTimeOut(){
  LCD_TimeOut();
  DFP_soundAufgelegt();
  passwordReset();
  servo_coinBack();
}
/*==LCD=====================================================================================================*/
void LCD_Update(){
	static unsigned int counterLCD = 0;

		if ( (( (millis() - UpdateLCDAfterDelayTimer) > UpdateLCDAfterDelay)) && !KeypadTyping && noTaxi) { UpdateLCD = true; Serial.println("LCD refresh"); }

		if (UpdateLCD && noTaxi) {
  			LCD_homescreen();
  			UpdateLCDAfterDelayTimer = millis();
  			UpdateLCD = false;
  //			Serial.println(KeypadTyping);
  //			Serial.println(KeypadCodeCorrect);
  //			Serial.println(KeypadCodeWrong);
  			Serial.print("UpdateLCD "); Serial.println(counterLCD);

  			if (KeypadTyping) {
  				LCD_keypadscreen();
  			}
  			else if (KeypadCodeCorrect) {
  				LCD_correct();
  //				KeypadCodeCorrect = false;
  			}
  			else if (KeypadCodeWrong) {
  				LCD_wrong();
  //				KeypadCodeWrong = false;
  			}
  			else { LCD_homescreen(); }
		  counterLCD++;
	   } else if (!noTaxi){
       LCD_TaxiArrived();
    }
}

void LCD_keypadscreen() {
  if( noTaxi ) {
    for ( int i=0; i<strlen((pass_tele_num.guess)); i++ ) {
        lcd.setCursor(countMiddle+i,0); lcd.print(pass_tele_num.guess[i]);
        if ( i == 11 ) {break;}
		}
	}
}

void LCD_homescreen() {
	lcd.clear();
	lcd.home();
	lcd.print("Num: ");
}

void LCD_TaxiArrived() {
	lcd.clear();
	lcd.print("Taxi Arrived");
}

void LCD_TimeOut(){
	lcd.clear();
  lcd.print("TimeOut.. Try Again");
	//delay(500); //TODO: remove
}

void LCD_correct(){
	lcd.clear();
  lcd.print("Calling Taxi...");
	//delay(500); //TODO: remove
	noTaxi = false;
}

void LCD_showCleared(){
    lcd.clear();
    lcd.print("CLEARED");
    //delay(750);
    LCD_homescreen();
}

void LCD_wrong(){
	lcd.clear();
  lcd.print("Calling Number...");
	//delay(500); //TODO: remove
}

/*==KEYPAD==================================================================================================*/
void Keypad_Update() {
  if (KeypadCodeCorrect) {
    KeypadCodeCorrect = false;
  }
  else if (KeypadCodeWrong) {
    KeypadCodeWrong = false;
  }

  MyKeypad.getKey();

	if(strlen((pass_tele_num.guess))== 5 && noTaxi){
		Serial.println("4 Zeichen eingebeben - ueberpruefe Passwort");
		UpdateLCD = true;
//		passwordCheckup = true;
		LCD_Update();
    delay(500);
		checkPassword();
		Serial.println("Check password Done");
	}
}

void keypadEvent(KeypadEvent eKey){
 Serial.println("Event Happened");
 switch( MyKeypad.getState() ) {
 	 case PRESSED:
 		 Serial.print("Taste: "); Serial.print(eKey); Serial.print(" -> Code: "); Serial.print(pass_tele_num.guess); Serial.println(eKey);
 		 KeypadTyping = true;
		 UpdateLCD = true;

 		 switch (eKey){
       case '#': Serial.println("Hash Not Used");
					  break;
			 case '*': Serial.println("cleared");
			 			passwordReset();
						if (noTaxi){
              LCD_showCleared();
						}
					  break;
			 default:
                pass_tele_num.append(eKey);
                playButtonPressed(eKey);
					  break;
 		 }
 		 break;

 	case HOLD:
 		Serial.print("HOLD: ");	Serial.println(eKey);
 		switch (eKey){
 			case '*': software_Reset();
 			break;
    	}
    	break;
 	}
}

void checkPassword() {
  UpdateLCD = true;
  UpdateDFP = true;
  UpdateServo = true;
	if ( pass_tele_num.evaluate() ) {
		KeypadCodeCorrect = true;
		KeypadTyping = false;
//		passwordCheckup = false;

		Serial.println( "Taxi is called" );
    passwordReset();
	}
	else {
		KeypadCodeWrong = true;
		KeypadTyping = false;
//		passwordCheckup = false;
		Serial.println( "Wrong number" );
		passwordReset();
	}
}

void passwordReset() {
	KeypadTyping = false;
	UpdateLCD = true;

	pass_tele_num.reset();
	Serial.println( "Password reset" );
}

/*============================================================================================================
//===INIT=====================================================================================================
//==========================================================================================================*/

/*==INPUTS==================================================================================================*/
bool input_Init() {
  inputs.begin(INPUT_I2C_ADD);
  for( int i=0; i<INPUT_AMOUNT; i++ ) {
    inputs.pinMode(inputPinArray[i], INPUT);
  }
  return true;
}

/*===LCD====================================================================================================*/
bool lcd_Init() {
	lcd.begin(20,4);  // 20*4 New LiquidCrystal
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

/*===Servo==================================================================================================*/
bool servo_Init(){
  myservo.begin(SERVO_I2C_ADD);
  delay(20);
  myservo.write(servo_wrongstate);
  delay(2500);
  myservo.write(servo_defaultstate);
  delay(20);
  servo_currentstate = servo_defaultstate;
  return true;
}

/*===KEYPAD=================================================================================================*/
bool keypad_Init() {
	MyKeypad.addEventListener(keypadEvent);    // Event Listener erstellen
	MyKeypad.begin( makeKeymap(KeypadKeys) );
	MyKeypad.setHoldTime(5000);
	MyKeypad.setDebounceTime(20);

	return true;
}

/*===MOTHER=================================================================================================*/
bool relay_Init() {
  relay.begin(RELAY_I2C_ADD);
  for (int i=0; i<REL_AMOUNT; i++) {
     relay.pinMode(i, OUTPUT);
     relay.digitalWrite(i, HIGH);
  }

  delay(500);

  for (int i=0; i<REL_AMOUNT; i++) {
     relay.digitalWrite(relayPinArray[i], relayInitArray[i]);
     Serial.print("     ");
   	 Serial.print("Relay ["); Serial.print(relayPinArray[i]); Serial.print("] set to "); Serial.println(relayInitArray[i]);
   	 delay(20);
  }

  Serial.println();

	return true;
}

/*===AUDIO=================================================================================================*/
bool DFP_Init() {
  altSerial.begin(9600);
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));

  if (!myDFPlayer.begin(altSerial)) {  //Use softwareSerial to communicate with mp3. mySoftwareSerial
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true){
      delay(0); // Code to compatible with ESP8266 watch dog.
    }
  }
  Serial.println(F("DFPlayer Mini online."));

  myDFPlayer.volume(20);  //Set volume value. From 0 to 30
  //  myDFPlayer.play(1);  //Play the first mp3
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
  delay(750);
}

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
      if (i == 39) Serial.print(F(" -> LCD"));
      if (i == 56) Serial.print(F(" -> LCD-I2C-Board"));
      if (i == 57) Serial.print(F(" -> Input-I2C-board"));
      if (i == 60) Serial.print(F(" -> Display"));
      if (i == 63) Serial.print(F(" -> Relay"));
      if (i == 22) Serial.print(F(" -> Servo-I2C-Board"));
      Serial.println();
      wire_device_count++;
      delay (1);
    }
  }
  Serial.print   (F("Found "));
  Serial.print   (wire_device_count, DEC);
  Serial.println (F(" device(s)."));

  Serial.println();

  delay(500);
}

void software_Reset() {
  Serial.println(F("Restarting in"));
  delay(250);
  for (byte i = 3; i>0; i--) {
    Serial.println(i);
    delay(100);
  }
  asm volatile ("  jmp 0");
}
