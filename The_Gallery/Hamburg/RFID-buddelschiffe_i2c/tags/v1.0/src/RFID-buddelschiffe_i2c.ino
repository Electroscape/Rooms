/*==========================================================================================================*/
/*		2CP - TeamEscape - Engineering
 *		by Robert Schloezer
 *
 *		v1.0 alpha
 *		- Ein RFID-I2C Modul
 *		- Soll RFID Karte auslesen
 *
 */
/*==========================================================================================================*/

const String title = String("RFID-I2C v1.0 alpha");


/*==INCLUDE=================================================================================================*/
// I2C Port Expander
#include "PCF8574.h"                  /* https://github.com/skywodd/pcf8574_arduino_library - modifiziert!  */
                                      /* Der Klassenname wurde geaendert, da es sonst Namenskonflikte gibt! */

#include "Wire.h"
extern "C" {
#include "utility/twi.h"  // from Wire library, so we can do bus scanning
}
// OLED
#include "SSD1306Ascii.h"             /* https://github.com/greiman/SSD1306Ascii                            */
#include "SSD1306AsciiWire.h"         /* https://github.com/greiman/SSD1306Ascii                            */

// RFID
//#include <PN532.h>                    /* https://github.com/elechouse/PN532                                 */
//#include <PN532_I2C.h>                /* https://github.com/elechouse/PN532                                 */
#include <STB-PN532.h>                    /* https://github.com/elechouse/PN532                                 */
#include <STB-PN532_I2C.h>                /* https://github.com/elechouse/PN532                                 */

// FastLED
#include "FastLED.h"


/*==DEFINE==================================================================================================*/
// Standards der Pinbelegung (Konvention)
	// Multiplexer
		#define MULTI_I2C_ADD				 0x70
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
    #define RFID_I2C_ADD         0x24     /* Ist durch Hardware des Readers vorgegeben									*/

		// LED
		#define CHIPSET       WS2812B       // LED Chip
		#define COLOR_ORDER   GRB           // Farbreihenfolge - STD: GRB

		#define DATA_PIN        3           // PWM Pin
		#define NUM_LEDS       64           // Anzahl der LED
		#define BRIGHTNESS    255           // Maximale Helligkeit

// ______________________EINSTELLUNGEN______________________
	// Pinbelegung
		#define EXP_MAGNET_PIN          0     /* Relay: Magnet 			 																				*/
	// Keypad
		#define KEYPAD_I2C_ADD       0x38     /* moeglich sind 0x38, 39, 3A, 3B, 3D                         */


/*==OLED====================================================================================================*/
SSD1306AsciiWire oled;
bool UpdateOLED = true;
unsigned long UpdateOLEDAfterDelayTimer = 0;
const int UpdateOLEDAfterDelay = 5000;         /* Zeit, bis Display kurz flackert als Online Signal			*/
String magnetString = String("ON");

/*==RFID====================================================================================================*/
const byte ANZ_READER = 4;
const byte MULTI_PORT[ANZ_READER] = {0, 1, 2, 3};
const String RFID_KeywordCode[] = {"SD",           "AH",           "GF",           "PA"           };
const byte uidArraySize = 7;

PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532i2c);

struct {
	bool cardOnReader;
	byte UID[uidArraySize];
	String cardString;
}readerState[ANZ_READER] {false, {0,0,0,0,0,0,0}, ""};

/*==PCF8574=================================================================================================*/
Expander_PCF8574 relay;

/*==LED=====================================================================================================*/
// Flags
bool light_on  = false;             // Ist das Licht schon an
bool light_off = true;              // Ist das Licht ausgeschaltet

// HSV Werte - global
const byte satur_min  =   0;        // Minimale Saettigung bei Farbwechsel
const byte bright_min = 150;        // Minimale Helligkeit bei Farbwechsel
      byte color      =   0;        // Farbe
      byte satur      =   0;        // Saettigung
      byte bright     =   0;        // Helligkeit

byte defined_color[] =  {0,   160,  96,  64, 0};	// rot, blau, gruen, gelb, schwarz
byte defined_sat[]		= {255, 255, 255, 255, 0};
byte defined_brght[]	= {255, 255, 255, 255, 0};

// Farbeinstellungen
byte warmweiss[3]   = { 20, 130, 255};
byte rot[3]         = {  0, 255, 255};
byte gelb[3]        = { 64, 255, 255};
byte blau[3]        = {160, 255, 255};
byte gruen[3]       = { 96, 255, 255};
byte lila[3]        = {192, 255, 255};

