/*
        2CP - TeamEscape - Engineering
        Author Martin Pek & Abdullah Saei

        - Modified Serial outputs
        - Optimize initalization delay to smooth restarts.
        - Running version.
        - Locking after correct solution.

        Todo:
        - Nothing! working and stable.
*/

/**************************************************************************/
//Setting Configurations
#include "header.h"

// I2C Port Expander
#include "PCF8574.h"
#include <Wire.h>
#include <SPI.h>

//Watchdog timer
#include <avr/wdt.h>

#define RELAY_I2C_ADD     	0x3F         // Relay Expander
#define OLED_I2C_ADD        0x3C         // Predefined by hardware
#define LCD_I2C_ADD         0x27         // Predefined by hardware

// uncomment to use
#define DEBUGMODE           0

// == constants

const enum REL_PIN relayPinArray[]  = {REL_1_PIN, REL_2_PIN, REL_3_PIN, REL_4_PIN, REL_SCHW_LI_PIN, REL_ROOM_LI_PIN, REL_7_PIN, REL_8_PIN};
const byte relayInitArray[] = {REL_1_INIT, REL_2_INIT, REL_3_INIT, REL_4_INIT, REL_SCHW_LI_INIT, REL_ROOM_LI_INIT, REL_7_INIT, REL_8_INIT};

#define REL_AMOUNT      8


// == LEDS ================================================//

#define RFID_1_LED_PIN          9     // Per Konvention ist dies RFID-Port 1                       */
#define RFID_2_LED_PIN          6     // Per Konvention ist dies RFID-Port 2                       */
#define RFID_3_LED_PIN          5     // Per Konvention ist dies RFID-Port 3                       */
#define RFID_4_LED_PIN          3     // Per Konvention ist dies RFID-Port 4

// NeoPixel
#include <Adafruit_NeoPixel.h>        // Ueber Bibliotheksverwalter
// NeoPixel
#define NEOPIXEL_NR_OF_PIXELS   1     /* Anzahl der Pixel auf einem Strang (Test 1 Pixel)          */
#define STRIPE_CNT              4

