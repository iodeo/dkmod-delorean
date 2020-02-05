// Connexion des fils : Rouge=5V Noir=11 Bleu=GND
// accessibles en broche mâle sur le port ICSP ;)

#define PIN_SWITCHBOX 11 //UNO
//#define PIN_SWITCHBOX 51 //MEGA
bool switchBox = LOW;

void setup() { 
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_SWITCHBOX, OUTPUT);
}

void loop() {
  switchBox = HIGH;
  digitalWrite(LED_BUILTIN, HIGH); // Arduino Built-in Led is ON
  digitalWrite(PIN_SWITCHBOX, !switchBox); // Green LED is ON
  delay(1000);
  switchBox = LOW;
  digitalWrite(LED_BUILTIN, LOW); // Arduino Built-in Led is OFF
  digitalWrite(PIN_SWITCHBOX, !switchBox); // Red LED is ON
  delay(1000);
}