// Led Typ
CRGB leds[NUM_LEDS];

// Feuer
CRGBPalette16 gPal;
bool gReverseDirection = false;

/*============================================================================================================
//===SETUP====================================================================================================
//==========================================================================================================*/

void setup() {

  Serial.begin(115200);

  if( OLED_Init() )									{Serial.println("OLED:   ok");}

	if( RFID_I2C_Init(ANZ_READER) ) 	{Serial.println("RFID:   ok");}

	if( Mother_Init() )								{Serial.println("Mother: ok");}

	if( ledInit() )										{Serial.println("LED:    ok");}

	OLED_homescreen();

	Serial.println(); Serial.println("===================START====================="); Serial.println();
	delay(1000);
}

/*============================================================================================================
//===LOOP=====================================================================================================
//==========================================================================================================*/

void loop() {
  RFID_I2C_Update();
}

/*============================================================================================================
//===RFID-I2C=================================================================================================
//==========================================================================================================*/
void RFID_I2C_Update() {
	for( int i=0; i<ANZ_READER; i++) {
		tcaselect(MULTI_PORT[i]);

		if( RFID_I2C_readCard(i, readerState[i].cardOnReader) ) {
			Serial.print("Reader ["); Serial.print(i); Serial.println("]:");
			Serial.print("Karte: "); Serial.println(readerState[i].cardOnReader);
			Serial.print("UID: ");
			for(int j=0; j<=uidArraySize; j++) {
				Serial.print(readerState[i].UID[j], HEX);
			}
			Serial.println();
			Serial.print("Inhalt: "); Serial.println(readerState[i].cardString); Serial.println();
			//lichtsteuerung(defined_color[colorSelect(readerState[i].cardString)], defined_sat[colorSelect(readerState[i].cardString)], defined_brght[colorSelect(readerState[i].cardString)], i);
		}
	}
}

bool RFID_I2C_readCard(byte readerNumber, bool lastCardState) {
	uint8_t success = 0;                          						// Flag to check if there was an error with the PN532
  uint8_t uid[uidArraySize] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        						// Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  uint8_t currentblock;                     						// Counter to keep track of which block we're on
  //uint8_t data[16];                         						// Array to store block data during reads
  uint8_t keyuniversal[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	bool authenticated = false;
	bool cardStateChange = false;

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
	memcpy( readerState[readerNumber].UID, uid, uidArraySize );

	if (success && uidLength == 4) {
		for (currentblock = 1; currentblock < 2; currentblock++) {
			success = nfc.mifareclassic_AuthenticateBlock (uid, uidLength, currentblock, 1, keyuniversal);

			if (success) {
				uint8_t data[16];                         						// Array to store block data during reads
				success = nfc.mifareclassic_ReadDataBlock(currentblock, data);

				if (success) {
					String str = "";
    			for (uint8_t i = 0; i < 16; i++) {
        		char c = data[i];
        		if (c <= 0x1f || c > 0x7f) {}
						else {str += char(c);}
    			}

					if ( readerState[readerNumber].cardString == str ) {
						readerState[readerNumber].cardOnReader = true;
						readerState[readerNumber].cardString = str;
						return false;
					}
					else {
						readerState[readerNumber].cardOnReader = true;
						readerState[readerNumber].cardString = str;
						return true;
					 }
				}

				else {}
      }

			else {}
    }
	}

	else if (success && uidLength == 7) {
		for (currentblock = 1; currentblock < 2; currentblock++) {
			success = nfc.mifareclassic_AuthenticateBlock (uid, uidLength, currentblock, 1, keyuniversal);

			if (success) {
				uint8_t data[32];                         						// Array to store block data during reads
				success = nfc.mifareultralight_ReadPage (4, data);

				if (success) {
					String str = "";
    			for (uint8_t i = 0; i < 4; i++) {
        		char c = data[i];
        		if (c <= 0x1f || c > 0x7f) {}
						else {str += char(c);}
    			}

					if ( readerState[readerNumber].cardString == str ) {
						readerState[readerNumber].cardOnReader = true;
						readerState[readerNumber].cardString = str;
						return false;
					}
					else {
						readerState[readerNumber].cardOnReader = true;
						readerState[readerNumber].cardString = str;
						return true;
					 }
				}

				else {}
      }

			else {}
    }
	}

	else {
		readerState[readerNumber].cardOnReader = false;
		readerState[readerNumber].cardString = "NO_CARD";
	}

	if (readerState[readerNumber].cardOnReader != lastCardState) {
		cardStateChange = true;
		}
	else if (readerState[readerNumber].cardOnReader == lastCardState) {
		cardStateChange = false;
		}

	return cardStateChange;
}

// INITIALISIERUNG
bool RFID_I2C_Init(byte readerQuantity) {
	for(int i=0; i<readerQuantity; i++) {
	Serial.print("     RFID["); Serial.print(i); Serial.print("]: ");
	//oled.print("RFID["); oled.print(i); oled.print("]: ");

	tcaselect(MULTI_PORT[i]);

  nfc.begin();

	delay(100);

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
		Serial.println(); Serial.println("Kein RFID-Reader gefunden");
		software_Reset();
    while (1); // halt
  }
	nfc.setPassiveActivationRetries(0xFF);
  nfc.SAMConfig();    // Konfiguration für um RFID Tags zu lesen
  Serial.println("ok");
	//oled.println("ok");
	}
return true;
}

