/*==========================================================================================================*/
/*		2CP - TeamEscape - Engineering
 *		by Abdullah Saei & Robert Schloezer
 *
 *		v1.5
 *		- Last Changes 08.07.2019
 *		- Add WDT
 *		- Bug fixing undefined state z = 5
 */
/*==========================================================================================================*/

const String title = String("REINRAUMSTEUERUNG v1.1");

/*==INCLUDE=================================================================================================*/
//Watchdog timer
#include <avr/wdt.h>
// Library
#include <Wire.h>                     //TWI / I2C    
// I2C Port Expander
#include "PCF8574.h"

// WS2812B LED Strip
#include <FastLED.h>

/*==DEFINE==================================================================================================*/
// LED
// PIN
#define PWM_1_PIN 3                       // Predefined by STB design
#define PWM_2_PIN 5                       // Predefined by STB design
#define PWM_3_PIN 6                       // Predefined by STB design
#define PWM_4_PIN 9                       // Predefined by STB design
// SETTINGS
#define LED_STRIP WS2812B                                                           // Type of LED Strip
#define MAX_DIMENSION ((kMatrixWidth>kMatrixHeight) ? kMatrixWidth : kMatrixHeight) // if w > h -> MAX_DIMENSION=kMatrixWidth, else MAX_DIMENSION=kMatrixHeight
#define NUM_LEDS (kMatrixWidth * kMatrixHeight)

// I2C ADRESSES
#define RELAY_I2C_ADD     	 0x3F         // Relay Expander
#define DETECTOR_I2C_ADD     0x39         // door opening detector Expander

// RELAY
// PIN
#define REL_ALARM_PIN            0        // three red lights on the wall -> blinks when procedure starts
#define REL_FAN_OUT_BIG_PIN      1        // big, yellow fan -> fog out
#define REL_FAN_OUT_SMALL_PIN    2        // small fan -> fog out
#define REL_FAN_IN_SMALL_PIN     3        // small fan -> fresh air in
#define REL_FOG_PIN              4        // fog machine
#define REL_KEYPAD_PIN           5        // keypad -> access to the clean room
#define REL_MAGNET_PIN           6        // magnet at the clean room door (needs to be on during the procedure, so the door is closed)
#define REL_ENTRY_ALARM_PIN      7        // alarm light at the room entrance
// INIT
#define REL_ALARM_INIT           1        // DESCRIPTION OF THE RELAY WIRING
#define REL_FAN_OUT_BIG_INIT     1        // DESCRIPTION OF THE RELAY WIRING
#define REL_FAN_OUT_SMALL_INIT   1        // DESCRIPTION OF THE RELAY WIRING
#define REL_FAN_IN_SMALL_INIT    1        // DESCRIPTION OF THE RELAY WIRING
#define REL_FOG_INIT             1        // DESCRIPTION OF THE RELAY WIRING
#define REL_KEYPAD_INIT          1        // DESCRIPTION OF THE RELAY WIRING
#define REL_MAGNET_INIT          1        // DESCRIPTION OF THE RELAY WIRING
#define REL_ENTRY_ALARM_INIT     1        // DESCRIPTION OF THE RELAY WIRING

// INPUT
#define REED_0_PIN    0                   // alarm light at the entrance/exit
#define REED_1_PIN    1                   // trigger for starting the procedure


/*==LED STRIP================================================================================================*/
static uint16_t x;
static uint16_t y;
static uint16_t z;

// Params for width and height
const uint8_t kMatrixWidth = 1;
const uint8_t kMatrixHeight = 64;

// Param for different pixel layouts
const bool kMatrixSerpentineLayout = true;
bool countup = true;
uint8_t ihue=0;


