/*=======================================================*/
/**		2CP - TeamEscape - Engineering
 *		by Abdullah Saei & Martin Pek
 *
 *		v2.0 beta
 *      - Block after correct solution
 *      - Use standard relay init
 *      - Separete Header
 *		- Modified Serial prints
 *
 * TODO:
 * 		- Add Header comments on each function
 * 		- Check deprecations
 *
 */
/*=======================================================*/
#include "header_s.h"
#include <stb_namespace.h>
using namespace stb_namespace;

/*==INCLUDE==============================================*/
// I2C Port Expander
#include <PCF8574.h>

// OLED
#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>

// Keypad
#include <Keypad.h>
#include <Keypad_I2C.h>
#include <Wire.h>

// Password
#include <Password.h>

/*==DEFINE====================================*/
// uncomment to use
#define DEBUGMODE 0

/*==OLED======================================*/
SSD1306AsciiWire oled;
bool UpdateOLED = true;
unsigned long UpdateOLEDAfterDelayTimer = 0;
String magnetString = String("ON");

// Konfiguration fuer Sparkfun-Keypads
// Keypad 1 2 3 4 5 6 7
// Chip   0 1 2 3 4 5 6
byte KeypadRowPins[KEYPAD_ROWS] = {4};           // Zeilen  - Messleitungen
byte KeypadColPins[KEYPAD_COLS] = {0, 1, 2, 3};  // Spalten - Steuerleitungen (abwechselnd HIGH)

bool KeypadTyping = false;
bool KeypadCodeCorrect = false;
bool KeypadCodeWrong = false;
bool endGame = false;                      // Only true when correct solution after smiley face
unsigned long KeypadCodeResetTimer = 0;    // ResetTimer
const int KeypadWaitAfterCodeInput = 500;  // warten, wie lang der Code noch angezeigt wird, bis er ausgewertet wird

Keypad_I2C MyKeypad(makeKeymap(KeypadKeys), KeypadRowPins, KeypadColPins,
                    KEYPAD_ROWS, KEYPAD_COLS, KEYPAD_I2C_ADD, PCF8574);
Password passLight = Password(secret_password);  // Schaltet das Licht im Büro an

/*==PCF8574===================================*/
Expander_PCF8574 relay, LetKeypadWork;

/*==Serial Printing===========================*/
const int ctrlPin = A0;  // the control pin of max485 rs485 LOW read, HIGH write

/*================================================
//===SETUP========================================
//==============================================*/
void setup() {
    Serial_Init();
    OLED_Init();
    Keypad_Init();
    relay_Init();

    delay(2000);
    printWithHeader("Setup Complete", "SYS");
}

/*=================================================
//===LOOP==========================================
//===============================================*/
void loop() {
    Keypad_Update();

    OLED_Update();

    heartbeat();
}

