String brainName = String("BrRFID");
String relayCode = String("UVL");

#define MAX485_WRITE HIGH
#define MAX485_READ LOW

// Standards der Adressierung (Konvention)
	// Relayboard und OLED
		#define RELAY_I2C_ADD     	 0x3F     /* Relay Expander																							*/
		#define LIGHT_OLED_ADD       0x3C     /* Ist durch Hardware des OLEDs 0x3C									*/
		#define EXIT_OLED_ADD        0x3D     /* Ist durch Hardware des OLEDs 									*/
// ______________________EINSTELLUNGEN______________________
	// RFID