uint16_t XY( uint8_t x, uint8_t y)
{
  uint16_t i;

  if( kMatrixSerpentineLayout == false) {
    i = (y * kMatrixWidth) + x;
  }

  if( kMatrixSerpentineLayout == true) {
    if( y & 0x01) {
      // Odd rows run backwards
      uint8_t reverseX = (kMatrixWidth - 1) - x;
      i = (y * kMatrixWidth) + reverseX;
    } else {
      // Even rows run forwards
      i = (y * kMatrixWidth) + x;
    }
  }

  return i;
}

CRGB leds[kMatrixWidth * kMatrixHeight];

/*==FLAGS===================================================================================================*/
bool firstRun     = false;
bool entranceOpen = false;
long timenow = 0;
/*==PCF8574=================================================================================================*/
Expander_PCF8574 relay;
Expander_PCF8574 iTrigger;


/*============================================================================================================
//===SETUP====================================================================================================
//==========================================================================================================*/
void setup() {
  wdt_disable();
  Serial.begin(115200);
  Serial.println(); Serial.println("===================date 02.12.2019====================="); Serial.println();
  
  delay(500);
  i2c_scanner();
  delay(500);
  
  if( led_Init() 	  )	{Serial.println("LED:     ok"); }
  if( input_Init()  ) {Serial.println("Inputs:  ok"); }
  if( Relay_Init() 	)	{Serial.println("Relay:   ok");	}

  //Alarm entrance runs for seconds after restart
  relay.digitalWrite(REL_ENTRY_ALARM_PIN, !REL_ENTRY_ALARM_INIT);
  //Fog out fan runs for seconds after restart
  relay.digitalWrite(REL_FAN_IN_SMALL_PIN, REL_FAN_IN_SMALL_INIT);
  delay(2000);
  wdt_enable(WDTO_8S);

  if(!iTrigger.digitalRead(REED_1_PIN)) {
    Serial.println("Lab door already open");
    delay(2000);
    Serial.println("Lab door: open (2nd check)");
      if(!iTrigger.digitalRead(REED_1_PIN)) {
        finalState();
      }
    Serial.println("Jumped to final state");
  }
  //Alarm entrance off
  relay.digitalWrite(REL_ENTRY_ALARM_PIN, REL_ENTRY_ALARM_INIT);
  //Fog out fan off
  relay.digitalWrite(REL_FAN_IN_SMALL_PIN, REL_FAN_IN_SMALL_INIT);
  Serial.println("===================START====================="); Serial.println();
}

/*============================================================================================================
//===LOOP=====================================================================================================
//==========================================================================================================*/
void loop() {

  wdt_reset();
  delay(10);
    if(!iTrigger.digitalRead(REED_0_PIN) && !firstRun && !entranceOpen) {
    Serial.println("Entrance: open");
    entranceOpen = true;
    relay.digitalWrite(REL_ENTRY_ALARM_PIN, !REL_ENTRY_ALARM_INIT);
  }
  else if(iTrigger.digitalRead(REED_0_PIN) && !firstRun  && entranceOpen){
    Serial.println("Entrance: closed");
    entranceOpen = false;
    relay.digitalWrite(REL_ENTRY_ALARM_PIN, REL_ENTRY_ALARM_INIT);
  }

  delay(10);
	if(!iTrigger.digitalRead(REED_1_PIN) && !firstRun) {
    Serial.println("Lab door: open");
    delay(2000);
    Serial.println("Lab door: open (2nd check)");
      if(!iTrigger.digitalRead(REED_1_PIN)) {
        relay.digitalWrite(REL_ENTRY_ALARM_PIN, REL_ENTRY_ALARM_INIT);
        Serial.println("Decontamination: start");
        wdt_reset();
        if( decontamination(false) ) { Serial.println("Decontamination: end"); }
        wdt_reset();
        firstRun = true;
      }
  }
  
  delay(10);
  if( firstRun && iTrigger.digitalRead(REED_1_PIN) ){
    Serial.println("Lab door: closed");

    timenow=millis();   //get current time and reset timer in case the door is not closed for continuous 3 mins

    while (iTrigger.digitalRead(REED_1_PIN)) //check if the lab door is closed
    {
        wdt_reset();
        //Check if the door is closed for 3 mins with interval of 100ms.
        if ((millis()-timenow)>180000)
        {
          Serial.println("Lab door: closed for 3 mins -> RESET");
          software_Reset();
        }
        delay(100);
        //Serial.println(millis()-timenow);
    }
    Serial.println("Lab door: opened before 3 mins -> timerReset");
  }
}


