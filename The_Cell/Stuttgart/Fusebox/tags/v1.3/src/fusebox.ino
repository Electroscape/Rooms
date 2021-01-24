/*==========================================================================================================*/
/*		2CP - TeamEscape - Engineering
 *		by Abdullah Saei & Martin Pek
 *
 *		v1.2b
 *		- removed WDG
 *		- 07.08.2019
 *		- For magnet
 *      - 12v PS starts 5 seconds after electronics
 *      - Standby added for LCD + PWreset
        - simplification on fusecheck
 */
/*==========================================================================================================*/

#define SVERSION        1
#define HHVERSION       0
#define BASEVERSION     SVERSION

const String title = String("FUSEBOX S v1.3");

/*==INCLUDE=================================================================================================*/
//Watchdog timer
#include <avr/wdt.h>
// Library
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

/*==DEFINE==================================================================================================*/
#define DEBUG_MODE 					0
// onBoardLED
#define ON_BOARD_LED_PIN            13
#define IGNORE_KEYPAD               0
// LED
// PIN
enum PWM_PIN {
    PWM_1_PIN = 3,                           // Predefined by STB design
    PWM_2_PIN = 5,                           // Predefined by STB design
    PWM_3_PIN = 6,                           // Predefined by STB design
    PWM_4_PIN = 9,                           // Predefined by STB design
};
// SETTINGS
// I2C ADRESSES
#define RELAY_I2C_ADD     	 0x3F         // Relay Expander
#define OLED_I2C_ADD         0x3C         // Predefined by hardware
#define LCD_I2C_ADD					 0x27 // Predefined by hardware
#define KEYPAD_I2C_ADD       0x38         // Keypad
#define FUSE_I2C_ADD         0x39         // Fuses

// RELAY
// PIN
enum REL_PIN{
  REL_1_PIN ,                              // 0 Fusebox lid
  REL_2_PIN ,                              // 1 Door opener / Alarm light
  REL_3_PIN ,                              // 2
  REL_4_PIN ,                              // 3
  REL_5_PIN ,                              // 4
  REL_6_PIN ,                              // 5
  REL_7_PIN ,                              // 6
  REL_8_PIN ,                              // 7 12v PS
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
  REL_1_INIT   =                0,        // COM-12V_IN, NO-12V_OUT, NC-/  set to 1 for magnet, 0 for mechanical
  REL_2_INIT   =                1,        // COM-12V_IN, NO-12V_OUT_DOOR, NC-12V_OUT_ALARM
  REL_3_INIT   =                1,        // NC-12V_OUT_ALARM
  REL_4_INIT   =                1,        // DESCRIPTION OF THE RELAY WIRING
  REL_5_INIT   =                1,        // DESCRIPTION OF THE RELAY WIRING
  REL_6_INIT   =                1,        // DESCRIPTION OF THE RELAY WIRING
  REL_7_INIT   =                1,        // DESCRIPTION OF THE RELAY WIRING
  REL_8_INIT   =                1,        // COM AC_volt, NO 12_PS+, NC-/
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
  INPUT_1_PIN,                             //  0 alarm light at the entrance/exit
  INPUT_2_PIN,                             //  1 trigger for starting the procedure
  INPUT_3_PIN,                             //  2
  INPUT_4_PIN,                             //  3
  INPUT_5_PIN,                             //  4
  INPUT_6_PIN,                             //  5
  INPUT_7_PIN,                             //  6
  INPUT_8_PIN
};
// AMOUNT
#define FUSE_COUNT             5    // same as input_count in older code

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

const unsigned long keypadStandbyTime = 5000;           // disabled backlight after this amount of incactivity

/*==Password================================================================================================*/
bool passwordCheckup = false;
bool PassWrong     = true;
const unsigned long reset_lockstatus_after = 15000;
unsigned long time_lastUnlock = millis();

/*==VARIABLES===============================================================================================*/

byte fuseState[FUSE_COUNT];
bool fuseCorrect = false;      // Fuses that have to be removed
bool fuseFalse   = false;      // Fuses that have to remain on the sockets
bool fuseSolutions[] = {0,0,0,0,1};

#define RESTARTTIME 15

