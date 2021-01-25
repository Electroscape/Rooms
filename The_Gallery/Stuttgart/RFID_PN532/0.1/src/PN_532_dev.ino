/*
        2CP - TeamEscape - Engineering
        Author Martin Pek

        changelog
        v0.1

        Todo:
        -hello!
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

// If using the breakout with SPI, define the pins for SPI communication.
#define PN532_SCK               13
#define PN532_MOSI              11
#define PN532_MISO              12

#define RFID_1_SS_PIN           8     /* Per Konvention ist dies RFID-Port 1                                */
#define RFID_2_SS_PIN           7     /* Per Konvention ist dies RFID-Port 2                                */
#define RFID_3_SS_PIN           4     /* Per Konvention ist dies RFID-Port 3                                */
#define RFID_4_SS_PIN           2     /* Per Konvention ist dies RFID-Port 4                                */

byte    RFID_SSPins[]  = {RFID_1_SS_PIN,  RFID_2_SS_PIN,  RFID_3_SS_PIN,  RFID_4_SS_PIN};

// very manual but ... its C its gonna be bitching when it doesnt know during compilte time
// uncomment as needed
static Adafruit_PN532 RFID_0(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_SSPins[0]);
static Adafruit_PN532 RFID_1(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_SSPins[1]);
static Adafruit_PN532 RFID_2(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_SSPins[2]);
static Adafruit_PN532 RFID_3(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_SSPins[3]);
static Adafruit_PN532 RFID_Readers[] = {RFID_0, RFID_1, RFID_2, RFID_3};
#define RFID_AMOUNT     4

static uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static char RFID_solutions[RFID_AMOUNT][2]  = {"AH", "SD", "GF", "AB"};
#define RFID_DATABLOCK      1

// If using the breakout or shield with I2C, define just the pins connected
// to the IRQ and reset lines.  Use the values below (2, 3) for the shield!

// IRQ needs to set to sth safe since pin2 is used as SS/SDA
// #define PN532_IRQ   (2)
// #define PN532_RESET (10)  // Not connected by default on the NFC Shield


/*==PCF8574=================================================================================================*/
Expander_PCF8574 relay;

void setup() {

    communication_init();

    Serial.println();
    Serial.println("I2C: ... ");
    if (i2c_scanner()) {Serial.println("I2C: OK!");} else {Serial.println("I2C: FAILED!");};

    Serial.println();
    Serial.println("RFID: ... ");
    if (RFID_Init()) {Serial.println("RFID: OK!");} else {Serial.println("RFID: FAILED!");};

    Serial.println();
    Serial.println("Relay: ... ");
    if (relay_Init()) {Serial.println("Relay: OK!");} else {Serial.println("Relay: FAILED!");};

    Serial.println();
    print_setup_end();
}

void loop() {

    if (RFID_Gate_locked()) {
        relay.digitalWrite(REL_1_PIN, REL_1_INIT);
    } else {
        relay.digitalWrite(REL_1_PIN, !REL_1_INIT);
    }
    delay(500);
}

bool RFID_Gate_locked() {

    uint8_t success;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
    uint8_t uidLength;

    for(int reader_nr=0; reader_nr<RFID_AMOUNT; reader_nr++) {

        Serial.print("Checking presence for reader: ");Serial.println(reader_nr);

        success = RFID_Readers[reader_nr].readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
        if (success) {
            Serial.println("Card present on reader!");
            RFID_Readers[reader_nr].PrintHex(uid, uidLength);
            if (uidLength != 4) {
                Serial.println("Card is not Mifare classic, discarding card");
                return true;
            }
            if (!readAndCompare(reader_nr, uid, uidLength)) {
                return true;
            }
        } else {
            Serial.println("No presence on the reader!");
            return true;
        }
    }
    return false;
}

// only bothering with mifare classic since i'm not aware of Ultralight cards
bool readAndCompare(int reader_nr, uint8_t *uid, uint8_t uidLength) {

    uint8_t success;

    // authentication may be shifted to another function if we need to expand
    success = RFID_Readers[reader_nr].mifareclassic_AuthenticateBlock(uid, uidLength, RFID_DATABLOCK, 0, keya);
    Serial.println("Trying to authenticate block 4 with default KEYA value");
    delay(100);
    if (!success) {
        Serial.println("Authentication failed, card may already be authenticated");
        return false;
    }

    uint8_t data[16];
    success = RFID_Readers[reader_nr].mifareclassic_ReadDataBlock(RFID_DATABLOCK, data);
    if (!success) {
        Serial.println("Reading failed, discarding card");
        return false;
    }

    RFID_Readers[reader_nr].PrintHexChar(data, 16);
    for(int i=0; i<sizeof(RFID_solutions[reader_nr]); i++) {
        if (RFID_solutions[reader_nr][i] != data[i]) {
            Serial.println("Wrong password");
            return false;
        }
    }

    Serial.println("!Event correct card has been placed");
    return true;

    delay(100);
}




/*==FUNCTIONS==================================================================================================*/

void RFID_dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}


bool RFID_Init() {

    for (int i=0; i<RFID_AMOUNT; i++) {

        Serial.print("initializing reader: "); Serial.println(i);
        RFID_Readers[i].begin();
        RFID_Readers[i].setPassiveActivationRetries(10);

        uint32_t versiondata = RFID_Readers[i].getFirmwareVersion();
        if (!versiondata) {
            Serial.print("Didn't find PN53x board");
            software_Reset();
        }
        Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
        Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
        Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);

        // configure board to read RFID tags
        RFID_Readers[i].SAMConfig();

        delay(100);
    }
    return true;
}

void dbg_println(String print_dbg) {
    Serial.println(print_dbg);
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

void communication_init() {
    Wire.begin();
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

bool i2c_scanner() {
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

    return true;
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