/*============================================================================================================
//===DECONTAMINATION==========================================================================================
//==========================================================================================================*/
void finalState(){
  // White light -----------------------------------------------
  for(int i = 0; i < kMatrixWidth; i++) {
    for(int j = 0; j < kMatrixHeight; j++) {
      leds[XY(i,j)] = CHSV(170,95,255);
    }
  }
  FastLED.show();
  Serial.println("LED: white");
  delay(100);

  relay.digitalWrite(REL_ALARM_PIN, REL_ALARM_INIT); Serial.println("Light: on");
  
  firstRun = true;
}

bool decontamination( bool cleanroomLocked ) {
  if( cleanroomLocked ) {
    relay.digitalWrite(REL_MAGNET_PIN, !REL_MAGNET_INIT);
    Serial.println("Clean room: locked");
    delay(500);
    relay.digitalWrite(REL_KEYPAD_PIN, !REL_KEYPAD_INIT);
    Serial.println("Clean room: Keypad off");
    delay(2500);
    wdt_reset();
  }

  for(int i = 0; i < kMatrixWidth; i++) {
    for(int j = 0; j < kMatrixHeight; j++) {
      leds[XY(i,j)] = CHSV(0,255,0);              // LED "off"
    }
  }
  FastLED.show();

  wdt_reset();
  relay.digitalWrite(REL_ALARM_PIN, !REL_ALARM_INIT); Serial.println("Light: off");
  delay(1000);
  relay.digitalWrite(REL_FAN_OUT_SMALL_PIN, !REL_FAN_OUT_SMALL_INIT); Serial.println("Fan OUT small: on");
  relay.digitalWrite(REL_FAN_IN_SMALL_PIN, !REL_FAN_IN_SMALL_INIT); Serial.println("Fan IN small: on");
  delay(4000);

  // Fog and blinking lights
  for(int z=0; z<35; z++){

    Serial.print("z: "); Serial.println(z);
    if (z==0) {relay.digitalWrite(REL_FOG_PIN, !REL_FOG_INIT); Serial.println("Fog: on");
    }
    else if (z==10) {relay.digitalWrite(REL_FOG_PIN, REL_FOG_INIT); Serial.println("Fog: off");
    }
    else if (z==12) {relay.digitalWrite(REL_FAN_OUT_BIG_PIN, !REL_FAN_OUT_BIG_INIT); Serial.println("Fan OUT big: on");}
    else{}//Serial.print("ez: "); Serial.println(z);}

    for(int i = 0; i < kMatrixWidth; i++) {
      for(int j = 0; j < kMatrixHeight; j++) {
        leds[XY(i,j)] = CHSV(170,95,255);
      }
    }
    FastLED.show();
    relay.digitalWrite(REL_ALARM_PIN, !REL_ALARM_INIT);
    wdt_reset();
    delay(1000);

    for(int i = 0; i < kMatrixWidth; i++) {
      for(int j = 0; j < kMatrixHeight; j++) {
        leds[XY(i,j)] = CHSV(0,255,255);
      }
    }
    FastLED.show();

    relay.digitalWrite(REL_ALARM_PIN, REL_ALARM_INIT);
    wdt_reset();
    delay(1000);

  }

  // Green light -----------------------------------------------
  for(int i = 0; i < kMatrixWidth; i++) {
    for(int j = 0; j < kMatrixHeight; j++) {
      leds[XY(i,j)] = CHSV(96,255,255);
    }
  }
  FastLED.show();
  Serial.println("LED: green");

  wdt_reset();
  delay(5000);

  relay.digitalWrite(REL_FAN_OUT_SMALL_PIN, REL_FAN_OUT_SMALL_INIT); Serial.println("Fan OUT small: off");
  relay.digitalWrite(REL_FAN_IN_SMALL_PIN, REL_FAN_IN_SMALL_INIT); Serial.println("Fan IN small: off");
  relay.digitalWrite(REL_ALARM_PIN, REL_ALARM_INIT); Serial.println("Light: on");
  if ( cleanroomLocked ) {
    relay.digitalWrite(REL_KEYPAD_PIN, REL_KEYPAD_INIT); Serial.println("Clean room: Keypad on");
  }

  // White light -----------------------------------------------
  for(int i = 0; i < kMatrixWidth; i++) {
    for(int j = 0; j < kMatrixHeight; j++) {
      leds[XY(i,j)] = CHSV(170,95,255);
    }
  }
  FastLED.show();
  Serial.println("LED: white");

  wdt_reset();
  delay(6000);

  if ( cleanroomLocked ) {
    relay.digitalWrite(REL_MAGNET_PIN, REL_MAGNET_INIT);
    Serial.println("Clean room: unlocked");
  }

  wdt_reset();
  delay(7000);
  wdt_reset();

  relay.digitalWrite(REL_FAN_OUT_BIG_PIN, REL_FAN_OUT_BIG_INIT);
  Serial.println("Fan OUT big: off");

  return true;

}
/*============================================================================================================
//===INPUTS===================================================================================================
//==========================================================================================================*/
bool input_Init() {
  Serial.print("1");
  iTrigger.begin(DETECTOR_I2C_ADD);
  Serial.print("2");
  iTrigger.pinMode(REED_0_PIN, INPUT);
  Serial.print("3");
  iTrigger.pinMode(REED_1_PIN, INPUT);
  Serial.print("4");
  return true;
}

