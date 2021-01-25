/**************************************************************************/
/*!
    @file     readMifare.pde
    @author   Adafruit Industries
	@license  BSD (see license.txt)

    This example will wait for any ISO14443A card or tag, and
    depending on the size of the UID will attempt to read from it.

    If the card has a 4-byte UID it is probably a Mifare
    Classic card, and the following steps are taken:

    - Authenticate block 4 (the first block of Sector 1) using
      the default KEYA of 0XFF 0XFF 0XFF 0XFF 0XFF 0XFF
    - If authentication succeeds, we can then read any of the
      4 blocks in that sector (though only block 4 is read here)

    If the card has a 7-byte UID it is probably a Mifare
    Ultralight card, and the 4 byte pages can be read directly.
    Page 4 is read by default since this is the first 'general-
    purpose' page on the tags.


This is an example sketch for the Adafruit PN532 NFC/RFID breakout boards
This library works with the Adafruit NFC breakout
  ----> https://www.adafruit.com/products/364

Check out the links above for our tutorials and wiring diagrams
These chips use SPI or I2C to communicate.

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

*/
/**************************************************************************/
// I2C Port Expander
#include "PCF8574.h"
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

String title = "PN532 Example";
String versionDate = "22.01.2020";
String version = "version 0.1";

#define RELAY_I2C_ADD     	0x3F         // Relay Expander
#define OLED_I2C_ADD        0x3C         // Predefined by hardware
#define LCD_I2C_ADD         0x27         // Predefined by hardware

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
    REL_8_PIN                                // 7
};

enum REL_INIT{
  REL_1_INIT   =                0,        // COM-12V_IN, NO-12V_OUT, NC-/  set to 1 for magnet, 0 for mechanical
  REL_2_INIT   =                1,        // COM-12V_IN, NO-12V_OUT_DOOR, NC-12V_OUT_ALARM
  REL_3_INIT   =                1,        // NC-12V_OUT_ALARM
  REL_4_INIT   =                1,        // DESCRIPTION OF THE RELAY WIRING
  REL_5_INIT   =                1,        // DESCRIPTION OF THE RELAY WIRING
  REL_6_INIT   =                1,        // DESCRIPTION OF THE RELAY WIRING
  REL_7_INIT   =                1,        // DESCRIPTION OF THE RELAY WIRING
  REL_8_INIT   =                1        // COM AC_volt, NO 12_PS+, NC-/
};

// == constants

const enum REL_PIN relayPinArray[]  = {REL_1_PIN, REL_2_PIN, REL_3_PIN, REL_4_PIN, REL_5_PIN, REL_6_PIN, REL_7_PIN, REL_8_PIN};
const byte relayInitArray[] = {REL_1_INIT, REL_2_INIT, REL_3_INIT, REL_4_INIT, REL_5_INIT, REL_6_INIT, REL_7_INIT, REL_8_INIT};

#define REL_AMOUNT      3


/*
Adafruit_PN532 *RFID_Readers[RFID_AMOUNT];
{
    Adafruit_PN532 RFID_0(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_SSPins[0]),
    Adafruit_PN532 RFID_1(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_SSPins[1]),
    Adafruit_PN532 RFID_2(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_SSPins[2]),
    Adafruit_PN532 RFID_3(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_SSPins[3])
};
*/


// If using the breakout with SPI, define the pins for SPI communication.
#define PN532_SCK       13
#define PN532_MOSI      11
#define PN532_MISO      12

#define RFID_1_SS_PIN           8     /* Per Konvention ist dies RFID-Port 1                                */
#define RFID_2_SS_PIN           7     /* Per Konvention ist dies RFID-Port 2                                */
#define RFID_3_SS_PIN           4     /* Per Konvention ist dies RFID-Port 3                                */
#define RFID_4_SS_PIN           2     /* Per Konvention ist dies RFID-Port 4                                */

byte    RFID_SSPins[]  = {RFID_1_SS_PIN,  RFID_2_SS_PIN,  RFID_3_SS_PIN,  RFID_4_SS_PIN};

