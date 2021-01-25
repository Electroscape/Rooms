/*==========================================================================================================*/
/*		2CP - TeamEscape - Engineering
 *		by Stephan Christoph & Robert Schloezer
 *
 *		v1.3 beta
 *		- Bis zu 4 MFRC522 Reader, inkl. LED Strip/Reader fuer Feedback
 *		- OLED Display ueber I2C
 *		- Schaltet Deckenlicht und Schwarzlicht ein und aus
 */
/*==========================================================================================================*/

const String title = String("RFID Multi-RFID v1.4 beta");


/*==INCLUDE=================================================================================================*/
// RFID
#include <SPI.h>                      /* Standardbibliothek                                                 */
#include <MFRC522.h>                  /* https://github.com/miguelbalboa/rfid                               */

// NeoPixel
#include <Adafruit_NeoPixel.h>        /* Ueber Bibliotheksverwalter                                         */

// OLED
#include "SSD1306Ascii.h"             /* https://github.com/greiman/SSD1306Ascii                            */
#include "SSD1306AsciiWire.h"         /* https://github.com/greiman/SSD1306Ascii                            */

// I2C Port Expander
#include "PCF8574.h"                  /* https://github.com/skywodd/pcf8574_arduino_library - modifiziert!  */
                                      /* Der Klassenname wurde geaendert, da es sonst Namenskonflikte gibt! */

/*==DEFINE==================================================================================================*/
// RFID
#define RFID_NR_OF_READERS      4     /* Anzahl der angeschlossenen Reader                                  */

#define RFID_RST_PIN           10     /* Per Konvention auf 10 festgelegt                                   */

#define RFID_1_SS_PIN           8     /* Per Konvention ist dies RFID-Port 1                                */
#define RFID_2_SS_PIN           7     /* Per Konvention ist dies RFID-Port 2                                */
#define RFID_3_SS_PIN           4     /* Per Konvention ist dies RFID-Port 3                                */
#define RFID_4_SS_PIN           2     /* Per Konvention ist dies RFID-Port 4                                */

// NeoPixel
#define NEOPIXEL_NR_OF_PIXELS   1     /* Anzahl der Pixel auf einem Strang (Test 1 Pixel)                   */

#define RFID_1_LED_PIN          9     /* Per Konvention ist dies RFID-Port 1                                */
#define RFID_2_LED_PIN          6     /* Per Konvention ist dies RFID-Port 2                                */
#define RFID_3_LED_PIN          5     /* Per Konvention ist dies RFID-Port 3                                */
#define RFID_4_LED_PIN          3     /* Per Konvention ist dies RFID-Port 4                                */

// OLED
#define OLED_I2C_ADD         0x3C     /* Ist durch Hardware des OLEDs vorgegeben! Nicht Aenderbar!          */

// Relayboard
#define RELAY_I2C_ADD     	 0x3F     /* Relay Expander - Per Konvention festgelegt - alle Pis auf HIGH     */

#define EXP_SCHW_LI_PIN         4     /* kann beliebig auf dem Relaisboard geaendert werden                 */
#define EXP_LICHT_PIN           5     /* kann beliebig auf dem Relaisboard geaendert werden                 */

/*==OLED====================================================================================================*/

SSD1306AsciiWire oled;

/*==RFID & NEOPIXEL=========================================================================================*/

const String RFID_KeywordCode[]            = {"SD",           "AH",           "GF",           "PA"           };

byte   RFID_SSPins[]                       = {RFID_1_SS_PIN,  RFID_2_SS_PIN,  RFID_3_SS_PIN,  RFID_4_SS_PIN  };
byte   RFID_LEDPins[]                      = {RFID_1_LED_PIN, RFID_2_LED_PIN, RFID_3_LED_PIN, RFID_4_LED_PIN };
bool   RFID_KeywordIsRight[]               = {false,          false,          false,          false          };
bool   RFID_CardIsOnReader[]               = {false,          false,          false,          false          };
bool   RFID_CardLeavedReader[]             = {false,          false,          false,          false          };
bool   RFID_Finished                       =  false;

MFRC522 RFID_Reader[RFID_NR_OF_READERS];
MFRC522::MIFARE_Key RFID_Key;
MFRC522::StatusCode RFID_Status;

