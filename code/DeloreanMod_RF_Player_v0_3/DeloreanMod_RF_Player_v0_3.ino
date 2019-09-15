/*
 * Sketch to be used within Delorean_RF_Player v0.3 hardware from dkMOD
 * Free software under GNU v3 License
 * Copyright henrio-net, 2019
 */

#include <EEPROM.h>
#include <RCSwitch.h>
#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

// Uncomment to enable debug on serial port
//#define DEBUG 1

// eaglemoss PCB
#define EA_BP1 8 // pin to button1(R)
#define EA_BP2 7 // pin to button2
#define EA_BP3 6 // pin to button3
#define EA_BP4 5 // pin to button4
#define EA_BP5 4 // pin to button5
#define EA_BP6 3 // pin to button6(F)
#define EA_POWER 16 // pin to MOSFET (A2)
#define DELAY_OUTPUT 100 // delay after acting on a button

// dfPlayer
#define DF_TX 10 // pin to DFPlayer Tx
#define DF_RX 11 // pin to DFPlayer Rx
#define DF_CONNECT_MAX_COUNT 5 // Max connection tentatives
#define DF_READ_MAX_COUNT 25 // Max tentatives to get tracks number

// Parameters for RF Remote Control
#define LONG_PRESS_DELAY 250 // Threshold between count
#define LONG_PRESS_COUNT 4 // Threshold between  Short and Long push
#define PARAM_RF_TIMEOUT 10000 // TimeOut for RF code detection (waiting for rf button push)
#define N_RF_BUTTONS 6 // Number of buttons on remote control

// On-Board button
#define CONFIG_PIN 17 // pin to on-board button for config (A3)

// Parameter for Sequencer
#define SEQ_MAX_SIZE 329 // max number of actions
#define TIME_OFFSET 2000

// EEPROM addresses
#define EE_ADDR_VOLUME 0  //1 byte allocated (DFPlayer volume)
#define EE_ADDR_EQUALIZER 1 // 1 byte alloc. (DFPlayer equalizer)
#define EE_ADDR_RFCODES 2 // 4*N_RF_BUTTONS bytes alloc.
#define EE_ADDR_SEQUENCE 2+4*N_RF_BUTTONS // up to 990 bytes


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

// Declarations for MP3 Player
SoftwareSerial mySoftwareSerial(DF_TX, DF_RX); // ATMEGA RX, TX
DFRobotDFPlayerMini myDFPlayer;
byte dfVolume;
byte dfEqualizer;
bool dfPlay = true;
bool dfStop = false;
bool dfConnected = false;
bool dfStopAfterTrack = false; // Used to cut music at end of track
unsigned int advertsNumber = 0;
unsigned int tracksNumber = 0;
unsigned int track = 1;