/*============================================================================================================
//===RELAY====================================================================================================
//==========================================================================================================*/
bool Mother_Init() {
relay.begin(RELAY_I2C_ADD);
for (int i=0; i<=7; i++) {
	 relay.pinMode(i, OUTPUT);
	 relay.digitalWrite(i, HIGH);
}
Serial.print("     MOTHER: "); Serial.println("Pins auf HIGH gezogen");
delay(250);
relay.digitalWrite(EXP_MAGNET_PIN, LOW);      /* Licht-Relais an, damit Licht initial aus.              NC -> Licht an   */
Serial.print("     MOTHER: "); Serial.println("Magnet Pin auf LOW gezogen");
delay(500);

return true;
}

/*============================================================================================================
//===MULTIPLEXER==============================================================================================
//==========================================================================================================*/
void tcaselect(uint8_t i) {
  if (i > 7) return;

  Wire.beginTransmission(MULTI_I2C_ADD);
  Wire.write(1 << i);
  Wire.endTransmission();
}

/*============================================================================================================
//===OLED=====================================================================================================
//==========================================================================================================*/
bool OLED_Init() {
  Wire.begin();
	Wire.setClock(400000);
  oled.begin(&Adafruit128x64, OLED_I2C_ADD);
  oled.set400kHz();
  oled.setScroll(true);
  oled.setFont(System5x7);

  print_logo_infos(title);

  i2c_scanner();
	return true;
}

void OLED_homescreen() {
	oled.clear();
	oled.setFont(Adafruit5x7);
	oled.println();
	oled.setFont(Arial_bold_14);
	oled.println();
	oled.print(F("  RFID-I2C-TEST "));
}

