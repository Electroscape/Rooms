#define KAMERA_1_PIN			3
#define KAMERA_2_PIN			4
#define KAMERA_1_LED_PIN		A1
#define KAMERA_2_LED_PIN		A2
#define SCHRANKE_PIN			5
#define ALARM_PIN				6
#define RESET_PIN				7
#define US_TRIGGER_1_PIN		8
#define US_ECHO_1_PIN			9
#define US_TRIGGER_2_PIN		10
#define US_ECHO_2_PIN			11

/* ULTRASCHALL ----------------------------------------------------------------------- */
const int usAnzahl = 2;
int usTriggerNR[usAnzahl] 	= {US_TRIGGER_1_PIN, US_TRIGGER_2_PIN};
int usEchoNR[usAnzahl] 		= {US_ECHO_1_PIN, US_ECHO_2_PIN};
unsigned long minDistanz[usAnzahl] = {0, 0};
//unsigned long dauer 	 = 0;
//unsigned long entfernung[usAnzahl] = {0, 0};

/* KAMERA ---------------------------------------------------------------------------- */
const int camAnzahl 	= 2;
int camNR[camAnzahl] 	= {KAMERA_1_PIN, KAMERA_2_PIN};				// Gelb Sensor Input
int camLedNR[camAnzahl] = {KAMERA_1_LED_PIN, KAMERA_2_LED_PIN};		// Weiss LED Output  (Lila =  MINUS)

bool AlarmSystem = true;
bool AlarmOn = false;

void setup() {
	Serial.begin(115200);

	// Kameras
	for(int i=0; i<camAnzahl; i++) {
		pinMode(camNR[i], INPUT);
		Serial.print(F("Kamera ")); Serial.print(camNR[i] + 1); Serial.println(" init: done");
		pinMode(camLedNR[i], OUTPUT);
		Serial.print(F("Kamera LED")); Serial.print(camLedNR[i] + 1); Serial.println(" init: done");
	}
	Serial.println();

	// Lichtschranke
	pinMode(SCHRANKE_PIN, INPUT);
	Serial.print(F("Lichtschranke")); Serial.println(" init: done");

	// Ultraschall
	for(int i=0; i<usAnzahl; i++) {
			pinMode(usTriggerNR[i], OUTPUT);
			Serial.print(F("Ultraschall-Trigger ")); Serial.print(i + 1); Serial.println(" init: done");
			pinMode(usEchoNR[i], INPUT);
			Serial.print(F("Ultraschall-Echo ")); Serial.print(i + 1); Serial.println(" init: done");
	}

	Serial.println(F("Ultraschall-Kalibrierung"));

	for(int i=0; i<usAnzahl; i++) {
				minDistanz[i] = ultraschall_kalibrierung(usTriggerNR[i], usEchoNR[i]);
				Serial.print(F("US[")); Serial.print(i + 1); Serial.print(F("] : ")); Serial.print(minDistanz[i]); Serial.println(" cm");
		}

	Serial.println();

	// Alarm
	pinMode(ALARM_PIN, OUTPUT);
	Serial.print(F("Alarm")); Serial.println(" init: done");
	Serial.println();

	// Reset
	pinMode(RESET_PIN, INPUT);
	Serial.print(F("Reset")); Serial.println(" init: done");
	Serial.println();
}

void loop(){

	if(kameras() && AlarmSystem && !digitalRead(RESET_PIN)) {
		if(lichtschranke() || ultraschall()) {
			if(!AlarmOn){Serial.println("Alarm an");}
			AlarmOn = true;
		}
		else {
			if(AlarmOn){Serial.println("Alarm aus");}
			AlarmOn = false;
		}
	}
	else if(!kameras() && !digitalRead(RESET_PIN)) {
		AlarmOn = false;
		alarm(AlarmOn);
		Serial.println(F("Alarmsystem ausgeschaltet"));
		AlarmSystem = false;
	}
	else if(digitalRead(RESET_PIN)) {software_Reset();}

	alarm(AlarmOn);
}


void alarm(bool Alarm) {
	if(Alarm){digitalWrite(ALARM_PIN, HIGH);}
	else if(!Alarm){digitalWrite(ALARM_PIN, LOW);}
}

/* KAMERAS ------------------------------------------------------------------------*/
bool kameras() {
	bool AlarmSystemOn = true;
	bool CamCheck[camAnzahl] = {true, true};

	for(int i=0; i<camAnzahl; i++) {
		digitalWrite(camLedNR[i], digitalRead(camNR[i]));
		CamCheck[i] = digitalRead(camNR[i]);
	}
	AlarmSystemOn = CamCheck[0] || CamCheck[1];

return AlarmSystemOn;
}

bool lichtschranke() {
	bool AlarmLS = false;

	AlarmLS = digitalRead(SCHRANKE_PIN);
	AlarmLS = !AlarmLS;

return AlarmLS;
}

/* ULTRASCHALL --------------------------------------------------------------------*/
bool ultraschall() {
	bool AlarmUS = false;
	const int itera = 1;
	long dauer = 0;
	long entfernung[itera] = {0};
	long entfernung_average = 0;

	for(int i=0; i<usAnzahl; i++) {
		for(int j=0; j<itera; j++) {
			digitalWrite(usTriggerNR[i], LOW);
			delay(5);
			digitalWrite(usTriggerNR[i], HIGH);
			delay(10);
			digitalWrite(usTriggerNR[i], LOW);
			dauer = pulseIn(usEchoNR[i], HIGH);
			entfernung[j] = (dauer/2) * 0.03432;
		}
		entfernung_average = floating_average(entfernung, itera);
		if(entfernung_average <= (minDistanz[i]*0.8)) {AlarmUS = true;}
	}
return AlarmUS;
}

long ultraschall_kalibrierung(int trigger_pin, int echo_pin) {				// Messung der Entfernung im leeren Raum
	long normDistanz = 0;
	long dauer = 0;

	digitalWrite(trigger_pin, LOW);
	delay(5);
	digitalWrite(trigger_pin, HIGH);
	delay(10);
	digitalWrite(trigger_pin, LOW);
	dauer = pulseIn(echo_pin, HIGH);
	normDistanz = (dauer/2) * 0.03432;

return normDistanz;
}

long floating_average(long *value_average, int iterationen) {
	long average = 0;

	for(int i=0; i<iterationen; i++) {
		average += value_average[i];
	}
	average = average/iterationen;

return average;
}

/* RESET --------------------------------------------------------------------------*/
void software_Reset() {
  Serial.println(F("Neustart in"));
  delay(500);
  for (byte i = 3; i>0; i--) {
    Serial.println(i);
    delay(1000);
  }
  asm volatile ("  jmp 0");
}