// Declaration for EaglemossPCB
const byte buttonPin[] = {EA_BP1,EA_BP2,EA_BP3,EA_BP4,EA_BP5,EA_BP6}; // write pin-out in an array for use in loops
bool powerOn = true;

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
  /* Disable digital input buffers on all analog input pins : bits 0-5 of DIDR0 register to one.
  DIDR0 = DIDR0 | B00111111;*/
  // Disable digital input buffers on analog input pins (excepts A3 = CONFIG_PIN)
  DIDR0 = DIDR0 | B00110111;


  //------ LAUNCH SERIAL COM FOR DEBUG 
  
  #ifdef DEBUG
    Serial.begin(9600);
  #endif

  //------ INITIALIZE DIGITAL PINS
  
  // Built-in led
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  // On-Board switch
  pinMode(CONFIG_PIN, INPUT_PULLUP);
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
  
  // -- ENTERING LEARNING MODE (IF CONFIG_PIN IS LOW)
  
  timer = millis();
  while( digitalRead(CONFIG_PIN)==LOW && millis()-timer < LONG_PRESS_DELAY ) delay(50);
  if (millis()-timer >= LONG_PRESS_DELAY) {
    // LEARNING MODE
    blinkLed(10,10);
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
      /*if ( (rfCommand.code > 0) && (rfCommand.longPush == true) ) {
        #ifdef DEBUG
          Serial.println("  Got longPush -> setting code = " + rfCommand.code);
        #endif
        // Buffering new code if long push - for consistence
        rfParam.code[i] = rfCommand.code;
        blinkLed(5,10); //blink slow
      }*/
      if (rfCommand.code > 0) {
        #ifdef DEBUG
          Serial.println("  Got command -> setting code = " + rfCommand.code);
        #endif
        // Buffering new code if long push - for consistence
        rfParam.code[i] = rfCommand.code;
        blinkLed(5,10); //blink slow
      }
      else {
        #ifdef DEBUG
          Serial.println(F("  Didn't get code.. keeping old one"));
        #endif
        blinkLed(10,5); //blink quick
      }
    }
    #ifdef DEBUG
      Serial.println(F("  Operation ended correctly -> Saving parameters"));
    #endif
    EEPROM.put(EE_ADDR_RFCODES, rfParam);
    blinkLed(5,3); //blink slow
  }


  //------ INITIALIZE DFPLAYER IF PRESENT
  //--  Recovering parameters from EEPROM
  EEPROM.get(EE_ADDR_VOLUME, dfVolume);
  dfVolume = max(15,dfVolume%31);
  EEPROM.get(EE_ADDR_EQUALIZER, dfEqualizer);
  dfEqualizer = dfEqualizer%6;
  #ifdef DEBUG
    Serial.println(F("Recovering dfPlayer parameters from EEPROM"));
    Serial.print(F("  Volume : ")); Serial.println(dfVolume);
    Serial.print(F("  Equalizer : ")); Serial.println(dfEqualizer);
  #endif
  //-- Start communication
  dfConnected = connectDFPlayer();

  // COMMAND ALL EAGLEMOSS PCB BUTTONS
  // uncomment to power on all buttons after setup
  // pushAllButtons();
  
}

