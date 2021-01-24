#include <FastLED.h>
#include <DFRobotDFPlayerMini.h>
#include <SoftwareSerial.h>
#include "PCF8574.h"

// INPUT
#define BUTTON_0_PIN    0     //2
#define BUTTON_1_PIN    1     //10

// LED
#define LED_STRIP WS2812B
#define MAX_DIMENSION ((kMatrixWidth>kMatrixHeight) ? kMatrixWidth : kMatrixHeight) // wenn Breite größer Höhe -> MAX_DIMENSION=kMatrixWidth, sonst MAX_DIMENSION=kMatrixHeight
#define NUM_LEDS (kMatrixWidth * kMatrixHeight)
#define LED_PIN 3

// RELAY PIN
#define RELAY_I2C_ADD     	 0x3F     // Relay Expander
#define REL_ALARM            0        //4
#define REL_LUFT_GR          1        //5
#define REL_LUFT_KL          2        //6
#define REL_ZULUFT_KL        3        //7
#define REL_NEBEL            4        //8
#define REL_KEYPAD           5        //11
#define REL_MAGNET           6        //12

// Trigger Input (öffnungsmelder)
#define DETECTOR_I2C_ADD     	 0x38     // door opening detector Expander

bool ersterDurchlauf = false;

// Params for width and height
const uint8_t kMatrixWidth = 1;
const uint8_t kMatrixHeight = 64;

// Param for different pixel layouts
const bool    kMatrixSerpentineLayout = true;
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

// The leds
CRGB leds[kMatrixWidth * kMatrixHeight];

// The 32bit version of our coordinates
static uint16_t x;
static uint16_t y;
static uint16_t z;

uint16_t speed = 5; 	// a nice starting speed, mixes well with a scale of 100
uint16_t scale = 100;	// "Entfernung"

// This is the array that we keep our computed noise values in
uint8_t noise[MAX_DIMENSION][MAX_DIMENSION];

/*==PCF8574=================================================================================================*/
Expander_PCF8574 relay;
Expander_PCF8574 iTrigger;

void setup() {
  Serial.begin(115200);
  Serial.println("resetting!");
  //pinMode(BUTTON_0_PIN, INPUT);
  Serial.println("Trigger begin:");
  iTrigger.begin(DETECTOR_I2C_ADD);
  iTrigger.pinMode(BUTTON_1_PIN, INPUT);
  Serial.println("Trigger Done");
//  pinMode(BUTTON_2_PIN, INPUT);
//  pinMode(BUTTON_3_PIN, INPUT);

//TODO RelayChange:
Serial.println("Expander begin: start");
relay.begin(RELAY_I2C_ADD);
Serial.println("Expander begin: ok");

for (int i=0; i<=6; i++) {
   relay.pinMode(i, OUTPUT);
   relay.digitalWrite(i, HIGH);
}

//   pinMode(REL_LUFT_KL, OUTPUT);
//   pinMode(REL_ZULUFT_KL, OUTPUT);
//   pinMode(REL_ALARM, OUTPUT);
//   pinMode(REL_LUFT_GR, OUTPUT);
//   pinMode(REL_NEBEL, OUTPUT);
//   pinMode(REL_KEYPAD, OUTPUT);
//   pinMode(REL_MAGNET, OUTPUT);
//   digitalWrite(REL_LUFT_KL, HIGH);
//   digitalWrite(REL_ZULUFT_KL, HIGH);
//   digitalWrite(REL_ALARM, HIGH);
//   digitalWrite(REL_LUFT_GR, HIGH);
//   digitalWrite(REL_NEBEL, HIGH);
//   digitalWrite(REL_KEYPAD, HIGH);
//   digitalWrite(REL_MAGNET, HIGH);

  Serial.println("Pins auf HIGH gezogen");

  delay(2000);

  LEDS.addLeds<LED_STRIP,LED_PIN,GRB>(leds,NUM_LEDS);
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
delay(2000);

for(int i = 0; i < kMatrixWidth; i++) {
      for(int j = 0; j < kMatrixHeight; j++) {
          leds[XY(i,j)] = CHSV(40,255,255);
      }
}
FastLED.show();
delay(2000);

for(int i = 0; i < kMatrixWidth; i++) {
      for(int j = 0; j < kMatrixHeight; j++) {
          leds[XY(i,j)] = CHSV(120,255,255);
      }
}
FastLED.show();
delay(2000);

for(int i = 0; i < kMatrixWidth; i++) {
      for(int j = 0; j < kMatrixHeight; j++) {
          leds[XY(i,j)] = CHSV(200,255,255);
      }
}
FastLED.show();
delay(2000);

for(int i = 0; i < kMatrixWidth; i++) {
      for(int j = 0; j < kMatrixHeight; j++) {
          leds[XY(i,j)] = CHSV(20,255,255);
      }
}
FastLED.show();
delay(2000);

  for(int i = 0; i < kMatrixWidth; i++) {
      for(int j = 0; j < kMatrixHeight; j++) {
          leds[XY(i,j)] = CHSV(170,95,255);
      }
}
FastLED.show();
}