Adafruit_NeoPixel LED_Stripe_1 = Adafruit_NeoPixel(NEOPIXEL_NR_OF_PIXELS, RFID_1_LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel LED_Stripe_2 = Adafruit_NeoPixel(NEOPIXEL_NR_OF_PIXELS, RFID_2_LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel LED_Stripe_3 = Adafruit_NeoPixel(NEOPIXEL_NR_OF_PIXELS, RFID_3_LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel LED_Stripe_4 = Adafruit_NeoPixel(NEOPIXEL_NR_OF_PIXELS, RFID_4_LED_PIN, NEO_GRB + NEO_KHZ800);

/*==PCF8574=================================================================================================*/

Expander_PCF8574 relay;

/*============================================================================================================
//===SETUP====================================================================================================
//==========================================================================================================*/

void setup() {

  Serial.begin(115200);

  OLED_Init();

  RFID_Init();

  Relayboard_Init();

}

/*============================================================================================================
//===LOOP=====================================================================================================
//==========================================================================================================*/

void loop() {

  RFID_Loop();

}

/*============================================================================================================
//===INIT=====================================================================================================
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

  //SelfTest - If false LED red or other Indicator...
  //DOES NOT WORK AT THE MOMENT
  /*
    for (uint8_t reader_nr = 0; reader_nr < RFID_NR_OF_READERS; reader_nr++) {
    Serial.print(F("Reader[")); Serial.print(reader_nr + 1); Serial.print(F("] SelfTest"));
    if (RFID_Reader[reader_nr].PCD_PerformSelfTest()) {
      Serial.println(F(" ok!"));
      NeoPixel_StripeOn(reader_nr, RFID_KeywordCode[reader_nr]);
      delay(500);
      NeoPixel_StripeOff(reader_nr);
      RFID_Reader[reader_nr].PCD_StopCrypto1();
    }
    else {
      Serial.println(F("] fail."));
      NeoPixel_StripeOn(reader_nr, "rot");
    }
    }
  */

  //Prepare RFID_Key - All Keys are set to FFFFFFFFFFFFh at chip delivery from the factory!
  //We will not change it!!! We use this Key!!!
  for (byte i = 0; i < 6; i++) RFID_Key.keyByte[i] = 0xFF;

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

// I2C Scanner - scannt nach angeschlossenen I2C Ger�ten
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
  if (color_str == "AA") {
    color = LED_Stripe_1.Color(  0,   0, 255); //blau - SD
  } else
  if (color_str == "AB") {
    color = LED_Stripe_1.Color(255,   0,   0); //rot - AH
  } else
  if (color_str == "AC") {
    color = LED_Stripe_1.Color(  0, 255,   0); //gruen - GF
  } else
  if (color_str == "AD") {
    color = LED_Stripe_1.Color(255, 150,   0); //gelb - PA
  } else
	if (color_str == "SD") {
    color = LED_Stripe_1.Color(255, 70,   0); //gold
  } else
  if (color_str == "AH") {
    color = LED_Stripe_1.Color(255, 70,   0); //gold
  } else
  if (color_str == "GF") {
    color = LED_Stripe_1.Color(255, 70,   0); //gold
  } else
  if (color_str == "PA") {
    color = LED_Stripe_1.Color(255, 70,   0); //gold
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

void NeoPixel_StripeEndGame(byte i) {

  long int color_green = LED_Stripe_1.Color(  0,   255,   0);

  if (i == 0) {
    LED_Stripe_1.setPixelColor(0, color_green);
    LED_Stripe_1.show();
  }
  if (i == 1) {
    LED_Stripe_2.setPixelColor(0, color_green);
    LED_Stripe_2.show();
  }
  if (i == 2) {
    LED_Stripe_3.setPixelColor(0, color_green);
    LED_Stripe_3.show();
  }
  if (i == 3) {
    LED_Stripe_4.setPixelColor(0, color_green);
    LED_Stripe_4.show();
  }
}

void NeoPixel_FadeAllRightStripes() {



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

/*============================================================================================================
//===RELAYBOARD===============================================================================================
//==========================================================================================================*/
void Relayboard_Init() {

  Serial.print(F("Relayboard "));
  oled.print(F("Relayboard "));
  relay.begin(RELAY_I2C_ADD);
  for (int i=0; i<=7; i++) {
     relay.pinMode(i, OUTPUT);
     relay.digitalWrite(i, HIGH);
  }
  relay.digitalWrite(EXP_LICHT_PIN, HIGH); //Licht an
  relay.digitalWrite(EXP_SCHW_LI_PIN, LOW); //Licht an
  Serial.println(F("ok.")); oled.println(F("ok."));
  for (uint8_t reader_nr = 0; reader_nr < RFID_NR_OF_READERS; reader_nr++) {
    NeoPixel_StripeOn(reader_nr, RFID_KeywordCode[reader_nr]);
  }
  delay(250);
  for (uint8_t reader_nr = 0; reader_nr < RFID_NR_OF_READERS; reader_nr++) {
    NeoPixel_StripeOff(reader_nr);
  }
  Serial.println(); oled.println();

}

/*============================================================================================================
//===RFID=====================================================================================================
//==========================================================================================================*/

//For UID-Dump
void RFID_dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

/*==========================================================================================================*/

boolean RFID_KeywordsAreAllRight() {
  boolean result = true;
  for (uint8_t reader_nr = 0; reader_nr < RFID_NR_OF_READERS; reader_nr++) {
    result = result && RFID_KeywordIsRight[reader_nr];
  }
  return result;
}

/*==========================================================================================================*/

void RFID_Loop() {

  //CHECK IF ALL KEYWORDS ARE RIGHT
  //Zuerst prüfen, ob alle Code-Wörter schon als "richtig" im Array markiert wurden. Wenn das der Fall ist und "RFID_Finished" noch nicht gesetzt wurde, wird dieser Teil ausgeführt.
  //Damit dieser Teil nur ein einziges mal ausgeführt wird, muss "RFID_Finished" hier auf true gesetzt werden. Danach wird alles in der RFID_Loop immer übersprungen - sie ist beendet.
  if (RFID_KeywordsAreAllRight() && !RFID_Finished) {

    //FINISCHED - DO NOTHING MORE WITH RFID, DO SOMETHING ELSE HERE FOR ONLY ONE TIME!!!
    Serial.println("RFID_Finished");
    RFID_Finished = true;

    //BLINK 5 TIMES
    delay(200);
    for (uint8_t i = 0; i < 2; i++) {
      //
      for (uint8_t reader_nr = 0; reader_nr < RFID_NR_OF_READERS; reader_nr++) {
        NeoPixel_StripeOff(reader_nr);
      }
      delay(250);
      for (uint8_t reader_nr = 0; reader_nr < RFID_NR_OF_READERS; reader_nr++) {
        NeoPixel_StripeOn(reader_nr, RFID_KeywordCode[reader_nr]);
      }
      delay(200);
    }

    RFID_EndGame();

  }
  //Dieser Teil wird immer nurchlaufen, sofern "RFID_Finisched" noch nicht gesetzt wurde. Also immer, so lange das Rätsel nicht gelöst wurde.
  else if (!RFID_Finished) {

    //Für alle 4 Reader wird diese For-Schleife einmal durchlaufen.
    //GO FOR ALL RFID READER
    for (uint8_t reader_nr = 0; reader_nr < RFID_NR_OF_READERS; reader_nr++) {

      // LOOK FOR NEW CARDS AND READ BY CARD SERIAL
      //Prüft, ob eine neue Karte aufgelegt wurde. Nur wenn eine Karte frisch aufgelegt wurde und die Seriennummer der Karte gelesen werden kann, geht es weiter.
      if (RFID_Reader[reader_nr].PICC_IsNewCardPresent() && RFID_Reader[reader_nr].PICC_ReadCardSerial() && (!RFID_CardIsOnReader[reader_nr])) {
        //Im ersten Block der Karte steht das Codewort, nur dieser Block wird ausgelesen - nicht mehr!
        byte block = 1;

        Serial.print(F("Reader[")); Serial.print(reader_nr + 1); Serial.print(F("] > "));
        oled.print(F("["));         oled.print(reader_nr + 1);   oled.print(F("] > "));

        //* AUTHENTICATION *
        //Alle Daten liegen IMMER verschlüsselt auf der Karte. Der Key der Karte ist IMMER FFFFFFFFFFFFh und dieser Key wird bei der Initialisierung der RFID-Reaer einmal erstellt und in "RFID_Key" gespeichert.
        //Wenn der Key richtig ist, sollte die Authentifizierung immer funktionieren. Wenn nicht, wird das später abgefangen.
        RFID_Status = RFID_Reader[reader_nr].PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 1, &RFID_Key, &(RFID_Reader[reader_nr].uid));
        if (RFID_Status == MFRC522::STATUS_OK) {

          // ***READ BLOCK FROM CARD***
          // need Buffer and Length
          //Die Authentifizierung war erfolgreich! Jetzt beginnt das lesen des ersten Blocks, in dem das Codewort steht.
          byte buffer_read[30];
          byte buffer_read_length = sizeof(buffer_read);
          RFID_Status = RFID_Reader[reader_nr].MIFARE_Read(block, buffer_read, &buffer_read_length);
          if (RFID_Status == MFRC522::STATUS_OK) {
            //Read Keyword from Block - all other Information will be kicked.
            String str = "";
            for (int i = 0; i < 16; i++) {
              if (buffer_read[i] == 0xFF) {
                break;
              }
              str += char(buffer_read[i]);
            }

            //PRINT CODEWORD FROM BLOCK AND CO
            //Hier sollte das Codewort erfolgreich gelesen worden sein und kann nun ausgegeben un weiter verarbeitet werden.
            Serial.print(str); oled.print(str);
            NeoPixel_StripeOn(reader_nr, str);
            RFID_CardIsOnReader[reader_nr] = true;

            //****CHECK KEYWORD****
            //Hier wird das Codewort verarbeitet und ggf. die Richtigkeit gespeichert.
            if (str == RFID_KeywordCode[reader_nr]) {
              RFID_KeywordIsRight[reader_nr] = true;
              Serial.print(F("\t> Right!")); oled.print(F(" > Right!"));
            }
            else {
              RFID_KeywordIsRight[reader_nr] = false;
              Serial.print(F("\t> Wrong!")); oled.print(F(" > Wrong!"));
            }
            //****END CHECK KEYWORD****

          }

          // ***ELSE READ BLOCK FROM CARD***
          //Das Lesen des ersten Blcoks war nicht erfolgreich. Das kann vorkommen, wenn die Karte zu weit weg ist, zu dicht dran ist oder sonst irgendwelche Funk-Probleme vorliegen.
          //Kein Problem. Neuer Versuch beim nächsten Durchlauf. Kein weiterer Handlungsbedarf.
          else {
            Serial.print("READ: Failed! "); oled.print("READ: Failed! ");
            Serial.print(RFID_Reader[reader_nr].GetStatusCodeName(RFID_Status));
            //do nothing and try again next run
          }
          // ***END READ BLOCK FROM CARD***

          Serial.println(); oled.println();
        }

        //* ELSE AUTHENTICATION *
        //Authentifizierung war nicht erfolgreich. Kann aufgrund eines Lesefehler auch vorkommen. Prinzipiell auch kein Drama, aber hier muss die Verbindung zum Reader gezielt unterbrochen werden
        //und die Schleife unterbrochen werden. Das System dahinter habe ich nicht ganz verstanden - die Funktionen der Bibliothe unterscheiden sich hier etwas in der Ausgabe. Wichtig für und: Gezielt unterbrechen, Loop und Kommunikation.
        else {
          //* BAD AUTHENTICATION *
          Serial.print(F("AUTH: Failed! ")); oled.println(F("AUTH: Failed! "));
          Serial.println(RFID_Reader[reader_nr].GetStatusCodeName(RFID_Status));
          //do nothing and try again next run
          RFID_Reader[reader_nr].PCD_StopCrypto1();
          break;
        }
        //* END AUTHENTICATION *

        //DISCONNECT CONNECTION WITH READER
        //Egal ob positive oder negative Auth oder Read, in jedem Fall muss die Kommunikation zum Reader beendet werden! Das macht StopCrypto1 - warum auch immer die so heißt.
        RFID_Reader[reader_nr].PCD_StopCrypto1();
      }

      //ELSE LOOK FOR NEW CARDS
      //Wenn keine neue Karte gefunden wurde, heißt das im Umkehrschluss, dass grad keine Karte auf dem Reader liegt. Somit muss das entsprechend "vermerkt" werden. Das Keyword ist dann natürlich auch nicht mehr richtig.
      else {

        if (!RFID_Reader[reader_nr].PICC_IsNewCardPresent() && (RFID_CardIsOnReader[reader_nr])) {

          RFID_CardIsOnReader[reader_nr] = false;
          RFID_KeywordIsRight[reader_nr] = false;
          NeoPixel_StripeOff(reader_nr);

        }

      }
      //END ELSE LOOK FOR NEW CARDS

    }
    //END GO FOR ALL RFID READER

  }
  //END ELSE CHECK IF ALL KEYWORDS ARE RIGHT

}

void RFID_EndGame() {
  Serial.println();
  Serial.println(F("+--------------------------------------+"));
  Serial.println(F("|          !!! ALL RIGHT !!!           |"));
  Serial.println(F("|  Waiting for new Game! Please reset! |"));
  Serial.println(F("+--------------------------------------+"));
  oled.clear();
  oled.println(F("!!! ALL RIGHT !!!"));
  oled.println(F("Waiting for new Game!"));
  oled.println(F("Please reset!"));

	// Neopixel
	for (uint8_t reader_nr = 0; reader_nr < RFID_NR_OF_READERS; reader_nr++) {
    NeoPixel_StripeEndGame(reader_nr);
  }

  //Licht aus
  relay.digitalWrite(EXP_LICHT_PIN, LOW);  //Licht aus
  relay.digitalWrite(EXP_SCHW_LI_PIN, HIGH); //Licht
  delay(4000);
  relay.digitalWrite(EXP_LICHT_PIN, HIGH);
}