/*===============================================
//===BASICS======================================
//=============================================*/
void print_logo_infos(String progTitle) {
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

// I2C Scanner - scannt nach angeschlossenen I2C Geräten
void i2c_scanner() {
    Serial.println(F("I2C scanner:"));
    Serial.println(F("Scanning..."));
    oled.setFont(System5x7);
    oled.println(F("I2C Scan..."));
    byte wire_device_count = 0;

    for (byte i = 8; i < 120; i++) {
        Wire.beginTransmission(i);
        if (Wire.endTransmission() == 0) {
            Serial.print(F("Found address: "));
            Serial.print(i, DEC);
            Serial.print(F(" (0x"));
            oled.print(F(" (0x"));
            Serial.print(i, HEX);
            oled.print(i, HEX);
            Serial.print(F(")"));
            oled.print(F(")"));
            if (i == 57) {
                Serial.print(F(" -> Buttons"));
                oled.print(" -> Buttons");
            }
            if (i == 60) {
                Serial.print(F(" -> Display"));
                oled.print(" -> Display");
            }
            if (i == 63) {
                Serial.print(F(" -> Relay"));
                oled.print(" -> Relay");
            }
            Serial.println();
            oled.println();
            wire_device_count++;
            delay(1);
        }
    }
    Serial.print(F("Found "));
    oled.print(F("Found "));
    Serial.print(wire_device_count, DEC);
    oled.print(wire_device_count, DEC);
    Serial.println(F(" device(s)."));
    oled.println(F(" device(s)."));

    Serial.println();
    oled.println();

    delay(2000);
}

// Software Reset - Startet den Arduino neu
void software_Reset() {
    printWithHeader("RESTART", "SYS");
    Serial.println("Expander Ports:");
    //? Open all Relays before restart!! is it necessary?
    for (int i = 0; i <= 7; i++) {
        relay.pinMode(i, OUTPUT);
        relay.digitalWrite(i, HIGH);
    }
    Serial.println("HIGH");
    Serial.println(F("Neustart in"));
    oled.clear();
    oled.println("Neustart in");
    delay(250);
    for (byte i = 3; i > 0; i--) {
        Serial.println(i);
        oled.println(i);
        delay(1000);
    }
    asm volatile("  jmp 0");
}

/**
 * Initialize Serial and MAX485
 *
 * @param void
 * @return void
 */
void Serial_Init() {
    Serial.begin(115200);
    delay(1000);
    // initialize MAX485 ctrl pin as an output
    pinMode(ctrlPin, OUTPUT);
    // print with header to set to the correct mode
    printWithHeader("Setup Begin", "SYS");
    Serial.println("\n");
}

/*===================================================
//===OLED============================================
//=================================================*/
void OLED_Init() {
    Wire.begin();

    oled.begin(&Adafruit128x64, OLED_I2C_ADD);
    oled.set400kHz();
    oled.setScroll(true);
    oled.setFont(System5x7);

    print_logo_infos(title);

    i2c_scanner();
}

/**
 * Update flags and send heartpulse messages
 *
 * @param void
 * @return void
 */
void OLED_Update() {
    if ((((millis() - UpdateOLEDAfterDelayTimer) > UpdateOLEDAfterDelay)) && !KeypadTyping) {
        UpdateOLED = true;
    }

    if (UpdateOLED) {
        UpdateOLEDAfterDelayTimer = millis();
        UpdateOLED = false;

        if (KeypadTyping) {
            OLED_keypadscreen();
        } else if (KeypadCodeCorrect) {
            OLED_smileySmile();
            delay(1000);
            UpdateOLED = true;
            KeypadCodeCorrect = false;
        } else if (KeypadCodeWrong) {
            OLED_smileySad();
            delay(1000);
            UpdateOLED = true;
            KeypadCodeWrong = false;
        } else {
            OLED_homescreen();
        }
    }
}

void OLED_homescreen() {
    oled.clear();
    oled.setFont(Adafruit5x7);
    oled.println();
    oled.setFont(Arial_bold_14);
    oled.println();
    oled.print(F("  Magnet: "));
    oled.println(magnetString);
}

// Update Oled with keypad typing
void OLED_keypadscreen() {
    oled.clear();
    oled.setFont(Adafruit5x7);
    oled.println();
    oled.setFont(Arial_bold_14);
    oled.println();
    oled.print(F("  "));
    oled.print(F("  "));
    for (unsigned int i = 0; i < strlen((passLight.guess)) + 1; i++) {
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
    oled.print(F("      '._____.'   "));
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
    oled.print(F("      '._____.'   "));
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

/*=========================================================
//===KEYPAD================================================
//=======================================================*/
/**
 * Initialize Keypad
 *
 * @param void
 * @return void
 * @note set PCF to high input to activate Pull-up resistors first, then initialize keypad library
 */
void Keypad_Init() {
    LetKeypadWork.begin(KEYPAD_I2C_ADD);
    for (int i = 0; i <= 7; i++) {
        LetKeypadWork.pinMode(i, INPUT);
        LetKeypadWork.digitalWrite(i, HIGH);
    }
    delay(100);
    oled.print(F("Keypad "));
    MyKeypad.addEventListener(keypadEvent);  // Event Listener erstellen
    MyKeypad.begin(makeKeymap(KeypadKeys));
    MyKeypad.setHoldTime(5000);
    MyKeypad.setDebounceTime(KeypadDebounceTime);
    oled.println(F("ok."));
}

/**
 * Clear password after TIMEOUT and auto-evaluate when reaches password length
 *
 * @param void
 * @return void
 */
void Keypad_Update() {
    MyKeypad.getKey();
    if ((millis() - KeypadCodeResetTimer > KeypadCodeResetSpan) && KeypadTyping) {
        printWithHeader("!Reset", relayCode);
        oled.clear();
        oled.println("Reset Passwort");
        passwordReset();
    }

    if (strlen((passLight.guess)) == strlen(secret_password) && !endGame) {
        UpdateOLED = true;
        OLED_Update();
        // Display last character for some time
        delay(KeypadWaitAfterCodeInput);
        checkPassword();
    }
}

/**
 * Listens to keypad inputs
 *
 * @param eKey Stores the pressed button
 * @return void
 */
void keypadEvent(KeypadEvent eKey) {
    switch (MyKeypad.getState()) {
        case PRESSED:
            KeypadTyping = true;
            UpdateOLED = true;
            KeypadCodeResetTimer = millis();

            switch (eKey) {
                default:
                    passLight.append(eKey);
                    printWithHeader(passLight.guess, relayCode);
                    // Serial print after printing with headers
                    // so serial buffer is clear
                    Serial.print("Pressed: ");
                    Serial.println(eKey);
                    break;
            }
            break;

        case HOLD:
            Serial.print("HOLD: ");
            Serial.println(eKey);
            break;

        default:
            break;
    }
}

/**
 * Initialise 8 Relays on I2C PCF
 *
 * @param void
 * @return true when done
 */
bool relay_Init() {
    Serial.println("initializing relay");
    relay.begin(RELAY_I2C_ADD);

    // init all 8,, they are physically disconnected anyways
    for (int i = 0; i < 8; i++) {
        relay.pinMode(relayPinArray[i], OUTPUT);
        relay.digitalWrite(relayPinArray[i], relayInitArray[i]);
        Serial.print("     ");
        Serial.print("Relay [");
        Serial.print(relayPinArray[i]);
        Serial.print("] set to ");
        Serial.println(relayInitArray[i]);
    }
    Serial.println();
    Serial.println("successfully initialized relay");
    return true;
}


/**
 * Evaluates guessed password
 *
 * @param void
 * @return void
 * @note it takes the action and sets the relay
 TODO: It should only evaluate and return true or false!!
 */
void checkPassword() {
    if (passLight.evaluate()) {
        KeypadCodeCorrect = true;
        KeypadTyping = false;
        UpdateOLED = true;

        printWithHeader("!Correct", relayCode);
        printWithHeader("Game Complete", "SYS");
        relay.digitalWrite(REL_PIC_VALVE_PIN, VALVE_OPEN);
        magnetString = String("OFF");
        passwordReset();
    } else {
        KeypadCodeWrong = true;
        KeypadTyping = false;
        UpdateOLED = true;

        printWithHeader("!Wrong", relayCode);
        passwordReset();
    }
}

/**
 * Clear password guess
 *
 * @param void
 * @return void
 */
void passwordReset() {
    KeypadTyping = false;
    UpdateOLED = true;
    passLight.reset();
}