void OLED_RFID_status(bool KartenStatus, int readerNr, byte anzahlReader) {
	if(anzahlReader == 1) {
		oled.clear();
		oled.setFont(Adafruit5x7);
		oled.println(F("RFID-I2C-TEST"));
		oled.setFont(Arial_bold_14);
		oled.println();
		oled.print("RFID-Reader["); oled.print(readerNr); oled.print("]: ");
		if (KartenStatus){
			oled.print("DA");
		}
		else {
			oled.print("WEG");
		}
	}
	else if(anzahlReader == 2) {
		oled.clear();
		oled.setFont(Adafruit5x7);
		oled.println(F("RFID-I2C-TEST"));
		oled.setFont(Arial_bold_14);
		oled.println();
		oled.print("RFID-Reader[1]: ");
		if(readerNr == 0) {
			if (KartenStatus){
				oled.println("DA");
			}
			else {
				oled.println("WEG");
			}
		}
		oled.println("RFID-Reader[2]: ");
		if(readerNr == 1) {
			if (KartenStatus){
				oled.println("DA");
			}
			else {
				oled.println("WEG");
			}
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
  delay(1000);
  oled.clear();
}

// I2C Scanner - scannt nach angeschlossenen I2C Geraeten
void i2c_scanner() {
  Serial.println (F("I2C Scan:"));
  oled.setFont(System5x7);
  oled.println   (F("I2C Scan..."));
  byte wire_device_count = 0;

  for (byte i = 8; i < 120; i++) {
    Wire.beginTransmission (i);
    if (Wire.endTransmission () == 0) {
      Serial.print   (F("     Adresse: "));
      Serial.print   (i, DEC);
      Serial.print   (F(" (0x"));
      oled.print     (F(" (0x"));
      Serial.print   (i, HEX);
      oled.print     (i, HEX);
      Serial.print (F(")"));
      oled.print   (F(")"));
			if (i == 36) {Serial.print(F(" -> RFID")); oled.print(" -> RFID");}
			if (i == 56) {Serial.print(F(" -> I2C Board (A0=A1=A2=GND)")); oled.print(" -> I2C Board");}
			if (i == 57) {Serial.print(F(" -> I2C Board (A0=VCC,A1=A2=GND)")); oled.print(" -> I2C Board");}
      if (i == 60) {Serial.print(F(" -> Display")); oled.print(" -> Display");}
      if (i == 63) {Serial.print(F(" -> Relay")); 	oled.print(" -> Relay");}
			if (i == 112) {Serial.print(F(" -> Multiplexer")); oled.print(" -> Multiplexer");}
      Serial.println();
      oled.println();
      wire_device_count++;
      delay (1);
    }
  }
	Serial.print   (F("     "));
  Serial.print   (wire_device_count, DEC);
  oled.print     (wire_device_count, DEC);
  Serial.println (F(" Devices"));
  oled.println   (F(" Devices"));
  delay(500);
	oled.clear();
}

void tca_scanner() {
	for (uint8_t t=0; t<8; t++) {
      tcaselect(t);
      Serial.print("TCA Port #"); Serial.println(t);

      for (uint8_t addr = 0; addr<=127; addr++) {
        if (addr == MULTI_I2C_ADD) continue;

        uint8_t data;
        if (! twi_writeTo(addr, &data, 0, 1, 1)) {
           Serial.print("Found I2C 0x");  Serial.println(addr,HEX);
        }
      }
    }
    Serial.println("\ndone");
}

// Software Reset - Startet den Arduino neu
void software_Reset() {
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

/*============================================================================================================
//===LED======================================================================================================
//==========================================================================================================*/
byte colorSelect(String schiffFarbe) {
	if (schiffFarbe == "SD") {return 0;}
	else if (schiffFarbe == "AH") {return 1;}
	else if (schiffFarbe == "GF") {return 2;}
	else if (schiffFarbe == "PA") {return 3;}
	else {return 4;}
}

void lichtsteuerung( byte color_par, byte satur_par, byte bright_par, byte readerNumber ) {
  if (readerNumber == 0) {
		for( int i=0; i<8; i++) {
			leds[i] = CHSV(color_par, satur_par, bright_par);
		}
		for( int i=56; i<64; i++) {
			leds[i] = CHSV(color_par, satur_par, bright_par);
		}
	}
	else if (readerNumber == 1) {
		for( int i=8; i<16; i++) {
			leds[i] = CHSV(color_par, satur_par, bright_par);
		}
		for( int i=48; i<56; i++) {
			leds[i] = CHSV(color_par, satur_par, bright_par);
		}
	}
	else if (readerNumber == 2) {
		for( int i=16; i<24; i++) {
			leds[i] = CHSV(color_par, satur_par, bright_par);
		}
		for( int i=40; i<48; i++) {
			leds[i] = CHSV(color_par, satur_par, bright_par);
		}
	}
	else if (readerNumber == 3) {
		for( int i=24; i<32; i++) {
			leds[i] = CHSV(color_par, satur_par, bright_par);
		}
		for( int i=32; i<40; i++) {
			leds[i] = CHSV(color_par, satur_par, bright_par);
		}
	}

	FastLED.show();
}

bool ledInit() {
  delay(100);
	FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  LEDS.setBrightness(BRIGHTNESS);

  // Aufblenden
  for(int i=0; i<=255; i++) {
    FastLED.showColor( CHSV(0, 255, i) );
  }
  // Farbe wechseln
  for(int i=0; i<=255; i++) {
    FastLED.showColor( CHSV(i, 255, 255) );
  }
  // Abblenden
  for(int i=255; i>=0; i--) {
    FastLED.showColor( CHSV(0, 255, i) );
  }

	// Farbpalette fuer Feuer Funktion
	gPal = CRGBPalette16( CRGB::Black, CRGB::Red, CRGB::OrangeRed, CRGB::Orange);

  return true;
}
