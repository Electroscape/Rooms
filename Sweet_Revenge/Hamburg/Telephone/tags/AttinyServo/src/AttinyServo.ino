/*==========================================================================================================*/
/*		2CP - TeamEscape - Engineering
 *		by Abdullah Saei & Robert Schloezer
 *
 *		v1.0
 *		-
 *		-
 *		-
 */
/*==========================================================================================================*/

const String title = String("Servo with Attiny85 v1.0");

// Code for the ATtiny85
#define I2C_SLAVE_ADDRESS 0x4 // Address of the slave

#include <TinyWireS.h>
#include "Servo.h"
int i=0;

void setup()
{
    TinyWireS.begin(I2C_SLAVE_ADDRESS); // join i2c network
    //TinyWireS.onReceive(receiveEvent); // not using this
    TinyWireS.onRequest(requestEvent);

    // Turn on LED when program starts
    pinMode(1, OUTPUT);
    digitalWrite(1, HIGH);
}

void loop()
{
    // This needs to be here
    TinyWireS_stop_check();
}

// Gets called when the ATtiny receives an i2c request
void requestEvent()
{
    TinyWireS.send(i);
    i++;
}