// very semi manual but ... its C ...
// uncomment as needed
static Adafruit_PN532 RFID_0(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_SSPins[0]);
static Adafruit_PN532 RFID_1(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_SSPins[1]);
static Adafruit_PN532 RFID_2(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_SSPins[2]);
static Adafruit_PN532 RFID_3(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_SSPins[3]);
static Adafruit_PN532 RFID_Readers[] = {RFID_0, RFID_1, RFID_2, RFID_3};

static int RFID_AMOUNT = sizeof(RFID_Readers);
static uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

// If using the breakout or shield with I2C, define just the pins connected
// to the IRQ and reset lines.  Use the values below (2, 3) for the shield!

// IRQ needs to set to sth safe since pin2 is used as SS/SDA
// #define PN532_IRQ   (2)
// #define PN532_RESET (10)  // Not connected by default on the NFC Shield

// Uncomment just _one_ line below depending on how your breakout or shield
// is connected to the Arduino:


/*==PCF8574=================================================================================================*/
Expander_PCF8574 relay;

// Use this line for a breakout with a software SPI connection (recommended):
// Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

// Use this line for a breakout with a hardware SPI connection.  Note that
// the PN532 SCK, MOSI, and MISO pins need to be connected to the Arduino's
// hardware SPI SCK, MOSI, and MISO pins.  On an Arduino Uno these are
// SCK = 13, MOSI = 11, MISO = 12.  The SS line can be any digital IO pin.
//Adafruit_PN532 nfc(PN532_SS);

void setup() {

    Serial.begin(115200);
    Wire.begin();

    // i2c_scanner();

    RFID_Init();

    // relay_Init();
}


void loop(void) {
    uint8_t success;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
    uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

    delay(1000);


    if (true) {
        for (int i=0; i<RFID_AMOUNT; i++) {

            Serial.print("Reader"); Serial.println(i);
            uint32_t versiondata = RFID_Readers[i].getFirmwareVersion();

            while (!versiondata) {
                if (!versiondata) {
                    Serial.println("Didn't find PN53x board");
                }
                versiondata = RFID_Readers[i].getFirmwareVersion();
                delay(100);
            }
            Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
            Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);

            success = RFID_Readers[i].readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
            Serial.println(success);

            if (success) {
                // Display some basic information about the card
                Serial.println("Found an ISO14443A card");
                Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
                Serial.print("  UID Value: ");
                RFID_Readers[i].PrintHex(uid, uidLength);
                Serial.println("");
            }

            delay(1000);
        }
    }


    // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
    // 'uid' will be populated with the UID, and uidLength will indicate
    // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)

    /*
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) {
    // Display some basic information about the card
    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");

    if (uidLength == 4)
    {
      // We probably have a Mifare Classic card ...
      Serial.println("Seems to be a Mifare Classic card (4 byte UID)");

      // Now we need to try to authenticate it for read/write access
      // Try with the factory default KeyA: 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
      Serial.println("Trying to authenticate block 4 with default KEYA value");
      uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

	  // Start with block 4 (the first block of sector 1) since sector 0
	  // contains the manufacturer data and it's probably better just
	  // to leave it alone unless you know what you're doing
      // 4th entry is keynumber, 0 for keyA and 1 for keyB
      success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 1, 0, keya);

      if (success)
      {
        Serial.println("Sector 1 (Blocks 4..7) has been authenticated");
        uint8_t data[16];

        // If you want to write something to block 4 to test with, uncomment
		// the following line and this text should be read back in a minute
        //memcpy(data, (const uint8_t[]){ 'a', 'd', 'a', 'f', 'r', 'u', 'i', 't', '.', 'c', 'o', 'm', 0, 0, 0, 0 }, sizeof data);
        // success = nfc.mifareclassic_WriteDataBlock (4, data);

        // Try to read the contents of block 4
        success = nfc.mifareclassic_ReadDataBlock(1, data);

        if (success)
        {
          // Data seems to have been read ... spit it out
          Serial.println("Reading Block 4:");
          nfc.PrintHexChar(data, 16);
          Serial.println("");

          // Wait a bit before reading the card again
          delay(1000);
        }
        else
        {
          Serial.println("Ooops ... unable to read the requested block.  Try another key?");
        }
      }
      else
      {
        Serial.println("Ooops ... authentication failed: Try another key?");
      }
    }

    if (uidLength == 7)
    {
      // We probably have a Mifare Ultralight card ...
      Serial.println("Seems to be a Mifare Ultralight tag (7 byte UID)");

      // Try to read the first general-purpose user page (#4)
      Serial.println("Reading page 4");
      uint8_t data[32];
      success = nfc.mifareultralight_ReadPage (4, data);
      if (success)
      {
        // Data seems to have been read ... spit it out
        nfc.PrintHexChar(data, 4);
        Serial.println("");

        // Wait a bit before reading the card again
        delay(1000);
      }
      else
      {
        Serial.println("Ooops ... unable to read the requested page!?");
      }
    }
  }
  */
}