void loop() {
  
  RfCommandStr rfCommand = getRfCommand();
  if (rfCommand.code != 0) {
    
    // SHORT PUSH COMMANDS
    if (rfCommand.longPush == false) {
      
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
          blinkLed(1,5);
        }
      }
      /*
      // OPTIONNAL RF BUTTON 7 AND 8

      // PRINT EEPROM
      if (rfCommand.code == rfParam.code[6]) {
        #ifdef DEBUG
          Serial.print(F("Short push on rf_button 7"));
        #endif
        printEEPROM();
      }

      // PLAY ALL ADVERTISE
      if (rfCommand.code == rfParam.code[6]) {
        #ifdef DEBUG
          Serial.println(F("Short push on rf_button 8"));
        #endif
        for (unsigned int i = 0; i < advertsNumber; i++) {
          Serial.print("Playing adv number "); Serial.println(i+1);
          myDFPlayer.advertise(i+1);
          delay(5000);
        }
      }
      */
      
      // shutdown after track
      dfStopAfterTrack = false;
    }
    
    // LONG PUSH COMMANDS
    else {

      // POWER ON/OFF
      if (rfCommand.code == rfParam.code[5]) {
        #ifdef DEBUG
          Serial.println(F("Long push on rf_button 6 detected : POWER ON/OFF"));
        #endif
        track = 1;
        if (powerOn || !dfStop) powerOn = turnPowerOff();
        else {
          powerOn = turnPowerOn();
          if (dfConnected) {
            myDFPlayer.playMp3Folder(track);
            dfPlay = true;
            dfStop = false;
          }
          pushAllButtons();
        }
        // shutdown if after sequence
        dfStopAfterTrack = false;
        blinkLed(2,5);
      }

      //PLAY OR RECORD SEQUENCE
      if (rfCommand.code == rfParam.code[4]) {
        #ifdef DEBUG
          Serial.println(F("Long push on rf_button 6 detected"));
          Serial.println(F(" Sequencer mode"));
        #endif
        timer = millis();
        while( digitalRead(CONFIG_PIN)==LOW && millis()-timer < LONG_PRESS_DELAY ) delay(50);
        if (millis()-timer >= LONG_PRESS_DELAY) {
          #ifdef DEBUG
            Serial.println(F("CONFIG_PIN is LOW > Record sequence"));
          #endif
          recordSequence(EE_ADDR_SEQUENCE, (dfConnected && dfPlay)?track:0);
          dfStopAfterTrack = false;
        }
        else {
          #ifdef DEBUG
            Serial.println(F("CONFIG_PIN is HIGH > Play sequence"));
          #endif
          // shutdown if after sequence
          dfStopAfterTrack = playSequence(EE_ADDR_SEQUENCE);
          blinkLed(2,5);
        }
      }

      else {
        
        // DF PLAYER COMMANDS
        if (dfConnected) {
          
          // PLAY/STOP MUSIC
          if (rfCommand.code == rfParam.code[0]) {
            #ifdef DEBUG
              Serial.println(F("Long push on rf_button 1 detected : PLAY/PAUSE MUSIC"));
            #endif
            if (!dfStop) {
              /* play / pause
              (dfPlay)?myDFPlayer.pause():myDFPlayer.start();
              dfPlay=!dfPlay; */
              // play/stop
              myDFPlayer.stop();
              dfPlay = false;
              dfStop = true;
            }
            else { // if dfP sleeps, wake it up
              myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
              myDFPlayer.outputSetting(true, dfVolume);
              myDFPlayer.EQ(dfEqualizer);
              myDFPlayer.volume(dfVolume);
              myDFPlayer.playMp3Folder(track);
              dfStop = false;
              dfPlay = true;
              delay(100);
              //checkTrackNumber();
            }
            blinkLed(2,5);
            dfStopAfterTrack = false; 
          }

          /* PREVIOUS TRACK (LOOP)
          if (rfCommand.code == rfParam.code[2]) {
            #ifdef DEBUG
              Serial.println(F("Long push on rf_button 3 detected : PREVIOUS TRACK (LOOP)"));
            #endif
            //checkTrackNumber();
            (((track+=tracksNumber)-=2)%=tracksNumber)++;
            myDFPlayer.playMp3Folder(track);
            blinkLed(2,5);
            dfStopAfterTrack = false;
          }*/

          // NEXT TRACK (LOOP)
          if (rfCommand.code == rfParam.code[1]) {
            #ifdef DEBUG
              Serial.println(F("Long push on rf_button 2 detected : NEXT TRACK (LOOP)"));
            #endif
            //checkTrackNumber();
            (track%=tracksNumber)++;
            myDFPlayer.playMp3Folder(track);
            blinkLed(2,5);
            dfStopAfterTrack = false;
          }
              
          // VOLUME AND EQUALIZER
          if (rfCommand.code == rfParam.code[2]) {
            #ifdef DEBUG
              Serial.print(F("Long push on rf_button 3 detected : "));
            #endif
            // READ CONFIG BUTTON
            timer = millis();
            while( digitalRead(CONFIG_PIN)==LOW && millis()-timer < LONG_PRESS_DELAY ) delay(50);
            if (millis()-timer >= LONG_PRESS_DELAY) {
              #ifdef DEBUG
                Serial.println(F("EQUALIZER (Config_pin)"));
              #endif
              dfEqualizer=(dfEqualizer+1)%6;
              myDFPlayer.EQ(dfEqualizer);
              EEPROM.put(EE_ADDR_EQUALIZER, dfEqualizer);
              blinkLed(dfEqualizer+1,5);
            }
            else {
              #ifdef DEBUG
                Serial.println(F("VOLUME"));
              #endif
              dfVolume=(dfVolume-10)%15+15;
              myDFPlayer.volume(dfVolume);
              EEPROM.put(EE_ADDR_VOLUME, dfVolume);
              blinkLed(int((dfVolume-10)/5),5);
            }
          }

          // ADVERTISEMENT
          if (rfCommand.code == rfParam.code[3]) {
            #ifdef DEBUG
              Serial.println(F("Long push on rf_button 4 detected : ADVERTISE"));
            #endif
            //checkTrackNumber();
            if (advertsNumber != 0) {
              if (dfPlay) {
                #ifdef DEBUG
                  unsigned int adTrack = random(1, advertsNumber+1);
                  Serial.print(F("random track = "));
                  Serial.println(adTrack);
                  myDFPlayer.advertise(adTrack);
                #endif
                #ifndef DEBUG
                  myDFPlayer.advertise(random(1, advertsNumber+1));
                #endif
                blinkLed(2,5);
                dfStopAfterTrack = false;
              }
              else { // status = stop
                myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
                myDFPlayer.outputSetting(true, dfVolume);
                myDFPlayer.EQ(dfEqualizer);
                myDFPlayer.volume(dfVolume);
                #ifdef DEBUG
                  unsigned int adTrack = random(1, advertsNumber+1);
                  Serial.print(F("random track = "));
                  Serial.println(adTrack);
                  myDFPlayer.play(adTrack);
                #endif
                #ifndef DEBUG
                  myDFPlayer.play(random(1, advertsNumber+1));
                #endif
                // go back to sleep after playing
                blinkLed(2,5);
                dfStopAfterTrack = true;
              }
            }
            else {
              #ifdef DEBUG
                Serial.println(F(" NO ADVERTISEMENT FOUND !"));
              #endif
              blinkLed(10,2);
              dfStopAfterTrack = false;
            }
          }

              
          /* EQUALIZER
          if (rfCommand.code == rfParam.code[6]) {
            #ifdef DEBUG
              Serial.println(F("Long push on rf_button 4 detected : EQUALIZER CHANGE"));
            #endif
            dfEqualizer=(dfEqualizer+1)%6;
            myDFPlayer.EQ(dfEqualizer);
            EEPROM.put(EE_ADDR_EQUALIZER, dfEqualizer);
            blinkLed(dfEqualizer+1,5);
          }*/
        }
                  
        else { // No dfplayer connected ..
          #ifdef DEBUG
            Serial.println(F("Long push detected but no DFPlayer connected.."));
            Serial.println(F("Retry connection:"));
          #endif
          blinkLed(10,2);
          // Retry connect
          dfConnected = connectDFPlayer();
        }
      }
    }
  }

  // PLAY WHOLE MP3 PLAYLIST (EXCEPT IF IT IS END OF SEQUENCE)
  if (dfConnected) {
    if (myDFPlayer.available()) {
      #ifdef DEBUG
        Serial.println(F("DFPlayer available"));
      #endif
      bool forceNext = false;
      if (myDFPlayer.readType() == WrongStack) {
        // Serial com wrong > Check player status 
        if (dfPlay) {
          int tmp = -1;
          while (tmp == -1) {
            delay(100);
            tmp = myDFPlayer.readState();
            #ifdef DEBUG
              Serial.print(F("Stack Wrong > Read state ="));
              Serial.println(tmp);
            #endif
          }
          // player supposed to play but actually stopped
          if (tmp != 0x0201) forceNext = true;
        }
      }
      if (myDFPlayer.readType() == DFPlayerPlayFinished || forceNext) {
        #ifdef DEBUG
          Serial.println(F("Play Finished"));
        #endif
        // power off when track has finished (end of sequencer / advertiser while stop)
        if (dfStopAfterTrack) {
          #ifdef DEBUG
            Serial.println(F(" Stops after track"));
          #endif
          powerOn = turnPowerOff();
          dfStopAfterTrack = false;
        }
        else { // continue playing
          if (myDFPlayer.read() == track + advertsNumber || forceNext) { //avoid redondancy at end of track
            //checkTrackNumber();
            if (track==tracksNumber) {
              #ifdef DEBUG
                Serial.println(F("End of playlist > Stop playing"));
              #endif
              track = 1;
              powerOn = turnPowerOff();
            }
            else {
              #ifdef DEBUG
                Serial.println(F("Play next"));
              #endif
              unsigned int tmp = 0;
              track++;
              while (tmp != track + advertsNumber) {
                myDFPlayer.playMp3Folder(track);
                delay(200);
                tmp = myDFPlayer.readCurrentFileNumber();
                #ifdef DEBUG
                  Serial.print(F(" > track = "));Serial.print(track);
                  Serial.print(F("   | current = "));Serial.println(tmp - advertsNumber);
                #endif
              }
            }
          }
        }
      }
      #ifdef DEBUG
        else printDetail(myDFPlayer.readType(), myDFPlayer.read());
      #endif
    }
  }
  
}


