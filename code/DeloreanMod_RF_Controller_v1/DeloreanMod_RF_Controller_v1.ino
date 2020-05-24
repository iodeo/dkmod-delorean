/*
 * Sketch to be used within Delorean_RF_Controller v1 hardware from dkMOD
 * Free software under GNU v3 License
 * Copyright henrio-net, 2020
 */

#include <EEPROM.h>
#include <RCSwitch.h>
#include "Arduino.h"

// Uncomment to enable debug on serial port
//#define DEBUG 1

// eaglemoss PCB
#define EA_BP1 9 // pin to button1(R)
#define EA_BP2 8 // pin to button2
#define EA_BP3 7 // pin to button3
#define EA_BP4 6 // pin to button4
#define EA_BP5 5 // pin to button5
#define EA_BP6 4 // pin to button6(F)
#define EA_POWER 17 // pin to MOSFET (A3)
#define DELAY_OUTPUT 100 // delay after acting on a button

// Parameters for RF Remote Control
#define LONG_PRESS_DELAY 250 // Threshold between count
#define LONG_PRESS_COUNT 4 // Threshold between  Short and Long push
#define PARAM_RF_TIMEOUT 10000 // TimeOut for RF code detection (waiting for rf button push)
#define N_RF_BUTTONS 6 // Number of buttons on remote control ! CHANGE SEQ_MAX_SIZE ACCORDINGLY

// On-Board button
#define LEARN_PIN 3 // pin to on-board button for config

// Parameter for Sequencer
#define SEQ_MAX_SIZE 300 // max 332 with 6 RF buttons

// EEPROM addresses
#define EE_ADDR_RFCODES 2 // 4*N_RF_BUTTONS bytes alloc.
#define EE_ADDR_SEQUENCE 2+4*N_RF_BUTTONS // up to 990 bytes


// BUILT-IN LED CONTROL
#define QUICK_BLK 5 // blink time for Short pushes
#define NORM_BLK 10 // blink time for Short pushes
#define LONG_BLK 25 // blink time for Long Pushes

/*// ARDUINO extra pins
// digital
#define EXT_11 11 // D11 | MOSI
#define EXT_12 12 // D12 | MISO
#define EXT_13 13 // D13 | SCK | BUILT-IN-LED
// analog (! pay attention to ADC config)
#define EXT_A0 14 // A0
#define EXT_A1 15 // A1
#define EXT_A2 16 // A2
*/

// Declarations for Remote Control
RCSwitch mySwitch = RCSwitch();
struct RfParamStr {
  unsigned long code[N_RF_BUTTONS];
};
struct RfCommandStr {
  bool longPush;
  unsigned long code;
};
RfParamStr rfParam;

// Declaration for EaglemossPCB
const byte buttonPin[] = {EA_BP1,EA_BP2,EA_BP3,EA_BP4,EA_BP5,EA_BP6}; // write pin-out in an array for use in loops
bool powerOn = false;

// Other
unsigned long timer;