Adafruit_NeoPixel LED_Stripe_1 = Adafruit_NeoPixel(NEOPIXEL_NR_OF_PIXELS, RFID_1_LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel LED_Stripe_2 = Adafruit_NeoPixel(NEOPIXEL_NR_OF_PIXELS, RFID_2_LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel LED_Stripe_3 = Adafruit_NeoPixel(NEOPIXEL_NR_OF_PIXELS, RFID_3_LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel LED_Stripe_4 = Adafruit_NeoPixel(NEOPIXEL_NR_OF_PIXELS, RFID_4_LED_PIN, NEO_GRB + NEO_KHZ800);

uint8_t gBrightness = 128;

// == PN532 imports and setup
#include <Adafruit_PN532.h>

// If using the breakout with SPI, define the pins for SPI communication.
#define PN532_SCK               13
#define PN532_MOSI              11
#define PN532_MISO              12

#define RFID_1_SS_PIN           8     /* Per Konvention ist dies RFID-Port 1                                */
#define RFID_2_SS_PIN           7     /* Per Konvention ist dies RFID-Port 2                                */
#define RFID_3_SS_PIN           4     /* Per Konvention ist dies RFID-Port 3                                */
#define RFID_4_SS_PIN           2     /* Per Konvention ist dies RFID-Port 4                                */

const byte RFID_SSPins[]  = {RFID_1_SS_PIN,  RFID_2_SS_PIN,  RFID_3_SS_PIN,  RFID_4_SS_PIN};

// very manual but ... its C its gonna be bitching when it doesnt know during compilte time
// uncomment as needed
const Adafruit_PN532 RFID_0(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_SSPins[0]);
const Adafruit_PN532 RFID_1(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_SSPins[1]);
const Adafruit_PN532 RFID_2(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_SSPins[2]);
const Adafruit_PN532 RFID_3(PN532_SCK, PN532_MISO, PN532_MOSI, RFID_SSPins[3]);

Adafruit_PN532 RFID_READERS[4] = {RFID_0, RFID_1, RFID_2, RFID_3}; //

uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
char RFID_reads[4][RFID_SOLUTION_SIZE]             = {"\0\0", "\0\0", "\0\0", "\0\0"}; //
#define RFID_DATABLOCK      1
int cards_solution[RFID_AMOUNT] = {0}; //0 no card, 1 there is card, 2 correct card
bool runOnce = false;
bool EndBlock = false;
bool printStats = true;

//=====Timer=============================/
unsigned long delayStart = 0; // the time the delay started
bool delayRunning = false; // true if still waiting for delay to finish

/*==PCF8574=================================================================================================*/
Expander_PCF8574 relay;

/*==Serial Printing=================================================================================================*/
const int readPin =  A0;      // the control pin of max485 rs485 LOW read, HIGH write

/*============================================================================================================
//===SETUP====================================================================================================
//==========================================================================================================*/

void setup() {

    Serial_Init();
    Serial.println("WDT endabled");
    wdt_enable(WDTO_8S);

    Serial.println();
    Serial.println("LED: ... ");
    if (LED_init()) {Serial.println("LED: OK!");} else {Serial.println("LED: FAILED!");};

    wdt_reset();

    Serial.println();
    Serial.println("I2C: ... ");
    if (i2c_scanner()) {Serial.println("I2C: OK!");} else {Serial.println("I2C: FAILED!");};

    wdt_reset();

    Serial.println();
    Serial.println("RFID: ... ");
    if (RFID_Init()) {Serial.println("RFID: OK!");} else {Serial.println("RFID: FAILED!");};

    wdt_reset();

    Serial.println();
    Serial.println("Relay: ... ");
    if (relay_Init()) {Serial.println("Relay: OK!");} else {Serial.println("Relay: FAILED!");};

    wdt_reset();

    delayStart = millis();   // start delay

    printWithHeader("Setup Complete", "SYS");
}

void loop() {

    //send refresh signal every interval of time
    Update_serial();

    if (!EndBlock){
        wdt_reset();
        RFID_loop();
        Update_LEDs();
        wdt_reset();

        //Game Solved
        if (RFID_Status() && !runOnce){
            Serial.println("GATE OPEN");
            Update_LEDs();
            relay.digitalWrite(REL_ROOM_LI_PIN, LIGHT_OFF);
            relay.digitalWrite(REL_SCHW_LI_PIN, LIGHT_ON);
            // Sometimes green LEDs miss the command then it needs to wait 3 secs to update.
            Update_LEDs();
            delay(1000);
            Update_LEDs();
            delay(1000);
            Update_LEDs();
            delay(1000);
            Update_LEDs();
            relay.digitalWrite(REL_ROOM_LI_PIN, LIGHT_ON);
            runOnce = true;
            wdt_reset();

            //Block the game
            Serial.println("Waiting for new Game!");
            EndBlock = true;
            Serial.println("Restart in required!");
            wdt_disable();
            printWithHeader("Game Complete", "SYS");

        } else {
            relay.digitalWrite(REL_ROOM_LI_PIN, REL_ROOM_LI_INIT);
            relay.digitalWrite(REL_SCHW_LI_PIN, REL_SCHW_LI_INIT);
        }

    } else {
        Update_LEDs();
        delay(500);
    }
}

void NeoPixel_StripeOn(byte i, String color_str) {

    uint32_t color;

    if (color_str == "red") {
        color = LED_Stripe_1.Color(255,   0,   0); //red
    } else
    if (color_str == "green") {
        color = LED_Stripe_1.Color(  0, 255,   0); //green
    } else
    if (color_str == "white") {
        color = LED_Stripe_1.Color(255, 255,   255); //white
    } else
    if (color_str == "gold") {
        color = LED_Stripe_1.Color(255, 70,   0); //gold
    } else
    if (color_str == "black") {
        color = LED_Stripe_1.Color(0, 0,   0); //black
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

// RFID functions

void RFID_loop() {
    uint8_t success;
    uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
    uint8_t uidLength;
    uint8_t data[16];

    int cards_present[RFID_AMOUNT] = {0}; //compare with previous card stats to detect card changes

    for (uint8_t reader_nr=0; reader_nr<RFID_AMOUNT; reader_nr++) {
        wdt_reset();
        //Serial.print("reader ");Serial.print(reader_nr);Serial.print(":");

        success = RFID_READERS[reader_nr].readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
        if (success) {
            //Serial.println(" Yes");

            if (uidLength != 4) { //Card is not Mifare classic, discarding card
                Serial.println("False Card!");
                continue;
            }

            if (!read_PN532(reader_nr, data, uid, uidLength)) {
                Serial.println("read failed");
                continue;
            } else {
                //Valid card present
                cards_present[reader_nr] = 1;

                //Update data
                data[RFID_SOLUTION_SIZE-1] ='\0';
                strncpy(RFID_reads[reader_nr], (char*)data, RFID_SOLUTION_SIZE);
            }

            if (!data_correct(reader_nr, data)) {
                //Serial.print(cards_present[reader_nr]);Serial.print(cards_solution[reader_nr]);
                //continue;
            } else {
                cards_present[reader_nr] = 2;
            }

        } else {
            //Serial.println(" No");
            //cards_present[reader_nr] = 0;
            //Serial.print(cards_present[reader_nr]);Serial.print(cards_solution[reader_nr]);
        }
        //Serial.print(cards_present[reader_nr]);Serial.println(cards_solution[reader_nr]);
        if (cards_solution[reader_nr] != cards_present[reader_nr]) {
            cards_solution[reader_nr] = cards_present[reader_nr];
            printWithHeader("Cards Changed", "SYS");
            runOnce = false;
            printStats = true;
        }
        //delay(1);
    }
}

bool RFID_Status() {

    if !(printStats) {
        return false;
    }
    printStats = false;
    // turn Write mode on:
    digitalWrite(readPin, HIGH);
    delay(5);
    Serial.println();
    Serial.print("!");
    Serial.print(brainName);
    Serial.print(",");
    Serial.print(relayCode);
    Serial.print(",");
    bool noZero = true;
    int sum = 0;
    size_t j; //index for wrong solution card
    for (uint8_t i=0; i < RFID_AMOUNT; i++) {
        sum += cards_solution[i];
        if (cards_solution[i]==2) {
            Serial.print("C");
            Serial.print(i+1);
        } else if (cards_solution[i]==1) {
            bool found = false;
            for ( j = 0; j < RFID_AMOUNT; j++) {
                if (strcmp(RFID_solutions[j],RFID_reads[i])==0) {
                    found = true;
                    break;
                }
            }
            if (found) {
                Serial.print("C");
                Serial.print(j+1);
            } else {
                Serial.print("XX");
            }

        } else if (cards_solution[i]==0) {
            noZero = false;
            Serial.print("__");
        } else {
            Serial.print("Unknown Card");
        }
        Serial.print(" ");
        delay(5);
    }

    Serial.println(", Done.");
    delay(25);
    // turn Read mode on:
    digitalWrite(readPin, LOW);
    //Serial.print("Sum: ");Serial.print(sum);

    if (sum==2*RFID_AMOUNT) {
        printWithHeader("Correct Solution", relayCode);
        return true;
    } else if (noZero) {
        printWithHeader("Wrong Solution", relayCode);
        return false;
    }
}


void Update_LEDs() {
    int sum = 0;
    bool noZero = true;
   //  Serial.println("UPDATE LEDS");
    for (uint8_t i=0; i < RFID_AMOUNT; i++){
        sum += cards_solution[i];
        if (cards_solution[i] == 0){
            noZero = false;
        }
    }

    if (sum == 2*RFID_AMOUNT) {
        for (size_t i = 0; i < RFID_AMOUNT; i++) {
            NeoPixel_StripeOn(i, "green");
        }
    } else if (noZero) {
        for (size_t i = 0; i < RFID_AMOUNT; i++) {
            NeoPixel_StripeOn(i, "red");
        }
    } else {
        for (size_t i = 0; i < RFID_AMOUNT; i++) {
            if(cards_solution[i] == 0) {
                NeoPixel_StripeOn(i, "black");
            } else {
                NeoPixel_StripeOn(i, "white");
            }
        }
    }
}


bool read_PN532(int reader_nr, uint8_t *data, uint8_t *uid, uint8_t uidLength) {

    bool success;

    // authentication may be shifted to another function if we need to expand
    success = RFID_READERS[reader_nr].mifareclassic_AuthenticateBlock(uid, uidLength, RFID_DATABLOCK, 0, keya);
    //dbg_println("Trying to authenticate block 4 with default KEYA value");
    delay(1); //was 100!!
    if (!success) {
        //dbg_println("Authentication failed, card may already be authenticated");
        return false;
    }

    success = RFID_READERS[reader_nr].mifareclassic_ReadDataBlock(RFID_DATABLOCK, data);
    if (!success) {
        Serial.println("Reading failed, discarding card");
    }
    return success;
}

void Update_serial(){
    // check if delay has timed out after UpdateSignalAfterDelay ms
    if ((millis() - delayStart) >= UpdateSignalAfterDelay) {
        delayStart = millis();
        printWithHeader("refresh","SYS");
        //printStats = true;
    }
}

bool data_correct(int current_reader, uint8_t *data) {
    uint8_t result = -1;

    for (int reader_nr=0; reader_nr<RFID_AMOUNT; reader_nr++) {
        for (int i=0; i<RFID_SOLUTION_SIZE-1; i++) {
            if (RFID_solutions[reader_nr][i] != data[i]) {
                // We still check for the other solutions to show the color
                // but we display it being the wrong solution of the riddle
                if (reader_nr == current_reader) {
                    //Serial.print("Wrong Card"); Serial.println(current_reader);
                }
                continue;
            } else {
                //Serial.print(data[i]);
                if (i >= RFID_SOLUTION_SIZE - 2) {
                    // its a valid card but not placed on the right socket
                    //dbg_println("equal to result of reader");
                    //dbg_println(str(i));
                    result = reader_nr;
                }
            }
        }
        //Serial.println(" ");
    }
    if (result<0){
        Serial.print((char*)data);
        Serial.println(" = Undefined Card!!!");
        return false;
    }

    return  result == current_reader;
}

/*==FUNCTIONS==================================================================================================*/

void RFID_dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}

void NeoPixel_Init(byte i) {
    switch (i) {
        case 0:
            LED_Stripe_1.begin();
            NeoPixel_StripeOn(i, "gold");
            break;
        case 1:
            LED_Stripe_2.begin();
            NeoPixel_StripeOn(i, "gold");
            break;
        case 2:
            LED_Stripe_3.begin();
            NeoPixel_StripeOn(i, "gold");
            break;
        case 3:
            LED_Stripe_4.begin();
            NeoPixel_StripeOn(i, "gold");
            break;
        default: break;
    }
    delay(100);
}

bool LED_init() {
    for (size_t i = 0; i < STRIPE_CNT; i++) {
        NeoPixel_Init(i);
    }
    delay(100);
    for (size_t i = 0; i < STRIPE_CNT; i++) {
        NeoPixel_StripeOff(i);
    }
    return true;
}

bool RFID_Init() {
    bool success;
    for (int i=0; i<RFID_AMOUNT; i++) {

        uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
        uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

        wdt_reset();

        Serial.print("initializing reader: "); Serial.println(i);
        RFID_READERS[i].begin();
        RFID_READERS[i].setPassiveActivationRetries(5);

        int retries = 0;
        while (true) {
            uint32_t versiondata = RFID_READERS[i].getFirmwareVersion();
            if (!versiondata) {
                Serial.println("Didn't find PN53x board");
                if (retries > 5) {
                    Serial.println("PN532 startup timed out, restarting");
                    software_Reset();
                }
            } else {
                Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
                Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
                Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
                break;
            }
            retries++;
        }
        // configure board to read RFID tags
        RFID_READERS[i].SAMConfig();
        delay(1); //was 20!!!
        success = RFID_READERS[i].readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
        //cards_solution[i]= int(success);
    }

    return success;
}


void print_logo_infos(String progTitle) {
    Serial.println(F("+-----------------------------------+"));
    Serial.println(F("|    TeamEscape HH&S ENGINEERING    |"));
    Serial.println(F("+-----------------------------------+"));
    Serial.println();
    Serial.println(progTitle);
    Serial.println();
}

bool relay_Init() {

    Serial.println("initializing relay");
    relay.begin(RELAY_I2C_ADD);

    for (int i=0; i<REL_AMOUNT; i++) {
        relay.pinMode(relayPinArray[i], OUTPUT);
        relay.digitalWrite(relayPinArray[i], relayInitArray[i]);
        Serial.print("     ");
        Serial.print("Relay ["); Serial.print(relayPinArray[i]); Serial.print("] set to "); Serial.println(relayInitArray[i]);
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


void Serial_Init(){
    Wire.begin();
    Serial.begin(115200);
    delay(2000);
    // initialize the read pin as an output:
    pinMode(readPin, OUTPUT);
    // turn Write mode on:
    digitalWrite(readPin, HIGH);
    Serial.println("\n");
    printWithHeader("Setup Begin", "SYS");
    // turn Write mode off:
    digitalWrite(readPin, LOW);
}


void printWithHeader(String message, String source){
  // turn Write mode on:
  digitalWrite(readPin, HIGH);
	Serial.println();
	Serial.print("!");
	Serial.print(brainName);
	Serial.print(",");
	Serial.print(source);
	Serial.print(",");
	Serial.print(message);
	Serial.println(",Done.");
	delay(50);
	// turn Write mode off:
    digitalWrite(readPin, LOW);
}

/*==RESET==================================================================================================*/

void software_Reset() {
    Serial.println(F("Restarting in"));
    delay(50);
    for (byte i = 3; i>0; i--) {
        Serial.println(i);
        delay(100);
    }
    asm volatile ("  jmp 0");
}