/*==LCD=====================================================================================================*/
byte  countMiddle = 8;
byte  keyCount    = countMiddle;



bool UpdateLCD      = true;

unsigned long lastLCDUpdate = 0;
const unsigned long UpdateLCDAfterDelay = 6000;        /* Refreshing the LCD periodically */


/*==TIMER===================================================================================================*/
unsigned long lastTimestamp_fuse = 0;
const unsigned long timespan_fuseCheck = 100;

/*==CONSTRUCTOR=============================================================================================*/
// PCF8574
Expander_PCF8574 relay;
Expander_PCF8574 iFuse;
Expander_PCF8574 LetsFixThis;
// Keypad
Keypad_I2C MyKeypad( makeKeymap(KeypadKeys), KeypadRowPins, KeypadColPins, KEYPAD_ROWS, KEYPAD_COLS, KEYPAD_I2C_ADD, PCF8574);
// Password
Password pass_fusebox = Password( "2517" );   // @Abdullah, please check if this is the right code
// LCD
LiquidCrystal_I2C lcd(LCD_I2C_ADD, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);


/*============================================================================================================
//===SETUP====================================================================================================
//==========================================================================================================*/
void setup() {
    //wdt_disable();
    Serial.begin(115200);
    Serial.println("!header_begin");
    delay(50);
    Serial.println(); Serial.println("==============FuseBox 20.08.2019=============");
    Serial.println(); Serial.println("===================SETUP=====================");
    Serial.println("==SV1.3==");
    Serial.println("==no hystersis protection==");
    Serial.println("==IGNORE_KEYPADSTATE==");
    Serial.println(IGNORE_KEYPAD);
    Serial.println("!header_end");

    Serial.println();

    Serial.println("!setup_begin");

    // i2c_scanner();

    if( lcd_Init() 		)	{Serial.println("LCD:     ok");	}
    delay(50);
    if( relay_Init() 	)	{Serial.println("Relay:   ok");	}
    delay(50);
    if( input_Init()  ) {Serial.println("Inputs:  ok"); }
    delay(50);
    if( Keypad_Init() ) {Serial.println("Keypad: ok");	}

    delay(500);
    i2c_scanner();
    delay(500);

    //Serial.println("WDT endabled");
    //wdt_enable(WDTO_8S);

    Serial.println("!setup_end");

    blink_onBoardled(500);
    Serial.println(); Serial.println("===================START====================="); Serial.println();
#if IGNORE_KEYPAD
    LCD_correct();
#endif
}


/*============================================================================================================
//===LOOP=====================================================================================================
//==========================================================================================================*/

void loop() {

    if ((millis() - lastTimestamp_fuse) > timespan_fuseCheck ) {

        if (fuseCheck()) {
            bool isNoHystersis = true;
            for (int i=0; i > 5; i++) {
                if (!fuseCheck()) {
                    isNoHystersis = false;
                    break;
                }
                delay(10);
            }
            if (isNoHystersis) {
                open_room_door();
            } else {
                Serial.println("Hysteresis detected not opening");
            }

        } else {
            close_room_door();
        }
        lastTimestamp_fuse = millis();
    }

    // periodically resetting the lock status if unlocked
    if (!PassWrong) {
        if ((millis() - time_lastUnlock) > reset_lockstatus_after) {
            PassWrong = true;
            relay.digitalWrite(REL_1_PIN, REL_1_INIT);
            delay(100);
        }
    }

    LCD_Update();
    Keypad_Update();

    delay(50);
    //wdt_reset();
}


void open_room_door() {
    if (relay.digitalRead(REL_2_PIN) == REL_2_INIT) {
        relay.digitalWrite( REL_2_PIN, !REL_2_INIT );
        delay(100);
    }
    if (relay.digitalRead(REL_3_PIN) == REL_3_INIT) {
        relay.digitalWrite( REL_3_PIN, !REL_3_INIT );
        delay(100);
    }
    Serial.println("Everything correct, open door");
}


void close_room_door() {
    relay.digitalWrite( REL_2_PIN, REL_2_INIT );
    relay.digitalWrite( REL_3_PIN, REL_3_INIT );
}

/*============================================================================================================
//===FUNCTIONS================================================================================================
//==========================================================================================================*/