/*============================================================================================================
//===LED======================================================================================================
//==========================================================================================================*/
bool led_Init() {
  int LEDdelay = 500;
  LEDS.addLeds<LED_STRIP,PWM_1_PIN,GRB>(leds,NUM_LEDS);
  LEDS.setBrightness(100);

  // Initialize our coordinates to some random values
  x = random16();
  y = random16();
  z = random16();

  for(int i = 0; i < kMatrixWidth; i++) {
    for(int j = 0; j < kMatrixHeight; j++) {
      leds[XY(i,j)] = CHSV(0,255,255);
    }
  }
  FastLED.show();
  delay(LEDdelay);

  for(int i = 0; i < kMatrixWidth; i++) {
    for(int j = 0; j < kMatrixHeight; j++) {
      leds[XY(i,j)] = CHSV(40,255,255);
    }
  }
  FastLED.show();
  delay(LEDdelay);

  for(int i = 0; i < kMatrixWidth; i++) {
    for(int j = 0; j < kMatrixHeight; j++) {
      leds[XY(i,j)] = CHSV(120,255,255);
    }
  }
  FastLED.show();
  delay(LEDdelay);

  for(int i = 0; i < kMatrixWidth; i++) {
    for(int j = 0; j < kMatrixHeight; j++) {
      leds[XY(i,j)] = CHSV(200,255,255);
    }
  }
  FastLED.show();
  delay(LEDdelay);

  for(int i = 0; i < kMatrixWidth; i++) {
    for(int j = 0; j < kMatrixHeight; j++) {
      leds[XY(i,j)] = CHSV(20,255,255);
    }
  }
  FastLED.show();
  delay(LEDdelay);

  for(int i = 0; i < kMatrixWidth; i++) {
    for(int j = 0; j < kMatrixHeight; j++) {
      leds[XY(i,j)] = CHSV(170,95,255);
    }
  }
  FastLED.show();

  return true;
}