void setup() {
  
  //------ DISABLING ADC MODULES
  
  // Get a random seed first..
  randomSeed(analogRead(0));
  // Disable the ADC : ADEN bit (bit 7) of ADCSRA register to zero.
  ADCSRA = ADCSRA & B01111111;
  // Disable analog comparator : ACD bit (bit 7) of ACSR register to one.
  ACSR = B10000000;
  // Disable digital input buffers on all analog input pins : bits 0-5 of DIDR0 register to one.
  DIDR0 = DIDR0 | B00111111;

  //------ LAUNCH SERIAL COM FOR DEBUG 
  
  #ifdef DEBUG
    Serial.begin(9600);
  #endif

  //------ INITIALIZE DIGITAL PINS
  
  // Built-in led
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  // On-Board switch
  pinMode(LEARN_PIN, INPUT_PULLUP);
  // Pins to be used in parallel with Eaglemoss buttons
  initializeButtonPins();
  // Pin to command eaglemoss pcb power
  pinMode(EA_POWER, OUTPUT);
  digitalWrite(EA_POWER, HIGH);
  

  //------ REMOTE CONTROL PARAMETERS
  
  mySwitch.enableReceive(0); //digitalPinToInterrupt(2) gives 0
  
  //-- RECOVERING PARAMETERS FROM EEPROM
  
  EEPROM.get(EE_ADDR_RFCODES, rfParam);
  #ifdef DEBUG
    Serial.println(F("Recovering rf parameters from EEPROM"));
    for (byte i = 0; i < N_RF_BUTTONS; i++) Serial.println("  code " + String(i+1) + ": " + rfParam.code[i]);
  #endif
  
  // -- ENTERING LEARNING MODE (IF LEARN_PIN IS LOW)
  
  timer = millis();
  while( digitalRead(LEARN_PIN)==LOW && millis()-timer < LONG_PRESS_DELAY ) delay(50);
  if (millis()-timer >= LONG_PRESS_DELAY) {
    // LEARNING MODE
    blinkLed(10,NORM_BLK);
    #ifdef DEBUG
      Serial.println(F("Entering rf learning mode"));
    #endif
    RfCommandStr rfCommand;
    rfCommand.longPush = false;
    rfCommand.code = 0;
    for (byte i = 0; i < N_RF_BUTTONS; i++) {
      #ifdef DEBUG
        Serial.println("  Setting button " + String(i+1));
      #endif
      timer = millis();
      while( !mySwitch.available() && (millis()-timer < PARAM_RF_TIMEOUT) ) {
        delay(50);
      }
      if (millis()-timer <= PARAM_RF_TIMEOUT) {
        // Command received before timeout
        rfCommand = getRfCommand();
      }
      else rfCommand.code = 0;
      #ifdef DEBUG
        Serial.print(F("   longPush = ")); Serial.println(rfCommand.longPush);
        Serial.print(F("   code = ")); Serial.println(rfCommand.code);
      #endif
      if (rfCommand.code > 0) {
        #ifdef DEBUG
          Serial.println("  Got command -> setting code = " + rfCommand.code);
        #endif
        rfParam.code[i] = rfCommand.code;
        blinkLed(5,NORM_BLK);
      }
      else {
        #ifdef DEBUG
          Serial.println(F("  Didn't get code.. keeping old one"));
        #endif
        blinkLed(5,QUICK_BLK);
      }
    }
    #ifdef DEBUG
      Serial.println(F("  Operation ended correctly -> Saving parameters"));
    #endif
    EEPROM.put(EE_ADDR_RFCODES, rfParam);
    blinkLed(10,QUICK_BLK);
  }
  
  // COMMAND ALL EAGLEMOSS PCB BUTTONS
  // uncomment to power on all buttons after setup
  // pushAllButtons();
  
}

void loop() {
  
  RfCommandStr rfCommand = getRfCommand();
  if (rfCommand.code != 0) {
    
    // SHORT PUSH COMMANDS
    if (rfCommand.longPush == false) {

      blinkLed(1,QUICK_BLK);
      
      // ACT ON EAGLEMOSS BUTTONS
      for (byte i = 0; i < 6; i++) {
        if (rfCommand.code == rfParam.code[i]) {
          #ifdef DEBUG
            Serial.print(F("Short push on rf_button "));
            Serial.print(i+1); Serial.println(" detected");
          #endif
          // powering eaglemoss pcb if needed
          if (!powerOn) powerOn = turnPowerOn();
          pushButton(i);          
        }
      }
    }
    
    // LONG PUSH COMMANDS
    else {

      blinkLed(1,LONG_BLK);

      // POWER ON/OFF - Button 6
      if (rfCommand.code == rfParam.code[5]) {
        #ifdef DEBUG
          Serial.println(F("Long push on rf_button 6 detected : POWER ON/OFF"));
        #endif
        if (powerOn) {
          powerOn = turnPowerOff();
          delay(500);
          blinkLed(2,QUICK_BLK);
        }
        else {
          turnPowerOff(); // reinit in case of external actions on BPs
          powerOn = turnPowerOn();
          pushAllButtons();
          delay(500);
          blinkLed(1,QUICK_BLK);
        }
      }

      //PLAY OR RECORD SEQUENCE - Button 5
      if (rfCommand.code == rfParam.code[4]) {
        #ifdef DEBUG
          Serial.println(F("Long push on rf_button 5 detected : SEQUENCER"));
        #endif       
        timer = millis();
        while( digitalRead(LEARN_PIN)==LOW && millis()-timer < LONG_PRESS_DELAY ) delay(50);
        if (millis()-timer >= LONG_PRESS_DELAY) {
          #ifdef DEBUG
            Serial.println(F("LEARN_PIN is LOW > Record sequence"));
          #endif
          delay(500);
          blinkLed(2,QUICK_BLK);
          recordSequence(EE_ADDR_SEQUENCE);
          delay(500);
          blinkLed(2,QUICK_BLK);
        }
        else {
          #ifdef DEBUG
            Serial.println(F("LEARN_PIN is HIGH > Play sequence"));
          #endif
          delay(500);
          blinkLed(1,QUICK_BLK);
          playSequence(EE_ADDR_SEQUENCE);
          delay(500);
          blinkLed(1,QUICK_BLK);
        }
      }

/* DISABLED LONGPUSH BUTTONS
      // LONG PUSH ON BUTTON 1
      if (rfCommand.code == rfParam.code[0]) {
        #ifdef DEBUG
          Serial.println(F("Long push on rf_button 1 detected"));
        #endif
        // Put code to execute here
      }
            // LONG PUSH ON BUTTON 2
      if (rfCommand.code == rfParam.code[1]) {
        #ifdef DEBUG
          Serial.println(F("Long push on rf_button 2 detected"));
        #endif
        // Put code to execute here
      }
            // LONG PUSH ON BUTTON 3
      if (rfCommand.code == rfParam.code[2]) {
        #ifdef DEBUG
          Serial.println(F("Long push on rf_button 3 detected"));
        #endif
        // Put code to execute here
      }
            // LONG PUSH ON BUTTON 4
      if (rfCommand.code == rfParam.code[3]) {
        #ifdef DEBUG
          Serial.println(F("Long push on rf_button 4 detected"));
        #endif
        // Put code to execute here
      }
-- DISABLED LONGPUSH BUTTONS */

    }
  }
}