/*==FUSES===================================================================================================*/
bool fuseCheck() {

    delay(10);
    // since the exit is faster with the last fuse which are the removed fuses
    // simply reverse the search
    int ret = 1;
    for (int i=FUSE_COUNT-1; i >= 0; i--) {
        Serial.println("");
        Serial.print("fuseNo"); Serial.print(i);
        Serial.print("with result: ");
        Serial.println(iFuse.digitalRead(inputPinArray[i]));
        Serial.println("");
        if (iFuse.digitalRead(inputPinArray[i]) != fuseSolutions[i]) {
            Serial.print("incorrect fuse installed at slot: ");
            Serial.print(i);
            Serial.print("with result: ");
            Serial.println(iFuse.digitalRead(inputPinArray[i]));

            return 0;
        }
        delay(15);
    }
    return ret;
}

/*==LCD=====================================================================================================*/
void LCD_Update() {

	if ( (( (millis() - lastLCDUpdate) > UpdateLCDAfterDelay)) && !KeypadTyping) { UpdateLCD = true; Serial.println("LCD refresh"); }

	if (UpdateLCD) {

		if (PassWrong) LCD_homescreen();
		lastLCDUpdate = millis();
		UpdateLCD = false;
		Serial.println("UpdateLCD");
#if DEBUG_MODE == 2
		Serial.println(KeypadTyping);
		Serial.println(KeypadCodeCorrect);
		Serial.println(KeypadCodeWrong);
#endif

		if (KeypadTyping) {
            lcd.backlight();
			LCD_keypadscreen();
		} else if (KeypadCodeCorrect) {
			LCD_correct();
            input_Init();
            delay(100);
            relay.digitalWrite(REL_1_PIN, !REL_1_INIT);
            Serial.println( "Fusebox open" );
            delay(100);
			KeypadCodeCorrect = false;
		} else if (KeypadCodeWrong) {
			LCD_wrong();
			UpdateLCD = true;
			KeypadCodeWrong = false;
		} else {
            if(PassWrong) LCD_homescreen();
        }

	} else {
        if (millis() - lastLCDUpdate > keypadStandbyTime) {
            passwordReset();
            if(PassWrong) LCD_homescreen();
            lcd.noBacklight();
        }
    }
}

