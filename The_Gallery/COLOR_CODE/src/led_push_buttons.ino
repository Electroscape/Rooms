/*=======================================================*/
/**		2CP - TeamEscape - Engineering
 *		by Abdullah Saei & Martin Pek
 *
 *		v2.0 beta
 *      - Accept passwords longer than length
 *      - Logic separation OLED and keypad
 *		- Use OLED SH1106
 *      - Remove deprecations
 *      - Block after correct solution
 *
 */
/*=======================================================*/
#include <stb_namespace.h>

#include "header_s.h"
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

/*==OLED======================================*/
SSD1306AsciiWire oled;
bool UpdateOLED = true;
unsigned long UpdateOLEDAfterDelayTimer = 0;

// Konfiguration fuer Sparkfun-Keypads
// Keypad 1 2 3 4 5 6 7
// Chip   0 1 2 3 4 5 6
byte KeypadRowPins[KEYPAD_ROWS] = {4};           // Zeilen  - Messleitungen
byte KeypadColPins[KEYPAD_COLS] = {0, 1, 2, 3};  // Spalten - Steuerleitungen (abwechselnd HIGH)

bool KeypadTyping = false;
bool KeypadCodeCorrect = false;
bool KeypadCodeWrong = false;
bool endGame = false;                    // Only true when correct solution after smiley face
unsigned long KeypadCodeResetTimer = 0;  // ResetTimer
const int OledWaitLastCharacter = 500;   // waiting time to show last character

Keypad_I2C MyKeypad(makeKeymap(KeypadKeys), KeypadRowPins, KeypadColPins,
                    KEYPAD_ROWS, KEYPAD_COLS, KEYPAD_I2C_ADD, PCF8574);
Password passLight = Password(secret_password);  // Schaltet das Licht im Büro an

/*==PCF8574===================================*/
Expander_PCF8574 relay, LetKeypadWork, leds;

unsigned long lastHeartbeat = millis();

/*================================================
//===SETUP========================================
//==============================================*/
void setup() {
    brainSerialInit();
    Serial.println(title);
    Serial.println(version);

    i2cScanner();

#ifndef OLED_DISABLE
    OLED_Init();
#endif
    Keypad_Init();
    relay_Init();

    delay(2000);
    printWithHeader("Setup Complete", "SYS");
}

/*=================================================
//===LOOP==========================================
//===============================================*/
void loop() {
    // Heartbeat message
    if (millis() - lastHeartbeat >= heartbeatFrequency) {
        lastHeartbeat = millis();
        printWithHeader(passLight.guess, relayCode);
    }

    Keypad_Update();
#ifndef OLED_DISABLE
    OLED_Update();
#endif

    // Block the arduino if correct solution
    if (endGame) {
        printWithHeader("Game Complete", "SYS");
        Serial.println("End Game, Please restart the arduino!");
        while (true) {
            delay(100);
        }
    }
}

/*===================================================
//===OLED============================================
//=================================================*/

#ifndef OLED_DISABLE

void print_logo_infos() {
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

void OLED_Init() {
    oled.begin(&SH1106_128x64, OLED_I2C_ADD);
    oled.set400kHz();
    oled.setScroll(true);
    oled.setFont(System5x7);
    print_logo_infos();
}

void OLED_Update() {
    if ((((millis() - UpdateOLEDAfterDelayTimer) > UpdateOLEDAfterDelay)) && !KeypadTyping) {
        UpdateOLED = true;
    }

    if (UpdateOLED) {
        UpdateOLEDAfterDelayTimer = millis();
        UpdateOLED = false;

        //display passcode typing
        OLED_keypadscreen();

        if (KeypadCodeCorrect) {
            OLED_smileySmile();
            endGame = true;
        } else if (KeypadCodeWrong) {
            OLED_smileySad();
            //some time to show the sad face :(
            delay(1000);
            UpdateOLED = true;
            KeypadCodeWrong = false;
        } else if (!KeypadTyping) {
            oledHomescreen();
        }
    }
}

void oledHomescreen() {
    oled.clear();
    oled.setFont(Adafruit5x7);
    oled.print("\n\n\n");
    oled.setFont(Verdana12_bold);
    oled.println("  Type your code..");
}

// Update Oled with keypad typing
void OLED_keypadscreen() {
    if (strlen(passLight.guess) < 1) return;
    oled.clear();
    oled.setFont(Adafruit5x7);
    oled.println();
    oled.setFont(Arial_bold_14);
    oled.println();
    oled.print(F("  "));
    oled.print(passLight.guess);
}

void OLED_smileySmile() {
    delay(OledWaitLastCharacter);
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
    delay(OledWaitLastCharacter);
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
#endif

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
    MyKeypad.addEventListener(keypadEvent);  // Event Listener erstellen
    MyKeypad.begin(makeKeymap(KeypadKeys));
    MyKeypad.setHoldTime(5000);
    MyKeypad.setDebounceTime(KeypadDebounceTime);
}

/**
 * Evaluates password after TIMEOUT and makes relay action
 *
 * @param void
 * @return void
 */
void Keypad_Update() {
    MyKeypad.getKey();
    if ((millis() - KeypadCodeResetTimer > KeypadCodeResetSpan) && KeypadTyping) {
        if (checkPassword()) {
            relay.digitalWrite(REL_PIC_VALVE_PIN, VALVE_OPEN);
        } else {
            printWithHeader("!Reset", relayCode);
        }
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
// small LEDs that are
bool led_init() {
    leds.begin(LED_I2C_ADD);
    for (int i = 0; i < 8; i++) {
        leds.pinMode(i, OUTPUT);
        leds.digitalWrite(i, HIGH);
    };
}

void blinkLed(byte led_no) {
    for (int i = 0; i < blink_amount; i++) {
        if (i > 0) { delay(blink_delay); }
        // good question is what datatype is pin ... 
        leds.digitalWrite(led_no, LOW);
        delay(blink_delay);
        leds.digitalWrite(led_no, HIGH);
    }
}

bool relay_Init() {
    Serial.println("initializing relay");
    relay.begin(RELAY_I2C_ADD);

    // init all 8,, they are physically disconnected anyways
    for (int i = 0; i < REL_AMOUNT; i++) {
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
 * @return (bool) true if correct password, false otherwise
 * @remark It only evaluates and return true or false!!
 */
bool checkPassword() {
    bool result = passLight.evaluate();
    KeypadTyping = false;
    UpdateOLED = true;
    if (result) {
        KeypadCodeCorrect = true;
        printWithHeader("!Correct", relayCode);
        blinkLed(GREEN_LED_PIN);
    } else {
        KeypadCodeWrong = true;
        printWithHeader("!Wrong", relayCode);
        blinkLed(GREEN_LED_PIN);
#ifndef OLED_DISABLE
        // Update OLED before reset
        OLED_keypadscreen();
#endif
        passwordReset();
    }
    return result;
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