//------ FUNCTIONS

//-- POWER INTERFACE

bool turnPowerOn() {
  #ifdef DEBUG
    Serial.println(F("call turnPowerOn"));
  #endif
  // powering eaglemoss pcb
  digitalWrite(EA_POWER,HIGH);
  return true;
}

bool turnPowerOff() {
  #ifdef DEBUG
    Serial.println(F("call turnPowerOff"));
  #endif
  // unpowering eaglemoss pcb
  digitalWrite(EA_POWER,LOW);
  //comment below to disable eaglemoss pcb when poweroff
  delay(50); digitalWrite(EA_POWER,HIGH);
  return false;
}

//-- IOs INTERFACE

void blinkLed(int number, int time_factor) {
  /* examples
   * time_factor 10 = cycle of 0.5s (350 on + 150 off)
   * time_factor 20 = cycle of 1s   (700 on + 300 off)
   * etc..
  */
  for (byte i = 0; i < number; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(35*time_factor);
    digitalWrite(LED_BUILTIN, LOW);
    delay(15*time_factor);
  }
}

void initializeButtonPins() {  
  // Set pins nominal mode to avoid shortcut
  for (byte i = 0; i < 6; i++) {
    pinMode(buttonPin[i], INPUT_PULLUP);
  }
}

void pushButton(int i) {
  #ifdef DEBUG
    Serial.print(F("Command button ")); Serial.println(i+1);
  #endif
  // send button command to eaglemoss pcb
  pinMode(buttonPin[i], OUTPUT);
  digitalWrite(buttonPin[i], LOW);
  delay(DELAY_OUTPUT);
  pinMode(buttonPin[i], INPUT_PULLUP);
}

void pushAllButtons() {
  #ifdef DEBUG
    Serial.println(F("Command all buttons :"));
  #endif
  for (byte i = 0; i < 6; i++) pushButton(i);
}

// -- RF UTILS

RfCommandStr getRfCommand() {

  RfCommandStr rfCommand;
  rfCommand.code = 0;
  rfCommand.longPush = false;
  unsigned long code = 0;
  int count = 0;
  
  while (mySwitch.available()) {
    if (mySwitch.getReceivedValue()==code) {
      count++;
    }
    else {
      code = mySwitch.getReceivedValue();
      count = 1;
    }    
    mySwitch.resetAvailable();
    delay(LONG_PRESS_DELAY);
  }
  if ( code != 0 ) {
    rfCommand.code = code;
    rfCommand.longPush = ( count >= LONG_PRESS_COUNT )?true:false;
    #ifdef DEBUG
      Serial.println(F("Getting rf command"));
      if (rfCommand.longPush == true) {
        Serial.print(F(" > Got LONG push with CODE = "));
      }
      else {
        Serial.print(F(" > Got SHORT push with CODE = "));  
      }
      Serial.print(code);
      Serial.print(F(" and COUNT = "));
      Serial.println(count);
    #endif
  }
  return rfCommand;
}

// -- SEQUENCER UTILS

