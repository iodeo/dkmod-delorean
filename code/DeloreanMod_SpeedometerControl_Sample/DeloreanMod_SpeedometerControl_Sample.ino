
// SPEEDOMETER PIN DEFINITION 
//  define where you connect speedometer SYNC wire on your arduino
#define SPEEDO_PIN 5

 
void setup() {

  // SPEEDOMETER SETUP
  //  speedometer stays in 0MPH STATE if it is powered after arduino
  //  if not, it may goes to SPEEDUP STATE in case it is pulled-down which depends on your system
  pinMode(SPEEDO_PIN, OUTPUT);
  digitalWrite(SPEEDO_PIN, HIGH);

  delay(5000); // delay is only for sample code
  
}

void loop() {

  // SPEEDOMETER START
  //  when getting LOW on SYNC pin, speedometer goes in SPEEDUP STATE
  //  it will speed-up for about 30 sec before timeout
  digitalWrite(SPEEDO_PIN, LOW);

  
  delay(25000);


  // SPEEDOMETER 88MPH
  //  when getting HIGH on SYNC pin, speedometer goes in 88MPH STATE 
  //  it will increase and stay at 88mph for about 10sec before timeout
  //  if SYNC pin get back to LOW before timeout, it will :
  //    -wait for about 3.75sec,
  //    -decrease speed quickly to 0mph (LOOP STATE)
  //    -then go in SPEEDUP STATE
  //  if timeout is reached it will : 
  //    -slowly go down to 0mp
  //    -then go in 0MPH STATE
  digitalWrite(SPEEDO_PIN, HIGH);
  

  delay(5000); // delay is only for sample code

}