/*============================================================================================================
//===MOTHER===================================================================================================
//==========================================================================================================*/
bool Relay_Init() {
  Serial.print("1");
  relay.begin(RELAY_I2C_ADD);
  Serial.print("2");
  for (int i=0; i<=7; i++) {
     relay.pinMode(i, OUTPUT);
     relay.digitalWrite(i, HIGH);
  }
  Serial.print("3");
	delay(1000);
  // Relay 0
	relay.digitalWrite(REL_ALARM_PIN, REL_ALARM_INIT);      /* Licht-Relais an, damit Licht initial aus.              NC -> Licht an   */
	Serial.print("     ");
	Serial.print("Alarm Pin ["); Serial.print(REL_ALARM_PIN); Serial.print("] set to "); Serial.println(REL_ALARM_INIT);
	delay(20);
  // Relay 1
	relay.digitalWrite(REL_FAN_OUT_BIG_PIN, REL_FAN_OUT_BIG_INIT);       /* Tür-Relais an, damit Ruhestromtür verschlossen bleibt. NC -> Tuer offen */
	Serial.print("     ");
	Serial.print("Big Fan OUT Pin ["); Serial.print(REL_FAN_OUT_BIG_PIN); Serial.print("] set to "); Serial.println(REL_FAN_OUT_BIG_INIT);
  delay(20);
  // Relay 2
	relay.digitalWrite(REL_FAN_OUT_SMALL_PIN, REL_FAN_OUT_SMALL_INIT);       /* Tür-Relais an, damit Ruhestromtür verschlossen bleibt. NC -> Tuer offen */
	Serial.print("     ");
	Serial.print("Small Fan OUT Pin ["); Serial.print(REL_FAN_OUT_SMALL_PIN); Serial.print("] set to "); Serial.println(REL_FAN_OUT_SMALL_INIT);
  delay(20);
  // Relay 3
	relay.digitalWrite(REL_FAN_IN_SMALL_PIN, REL_FAN_IN_SMALL_INIT);       /* Tür-Relais an, damit Ruhestromtür verschlossen bleibt. NC -> Tuer offen */
	Serial.print("     ");
	Serial.print("Small Fan IN Pin ["); Serial.print(REL_FAN_IN_SMALL_PIN); Serial.print("] set to "); Serial.println(REL_FAN_IN_SMALL_INIT);
  delay(20);
  // Relay 4
	relay.digitalWrite(REL_FOG_PIN, REL_FOG_INIT);       /* Tür-Relais an, damit Ruhestromtür verschlossen bleibt. NC -> Tuer offen */
	Serial.print("     ");
	Serial.print("Fog Pin ["); Serial.print(REL_FOG_PIN); Serial.print("] set to "); Serial.println(REL_FOG_INIT);
  delay(20);
  // Relay 5
	relay.digitalWrite(REL_KEYPAD_PIN, REL_KEYPAD_INIT);       /* Tür-Relais an, damit Ruhestromtür verschlossen bleibt. NC -> Tuer offen */
	Serial.print("     ");
	Serial.print("Keypad Pin ["); Serial.print(REL_KEYPAD_PIN); Serial.print("] set to "); Serial.println(REL_KEYPAD_INIT);
  delay(20);
  // Relay 6
	relay.digitalWrite(REL_MAGNET_PIN, REL_MAGNET_INIT);       /* Tür-Relais an, damit Ruhestromtür verschlossen bleibt. NC -> Tuer offen */
	Serial.print("     ");
	Serial.print("Magnet Pin ["); Serial.print(REL_MAGNET_PIN); Serial.print("] set to "); Serial.println(REL_MAGNET_INIT);

	return true;
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

void software_Reset()
{
  Serial.println(F("Restarting in"));
  delay(250);
  for (byte i = 3; i>0; i--) {
    Serial.println(i);
    delay(1000);
  }
  asm volatile ("  jmp 0");
}