void recordSequence(int ADDR) {

  // RESET SEQUENCE MEMORY
  for (int i = ADDR; i < ADDR+2+3*SEQ_MAX_SIZE; i++) EEPROM.put(i,0x00);

  // INITIALIZE SEQUENCE
  int index = 0;
  unsigned long refTime;
  unsigned long timing_ms;
  unsigned int timing;
  
  // reinitialize buttons
  powerOn = turnPowerOff();
  powerOn = turnPowerOn();
  refTime = millis();
  delay(1000);
  
  // ACQUISITION
  byte pinTriggered = -1; //same as 255 (means no pin triggered)
  unsigned long rfCode = 0;
  while (index < SEQ_MAX_SIZE) {
    rfCode = getRfCommand().code;
    if (rfCode == rfParam.code[4] || rfCode == rfParam.code[5]) break;
    timing_ms = millis()-refTime;
    if (timing_ms > 605000) break;    
    for (byte i = 0; i < 6; i++) {
      if (digitalRead(buttonPin[i]) == LOW) { // button is active
        if (pinTriggered != i) { // and it is a trigger
          digitalWrite(LED_BUILTIN, HIGH);
          pinTriggered = i;
          timing = timing_ms / 10; //ms -> 0.01s
          EEPROM.put(ADDR, timing);
          ADDR+=2; //sizeof(unsigned int)
          EEPROM.put(ADDR, i);
          ADDR+=1; //sizeof(byte)
          index++;
          digitalWrite(LED_BUILTIN, LOW);
        }
      }
      else { // button is not active
        if (pinTriggered == i) pinTriggered = -1; // it has been released
      }
    }
    delay(50); //DELAY_OUTPUT - 50 due to EEPROM write time
  }
  #ifdef DEBUG
    Serial.println(F("Finish recording"));
  #endif
}

void playSequence(int ADDR) {
  
  unsigned long refTime;
  unsigned long timing_ms;
  unsigned int timing[SEQ_MAX_SIZE];
  byte pin[SEQ_MAX_SIZE];
  bool stopSequence = false;

  powerOn = turnPowerOff();

  // recover sequence from EEPROM
  EEPROM.get(ADDR, timing[0]);
  ADDR+=2; //sizeof(unsigned int)
  EEPROM.get(ADDR, pin[0]);
  ADDR+=1; //sizeof(byte)
  int index = 1;
  while (index < SEQ_MAX_SIZE && timing[index-1] != 0) {
    EEPROM.get(ADDR, timing[index]);
    ADDR+=2; //sizeof(unsigned int)
    EEPROM.get(ADDR, pin[index]);
    ADDR+=1; //sizeof(byte)
    index++;
  }
  index--;

  #ifdef DEBUG
    Serial.println("F(SEQUENCE READ FROM EEPROM)");
    Serial.print(F(" - Number of actions: ")); Serial.println(index);
    Serial.println(F(" - Actions table (timing [ms], pin):"));
    for (int i = 0; i < index; i++) {
      Serial.print("\t");Serial.print(timing[i]);
      Serial.print("\t");Serial.println(pin[i]);
    }
  #endif
  
  powerOn = turnPowerOn();

  // play the sequence
  refTime = millis();
  
  #ifdef DEBUG
    Serial.println(" ** ref Time = " + String(refTime));
  #endif

  unsigned long rfCode = 0;
  
  for (int i = 0; i < index; i++) {
    timing_ms = timing[i];
    timing_ms *= 10;
    #ifdef DEBUG
      Serial.print(F(" > timing_ms = "));Serial.println(timing_ms);
    #endif
    while(millis()-refTime < timing_ms) {
      rfCode = getRfCommand().code;
      if (rfCode == rfParam.code[4] || rfCode == rfParam.code[5]) {
        #ifdef DEBUG
          Serial.println(F(" > Sequence stopped"));
        #endif
        stopSequence = true;
        break;
      }
    }
    if (stopSequence) break;
    pushButton(pin[i]);
  }
  #ifdef DEBUG
    Serial.println(F("END OF SEQUENCE"));
  #endif
  if (stopSequence) powerOn = turnPowerOff();
}

// -- DEBUG UTILS

#ifdef DEBUG  

  void printEEPROM() {
    Serial.println("EEPROM content (ADDR|VALUE)");
    for (unsigned int index = 0 ; index < EEPROM.length() ; index++) {
      Serial.print("\t");Serial.print(index);
      Serial.print("\t");Serial.println(EEPROM.read(index));
    }
  }
  
#endif