void LCD_keypadscreen() {
    if( PassWrong ) {
        for ( uint i=0; i<strlen((pass_fusebox.guess)); i++ ) {
            lcd.setCursor(8+i,1); lcd.print(pass_fusebox.guess[i]);
            if ( i == 11 ) {break;}
        }
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
}

void LCD_wrong(){
	lcd.clear();
	lcd.setCursor(7,1);
	lcd.print("ACCESS");
	lcd.setCursor(7,2);
	lcd.print("DENIED");
	delay(1000);
}

/*==KEYPAD==================================================================================================*/
void Keypad_Update() {
    MyKeypad.getKey();

    if(strlen((pass_fusebox.guess))== 4 && PassWrong) {
        Serial.println("4 Zeichen eingebeben - ueberpruefe Passwort");
        keyCount = countMiddle;
        UpdateLCD = true;
        passwordCheckup = true;
        LCD_Update();
        delay(500);
        checkPassword();
        Serial.println("Check password Done");
    }
}


void keypadEvent(KeypadEvent eKey) {
    Serial.println("Event Happened");
    lcd.backlight();
 switch( MyKeypad.getState() ) {
 	 case PRESSED:
 		 Serial.print("Taste: "); Serial.print(eKey); Serial.print(" -> Code: "); Serial.print(pass_fusebox.guess); Serial.println(eKey);
 		 KeypadTyping = true;
		 UpdateLCD = true;

 		 switch (eKey) {

       case '#': Serial.println("Hash Not Used");
					  break;
			 case '*': Serial.println("cleared");
    		 			passwordReset();
    					if (PassWrong) {
    						keyCount = countMiddle;
    						lcd.clear();
     						lcd.setCursor(6,1);
     						lcd.print("CLEARED");
     						delay(750);
     						LCD_homescreen();
    					}
					  break;
            default:
                if (PassWrong) { pass_fusebox.append(eKey);}
            break;
 		 }
 		 break;

 	case HOLD:
 		Serial.print("HOLD: ");	Serial.println(eKey);
 		switch (eKey){
      case '7':
#if DEBUG_MODE
        blink_onBoardled(200);
        for (int z = 1; z<10; z++) {
            Serial.println(z);
            delay(1000);
        };
#endif
      break;
 			case '*': software_Reset();
 			break;
    	}
    	break;
 	}
}

void checkPassword() {
	if ( pass_fusebox.evaluate() ) {
		KeypadCodeCorrect = true;
		KeypadTyping = false;
		UpdateLCD = true;
		passwordCheckup = false;
        PassWrong = false;
        time_lastUnlock = millis();
        Serial.println( "Correct password" );
		passwordReset();
	} else {
		KeypadCodeWrong = true;
		KeypadTyping = false;
		UpdateLCD = true;
		passwordCheckup = false;

		Serial.println( "Wrong password" );
		passwordReset();
	}
}

void passwordReset() {
	KeypadTyping = false;
	UpdateLCD = true;
	pass_fusebox.reset();
	Serial.println( "Password reset" );
}

/*============================================================================================================
//===INIT=====================================================================================================
//==========================================================================================================*/

/*==INPUTS==================================================================================================*/
bool input_Init() {
    delay(5);
    LetsFixThis.begin(FUSE_I2C_ADD);
    for (int i=0; i<=7; i++) {
        LetsFixThis.pinMode(i, INPUT);
        LetsFixThis.digitalWrite(i, HIGH);
    }
    delay(100);
    iFuse.begin(FUSE_I2C_ADD);
    delay(5);
    for (int i=0; i < FUSE_COUNT; i++) {
        iFuse.pinMode(inputPinArray[i], INPUT);
        fuseState[i] = iFuse.digitalRead(inputPinArray[i]);
        delay(5);
    }
  return true;
}

/*===LCD====================================================================================================*/
bool lcd_Init() {
	lcd.begin(20,4);  // 20*4 New LiquidCrystal
	lcd.setCursor(0,1);
	lcd.print("Starting...");

	for (int i =0; i<2; i++) {
		delay(500);
		lcd.noBacklight();
		delay(500);
		lcd.backlight();
	}

	LCD_homescreen();
#if IGNORE_KEYPAD
    lcd.noBacklight();
    lcd.print("");
#endif

	return true;
}

/*===KEYPAD=================================================================================================*/
bool Keypad_Init() {
	MyKeypad.addEventListener(keypadEvent);    // Event Listener erstellen
    delay(10);
    LetsFixThis.begin(FUSE_I2C_ADD);
    for (int i=0; i<=7; i++) {
     LetsFixThis.pinMode(i, INPUT);
     LetsFixThis.digitalWrite(i, HIGH);
    }
    delay(100);
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
        relay.digitalWrite(relayPinArray[i], relayInitArray[i]);
        // we dont set the pins signal, we do a silent restart and check conditions in the loop
        delay(100);
    }


    delay(100);
    //relay.pinMode(REL_8_PIN, OUTPUT);
    relay.digitalWrite(REL_8_PIN, !REL_8_INIT);

    Serial.println("12V Peripheral Power Supply ON");

    return true;
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

void blink_onBoardled(uint8_t delay_ms){
	pinMode(ON_BOARD_LED_PIN, OUTPUT);
	digitalWrite(ON_BOARD_LED_PIN, HIGH);
	delay(delay_ms);
	digitalWrite(ON_BOARD_LED_PIN, LOW);
	delay(delay_ms);
	digitalWrite(ON_BOARD_LED_PIN, HIGH);
	delay(delay_ms);
	digitalWrite(ON_BOARD_LED_PIN, LOW);
}

void software_Reset() {
#if DEBUG_MODE
    Serial.println(F("Restarting in"));
    for (byte i = RESTARTTIME; i>0; i--) {
        //wdt_reset();
        Serial.println(i);
        delay(1000);
    }
#endif
    blink_onBoardled(50);
    asm volatile ("  jmp 0");
}

/*
ISR(WDT_vect)
{
	blink_onBoardled(50);
}
*/