bool RFID_Gate_locked() {

    uint8_t data[16] = {};

    for(int reader_nr=0; i<RFID_AMOUNT; i++) {

        uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
        uint8_t uidLength;
        uint8_t success;

        success = cardPresent(reader_nr, uid);

        if (success) {
            Serial.print("Card present on reader: "); Serial.println(reader_nr);


        } else {
            return true
        }
    }
}

bool cardPresent(int reader_nr, uint8_t * uid) {

}





/*==FUNCTIONS==================================================================================================*/

bool RFID_Init() {


    if (true) {
        for (int i=0; i<RFID_AMOUNT; i++) {

             RFID_Readers[i].begin();
             uint32_t versiondata = RFID_Readers[i].getFirmwareVersion();
             if (! versiondata) {
               Serial.print("Didn't find PN53x board");
               // TODO: remove do some sort of timeout ... dont want to be stuck here without info
               while (1); // halt
             }
             Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
             Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
             Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);

             // configure board to read RFID tags
             RFID_Readers[i].SAMConfig();

             Serial.println();
             versiondata = RFID_Readers[0].getFirmwareVersion();
             Serial.print("Firmware ver. "); Serial.println((versiondata>>16) & 0xFF, DEC);
             Serial.println();
             delay(100);
        }
    }
    return true;
}

void dbg_println(String print_dbg) {
    Serial.println(print_dbg);
    //oled.println(print_dbg);
}

void print_logo_infos(String progTitle) {
    Serial.println(F("+-----------------------------------+"));
    Serial.println(F("|    TeamEscape HH&S ENGINEERING    |"));
    Serial.println(F("+-----------------------------------+"));
    Serial.println();
    Serial.println(progTitle);
    Serial.println();
    delay(200);
}

void print_serial_header() {
    print_logo_infos(title);

    dbg_println("!header_begin");
    dbg_println(title);
    dbg_println(versionDate);
    dbg_println(version);
}

void dbg_init() {
    Serial.begin(115200);
    print_serial_header();
}

void print_setup_end() {
    Serial.println("!setup_end");
    Serial.println(); Serial.println("===================START====================="); Serial.println();
}

bool relay_Init() {

    Serial.println("initializing relay");
    relay.begin(RELAY_I2C_ADD);
    delay(100);

    for (int i=0; i<REL_AMOUNT; i++) {
        relay.pinMode(i, OUTPUT);
        relay.digitalWrite(i, HIGH);
        delay(100);
    }

    delay(500);

    for (int i=0; i<REL_AMOUNT; i++) {
        relay.digitalWrite(relayPinArray[i], relayInitArray[i]);
        Serial.print("     ");
        Serial.print("Relay ["); Serial.print(relayPinArray[i]); Serial.print("] set to "); Serial.println(relayInitArray[i]);
        delay(20);
    }

    Serial.println();
    Serial.println("successfully initialized relay");

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
            if (i == 22) Serial.print(F(" -> Servo-I2C-Board"));
            Serial.println();
            wire_device_count++;
            delay (1);
        }
    }
    Serial.print   (F("Found "));
    Serial.print   (wire_device_count, DEC);
    Serial.println (F(" device(s)."));
    Serial.println("successfully scanned I2C");

    Serial.println();

    delay(500);
}

/*==RESET==================================================================================================*/

void software_Reset() {
    Serial.println(F("Restarting in"));
    delay(250);
    for (byte i = 3; i>0; i--) {
        Serial.println(i);
        delay(100);
    }
    asm volatile ("  jmp 0");
}