// Fill the x/y array of 8-bit noise values using the inoise8 function.
void fillnoise8() {
  for(int i = 0; i < MAX_DIMENSION; i++) {
    int ioffset = scale * i;
    for(int j = 0; j < MAX_DIMENSION; j++) {
      int joffset = scale * j;
      noise[i][j] = inoise8(x + ioffset,y + joffset,z);
    }
  }
  z += speed;
}


void loop() {
	//while(!iTrigger.digitalRead(BUTTON_0_PIN)){
  Serial.println("Door Close");
	if(!iTrigger.digitalRead(BUTTON_1_PIN) && !ersterDurchlauf) {
    Serial.println("Tuer offen");
    delay(2000);                              // 12.04.
    Serial.println("Tuer noch offen, starte Durchlauf");
    if(!iTrigger.digitalRead(BUTTON_1_PIN)) {          // 12.04.
    ersterDurchlauf = true;

    relay.digitalWrite(REL_MAGNET, LOW);
    Serial.println("Reinraum verriegelt");
    delay(500);
    relay.digitalWrite(REL_KEYPAD, LOW);
    Serial.println("Reinraum Keypad abgeschaltet");
    delay(2500);                              // 12.04.
		for(int i = 0; i < kMatrixWidth; i++) {
		      for(int j = 0; j < kMatrixHeight; j++) {
		          leds[XY(i,j)] = CHSV(0,255,0);  //leds[XY(i,j)] = CHSV(0,255,255);
		      }
		}
                FastLED.show();
                relay.digitalWrite(REL_ALARM, LOW);
                Serial.println("Licht aus");
                delay(1000);
                relay.digitalWrite(REL_LUFT_KL, LOW);
                Serial.println("Abluft - klein - an");
                relay.digitalWrite(REL_ZULUFT_KL, LOW);
                Serial.println("Zuluft an");

                delay(4000);    //delay(2000);

    for(int z=0; z<35; z++){
      if (z<5) {relay.digitalWrite(REL_NEBEL, LOW); Serial.println("Nebel an");}
      else {relay.digitalWrite(REL_NEBEL, HIGH); Serial.println("Nebel aus");}
      if (z>5) {relay.digitalWrite(REL_LUFT_GR, LOW); Serial.println("Abluft - gross - an");}

      for(int i = 0; i < kMatrixWidth; i++) {
  		      for(int j = 0; j < kMatrixHeight; j++) {
  		          leds[XY(i,j)] = CHSV(170,95,255);
  		      }
  		}
                  FastLED.show();
                  relay.digitalWrite(REL_ALARM, LOW);
                  delay(1000);
      for(int i = 0; i < kMatrixWidth; i++) {
            for(int j = 0; j < kMatrixHeight; j++) {
                leds[XY(i,j)] = CHSV(0,255,255);
        		 }
      }
                  FastLED.show();
                  relay.digitalWrite(REL_ALARM, HIGH);
                  delay(1000);
    }

// Gruenes Licht -----------------------------------------------
		for(int i = 0; i < kMatrixWidth; i++) {
				      for(int j = 0; j < kMatrixHeight; j++) {
				          leds[XY(i,j)] = CHSV(96,255,255);
				      }
				}
		FastLED.show();
    Serial.println("Gruenes Licht an");

                delay(5000);
                relay.digitalWrite(REL_LUFT_KL, HIGH);
                Serial.println("Abluft - klein - aus");
                relay.digitalWrite(REL_ZULUFT_KL, HIGH);
                Serial.println("Zuluft aus");
                relay.digitalWrite(REL_ALARM, HIGH);
                Serial.println("Licht an");
                relay.digitalWrite(REL_KEYPAD, HIGH);
                Serial.println("Reinraum Keypad an");

// Weisses Licht -----------------------------------------------
		for(int i = 0; i < kMatrixWidth; i++) {
				      for(int j = 0; j < kMatrixHeight; j++) {
				          leds[XY(i,j)] = CHSV(170,95,255);
				      }
				}
                FastLED.show();

                delay(6000);
                relay.digitalWrite(REL_MAGNET, HIGH);
                Serial.println("Reinraum Verriegelung aus");
                delay(9000);
                relay.digitalWrite(REL_LUFT_GR, HIGH);
                Serial.println("Abluft - gross - aus");

	}
}

if( ersterDurchlauf && iTrigger.digitalRead(BUTTON_1_PIN) ){
  Serial.println("Labortuer geschlossen");
  delay(180000);
  if(iTrigger.digitalRead(BUTTON_1_PIN)){
    Serial.println("Labortuer laenger als 3 Minuten geschlossen -> Reset");
    software_Reset();
  }
}

}

void software_Reset()
{
  Serial.println(F("Neustart in"));
  delay(250);
  for (byte i = 3; i>0; i--) {
    Serial.println(i);
    delay(1000);
  }
  asm volatile ("  jmp 0");
}