//------ FUNCTIONS

//-- POWER INTERFACE

bool turnPowerOn() {
  // powering eaglemoss pcb
  digitalWrite(EA_POWER,HIGH);
  // restart music
  if (dfConnected && dfStop) {
    myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
    myDFPlayer.outputSetting(true, dfVolume);
    myDFPlayer.EQ(dfEqualizer);
    dfStop = false;
  }
  return true;
}

bool turnPowerOff() {
  // unpowering eaglemoss pcb
  digitalWrite(EA_POWER,LOW);
  //comment below to disable eaglemoss pcb when poweroff
  delay(50); digitalWrite(EA_POWER,HIGH);
  // stop music
  if (dfConnected) myDFPlayer.stop();
  dfPlay = false;
  dfStop = true;
  return false;
}

//-- IOs INTERFACE

void blinkLed(int number, int time_factor) {
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

// -- DFPlayer UTILS

bool connectDFPlayer() {
  track = 1;
  #ifdef DEBUG
    Serial.print(F("Initiating Communication with DFPlayer "));
  #endif
  int count = 0;
  int temp;
  bool isValidNumber = false;
  mySoftwareSerial.end();
  delay(100);
  mySoftwareSerial.begin(9600);
  delay(500);
  while ( count < DF_CONNECT_MAX_COUNT && !myDFPlayer.begin(mySoftwareSerial) ) {
    #ifdef DEBUG
      Serial.print(".");
    #endif
    mySoftwareSerial.end();
    delay(100);
    mySoftwareSerial.begin(9600);
    delay(500);
    count++;
  }
  #ifdef DEBUG
    Serial.println();
  #endif
  if (count < DF_CONNECT_MAX_COUNT) {
    #ifdef DEBUG
      Serial.println(F("DFPlayer Mini connnected"));
      Serial.println(F("Reading number of files .."));
    #endif
    //-- Get number of files advertisements and tracks
    // launch first track to get number of advertisements 
    advertsNumber = 0;
    tracksNumber = 0;
    myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
    delay(100);
    myDFPlayer.outputSetting(true, 0);
    delay(100);
    myDFPlayer.volume(0);
    delay(1000);
    myDFPlayer.playMp3Folder(track);
    delay(500);
    count = 0;
    isValidNumber = false;
    while (count < DF_READ_MAX_COUNT && !isValidNumber) {
      temp = myDFPlayer.readCurrentFileNumber();
      delay(100);
      #ifdef DEBUG
        Serial.print(F("  readCurrentFile = ")); Serial.print(temp);
      #endif
      isValidNumber = (temp > 0 && temp <= 3000);
      // Considered valid if repeated two times
      if (isValidNumber) {
        #ifdef DEBUG
          Serial.print(F("   - Repetition "));
        #endif
        isValidNumber = (temp == myDFPlayer.readCurrentFileNumber());
        delay(100);
        if (isValidNumber) {
          #ifdef DEBUG
            Serial.print(F("   1 .. "));
          #endif
          isValidNumber = (temp == myDFPlayer.readCurrentFileNumber());
          delay(100);
          #ifdef DEBUG
            if (isValidNumber) Serial.println(F(" and 2 !"));
            else Serial.println(F("   2 failed .."));
          #endif
        }
        #ifdef DEBUG
          else Serial.println(F(" 1 failed"));
        #endif
      }
      #ifdef DEBUG
        else Serial.println(F("   - Not valid"));
      #endif
      count++;
    }
    if (count <= DF_READ_MAX_COUNT) advertsNumber = temp - track;
    else advertsNumber = 0;
    // get number of tracks
    count = 0;
    isValidNumber = false;
    while (count < DF_READ_MAX_COUNT && !isValidNumber) {
            temp = myDFPlayer.readFileCounts();
      delay(100);
      #ifdef DEBUG
        Serial.print(F("  readFileCounts = ")); Serial.print(temp);
      #endif
      isValidNumber = (temp > 0 && temp <= 3000);
      // Considered valid if repeated two times
      if (isValidNumber) {
        #ifdef DEBUG
          Serial.print(F("   - Repetition "));
        #endif
        isValidNumber = (temp == myDFPlayer.readFileCounts());
        delay(100);
        if (isValidNumber) {
          #ifdef DEBUG
            Serial.print(F("   1 .. "));
          #endif
          isValidNumber = (temp == myDFPlayer.readFileCounts());
          delay(100);
          #ifdef DEBUG
            if (isValidNumber) Serial.println(F(" and 2 !"));
            else Serial.println(F("   2 failed .."));
          #endif
        }
        #ifdef DEBUG
          else Serial.println(F(" 1 failed"));
        #endif
      }
      #ifdef DEBUG
        else Serial.println(F("   - Not valid"));
      #endif
      count++;
    }
    if (count <= DF_READ_MAX_COUNT) tracksNumber = temp - advertsNumber;
    else tracksNumber = 1;
    #ifdef DEBUG
      Serial.print(F("  >>>  Number of tracks : ")); Serial.println(tracksNumber);
      Serial.print(F("  >>>  Number of advertisements : ")); Serial.println(advertsNumber);
    #endif
    // stop player and initialize parameters for future play
    myDFPlayer.stop();
    delay(100);
    myDFPlayer.outputSetting(true, dfVolume);
    delay(100);
    myDFPlayer.volume(dfVolume);
    delay(100);
    myDFPlayer.EQ(dfEqualizer);
    delay(100);
    dfPlay = false;
    dfStop = true;
    return true;
  }
  else {
    #ifdef DEBUG
      Serial.println(F("Unable to connect (bad wiring or SD card missing"));
    #endif
    mySoftwareSerial.end();
    return false;
  }
}

/* resolu
 * void checkTrackNumber() {
  if (advertsNumber > 3000 || tracksNumber > 3000) {
    if (dfPlay) {
      advertsNumber = myDFPlayer.readCurrentFileNumber() - track;
      delay(100);
      tracksNumber = myDFPlayer.readFileCounts() - advertsNumber;
      delay(100);
      //advertsNumber--; // due to silence sound file
    }
    if (dfStop) {
      //-- Get number of files advertisements and tracks
      myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
      delay(100);
      myDFPlayer.outputSetting(true, 0);
      delay(100);
      myDFPlayer.volume(0);
      delay(100);
      myDFPlayer.playMp3Folder(1);
      delay(100);
      myDFPlayer.pause();
      delay(100);
      advertsNumber = myDFPlayer.readCurrentFileNumber() - 1;
      delay(100);
      tracksNumber = myDFPlayer.readFileCounts() - advertsNumber;
      delay(100);
      //-- Set params
      myDFPlayer.outputSetting(true, dfVolume);
      delay(100);
      myDFPlayer.volume(dfVolume);
      delay(100);
      myDFPlayer.EQ(dfEqualizer);
      delay(100);
    }   
    #ifdef DEBUG
      Serial.println(F("Rectification of Tracks number"));
      Serial.print(F(" - Number of tracks: ")); Serial.println(tracksNumber);
      Serial.print(F(" - Number of advertisements: ")); Serial.println(advertsNumber);
    #endif
  }
}*/


// -- SEQUENCE RECORDER
void recordSequence(byte ADDR, unsigned int track) {

  // RESET SEQUENCE MEMORY
  for (int i = ADDR; i < ADDR+2+3*SEQ_MAX_SIZE; i++) EEPROM.put(i,0x00);

  // INITIALIZE SEQUENCE
  unsigned int sequenceTrack;
  sequenceTrack=(dfPlay)?track:0;
  EEPROM.put(ADDR, sequenceTrack);
  ADDR += 2; //sizeof(unsigned int)
  int index = 0;
  unsigned long refTime = millis();
  unsigned long timing_ms;
  unsigned int timing;
  powerOn = turnPowerOff();
  blinkLed(3,10);
  powerOn = turnPowerOn();
  if (dfConnected && sequenceTrack!=0) myDFPlayer.playMp3Folder(sequenceTrack);
  // ACQUISITION
  byte pinTriggered = -1; //same as 255 (means no pin triggered)
  while (index < SEQ_MAX_SIZE) {
    if (digitalRead(CONFIG_PIN) == LOW) break;
    for (byte i = 0; i < 6; i++) {
      if (digitalRead(buttonPin[i]) == LOW) { // button is active
        if (pinTriggered != i) { // and it is a trigger
          pinTriggered = i;
          pushButton(i);
          timing_ms = millis()-refTime;
          timing = int(0.1*timing_ms); //ms > 0.01s
          EEPROM.put(ADDR, timing);
          ADDR+=2; //sizeof(unsigned int)
          EEPROM.put(ADDR, i);
          ADDR+=1; //sizeof(byte)
          index++;
        }
      }
      else { // button is not active
        if (pinTriggered == i) pinTriggered = -1; // it can be triggered again
      }
    }
    delay(max(DELAY_OUTPUT - 50, 50)); //delay decreased due to EEPROM write time
  }
  blinkLed(10,5);
  #ifdef DEBUG
    Serial.println(F("Finish recording"));
  #endif
}

bool playSequence(byte ADDR) {
  
  unsigned long refTime;
  unsigned long timing_ms;
  unsigned int timing[SEQ_MAX_SIZE];
  byte pin[SEQ_MAX_SIZE];

  powerOn = turnPowerOff();

  // recover sequence from EEPROM
  EEPROM.get(ADDR, track);
  ADDR += 2; //sizeof(unsigned int)
  int index = 0;
  EEPROM.get(ADDR, timing[index]);
  ADDR+=2;
  EEPROM.get(ADDR, pin[index]);
  ADDR+=1;
  while (index < SEQ_MAX_SIZE && timing[index] != 0) {
    index++;
    EEPROM.get(ADDR, timing[index]);
    ADDR+=2; //sizeof(unsigned int)
    EEPROM.get(ADDR, pin[index]);
    ADDR+=1; //sizeof(byte)
  }

  #ifdef DEBUG
    Serial.println("F(SEQUENCE READ FROM EEPROM)");
    Serial.print(F(" - Track number: ")); Serial.println(track);
    Serial.print(F(" - Number of actions: ")); Serial.println(index+1);
    Serial.println(F(" - Actions table (timing [ms], pin):"));
    for (int i = 0; i < index; i++) {
      Serial.print("\t");Serial.print(timing[i]);
      Serial.print("\t");Serial.println(pin[i]);
    }
  #endif
  
  blinkLed(2,10);
  powerOn = turnPowerOn();

  // start music
  if (dfConnected && track!=0) {
    dfPlay = true;
    dfStop = false;
    myDFPlayer.playMp3Folder(track);
  }

  // play the sequence
  refTime = millis();
  refTime -= TIME_OFFSET;
  
  #ifdef DEBUG
      Serial.println("ref Time = " + String(refTime));
    #endif
  for (int i = 0; i < index; i++) {
    timing_ms = 10*timing[i];
    while(millis()-refTime < timing_ms) {
      delay(DELAY_OUTPUT);
    }
    #ifdef DEBUG
      Serial.println(timing[i]);
    #endif
    pushButton(pin[i]);
  }
  #ifdef DEBUG
    Serial.println(F("END OF SEQUENCE"));
  #endif

  return dfConnected;
  
}

#ifdef DEBUG

  // -- DEBUG FUNCTIONS

  void printEEPROM() {
    Serial.println("EEPROM content (ADDR|VALUE)");
    for (unsigned int index = 0 ; index < EEPROM.length() ; index++) {
      Serial.print("\t");Serial.print(index);
      Serial.print("\t");Serial.println(EEPROM.read(index));
    }
  }
  
  void printDetail(uint8_t type, int value){
    //usage: printDetail(myDFPlayer.readType(), myDFPlayer.read());
    switch (type) {
      case TimeOut:
        Serial.println("Time Out!");
        break;
      case WrongStack:
        Serial.println(F("Stack Wrong!"));
        break;
      case DFPlayerCardInserted:
        Serial.println(F("Card Inserted!"));
        break;
      case DFPlayerCardRemoved:
        Serial.println(F("Card Removed!"));
        break;
      case DFPlayerCardOnline:
        Serial.println(F("Card Online!"));
        break;
      case DFPlayerUSBInserted:
        Serial.println("USB Inserted!");
        break;
      case DFPlayerUSBRemoved:
        Serial.println("USB Removed!");
        break;
      case DFPlayerPlayFinished:
        Serial.print(F("Number:"));
        Serial.print(value);
        Serial.println(F(" Play Finished!"));
        break;
      case DFPlayerError:
        Serial.print(F("DFPlayerError:"));
        switch (value) {
          case Busy:
            Serial.println(F("Card not found"));
            break;
          case Sleeping:
            Serial.println(F("Sleeping"));
            break;
          case SerialWrongStack:
            Serial.println(F("Get Wrong Stack"));
            break;
          case CheckSumNotMatch:
            Serial.println(F("Check Sum Not Match"));
            break;
          case FileIndexOut:
            Serial.println(F("File Index Out of Bound"));
            break;
          case FileMismatch:
            Serial.println(F("Cannot Find File"));
            break;
          case Advertise:
            Serial.println(F("In Advertise"));
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }   
  }
  
#endif
