
// SYNC WIRING
#define AUX PIN_PB5 // to 3dynamics flux capacitor
#define SYNC PIN_PB6 // to dkmod speedometer

// LED MATRIX SIZE
#define NB_ROW 20
#define NB_COL 10

// PIN ASSIGNEMENT
const byte rowPin[NB_ROW] = {
  PIN_PA0, PIN_PA3, PIN_PA1, PIN_PA5, PIN_PA7, 
  PIN_PC6, PIN_PC4, PIN_PC2, PIN_PC3, PIN_PC0, 
  PIN_PB3, PIN_PB4, PIN_PB2, PIN_PB1, PIN_PB7, 
  PIN_PD0, PIN_PD1, PIN_PD2, PIN_PD4, PIN_PD3
};
/* prototype version 
 * permut. row11/row12
 * permut. row15/row16
 * permut. row19/row20
 
const byte rowPin[NB_ROW] = {
  PIN_PA0, PIN_PA3, PIN_PA1, PIN_PA5, PIN_PA7, 
  PIN_PC6, PIN_PC4, PIN_PC2, PIN_PC3, PIN_PC0, 
  PIN_PB4, PIN_PB3, PIN_PB2, PIN_PB1, PIN_PD0, 
  PIN_PB7, PIN_PD1, PIN_PD2, PIN_PD3, PIN_PD4
};*/
const byte colPin[NB_COL] = {
  PIN_PB0, PIN_PA2, PIN_PA4, PIN_PA6, PIN_PC7, 
  PIN_PC5, PIN_PC1, PIN_PD7, PIN_PD6, PIN_PD5
};

// DISPLAY PARAM
// 0 (minimum light) to 255 (maximum)
const byte brightness[NB_ROW] = {
  25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, // 13 rows of Green LEDs
  220, 220, 220, 220, 220, 220, // 6 rows of Yellow LEDs
  255 // 1 row of Red LEDs
};

// LED LOGIC
#define ROW_ON LOW
#define ROW_OFF HIGH
#define COL_ON HIGH
#define COL_OFF LOW

// SEQUENCE DEFINITION
// --> see _sequence.ino and _timing.ino files
#define OFF_MODE 0
#define OFF_TIMEOUT 200
#define REST_MODE 1
#define REST_NB_FRAME 62
#define REST_COUNTOUT 2
#define SPEEDUP_MODE 2
#define SPEEDUP_NB_FRAME 128
#define SPEEDUP_COUNTOUT_MASTER 2
#define SPEEDUP_COUNTOUT_SLAVE 3
#define FINAL_MODE 3
#define FINAL_NB_FRAME 57
#define FINAL_STARTING_FRAME_SLAVE 20 //adjust start of final sequence in slave mode
#define FINAL_TIMEOUT_SLAVE 5750 //adjust end of full display in slave mode
#define FINAL_COUNTOUT_MASTER 1

// SYNC VARIABLES
#define INIT_FLAG 0
#define PULSE_FLAG 1
#define END_FLAG 2
#define PULSE_DELAY 1500 //adjust start of flux flash in master mode
#define PULSE_TIME 3600 //adjust end of 88mph in master mode
bool isMaster;
byte masterFlag = INIT_FLAG;

// DISPLAY VARIABLES
byte mode = REST_MODE;
unsigned long timeout = -1;
unsigned int countout = REST_COUNTOUT;
unsigned int nbFrame = REST_NB_FRAME;
unsigned int counter = 0;
unsigned int frame;
unsigned long timer;
unsigned long timeRef;
unsigned long nextTime;
byte columnHeight[NB_COL];
bool updateFrameFlag = true;
bool resetTimerFlag = true;


void setup() {

  // Some register config
  ADCSRA &= ~(1<<ADEN); // Disable ADC
  ACSR |= (1<<ACD); // Disable ACD
  MCUCR &= ~(1<<PUD); // Enable input pullup 

  // Display pin config
  for (byte i = 0; i < NB_ROW; i++) {
    pinMode(rowPin[i], OUTPUT);
    digitalWrite(rowPin[i], ROW_OFF);
  }
  for (byte j = 0; j < NB_COL; j++) {
    pinMode(colPin[j], OUTPUT);
    digitalWrite(colPin[j], COL_OFF);
  }
  
  // Detect other devices and determine master mode
  pinMode(SYNC, INPUT_PULLUP);
  pinMode(AUX, OUTPUT);
  delay(500);
  
  if (digitalRead(SYNC) == HIGH) {
    // No flux capacitor detected on SYNC line,
    // So we assume xmastree has to drive speedometer on SYNC line and flux capa on AUX line 
    isMaster = true;
    // init SYNC line
    pinMode(SYNC, OUTPUT);
    digitalWrite(SYNC, LOW);
    // init AUX line
    digitalWrite(AUX, HIGH);
  }
  else {
    // Flux capacitor detected on SYNC line,
    // So we assume both xmastree and speedometer are driven by flux capa on SYNC line
    isMaster = false;
    // init SYNC line
    //pinMode(SYNC, INPUT_PULLUP);
    // AUX line
    digitalWrite(AUX, LOW);
    delay(1000);
  }
  
}

void loop() {
  
  // Update frame data
  if (updateFrameFlag) {
    updateFrameFlag = false;
    nextTime = getTiming(mode, frame);
    for (byte j = 0; j < NB_COL; j++) {
      columnHeight[j] = getColumnHeight(mode, frame, j);
    }
  }

  // Update timer
  if (resetTimerFlag) {
    resetTimerFlag = false;
    timer = 0;
    timeRef = millis() - nextTime;
  }
  else {
    timer = millis() - timeRef;
  }

  // Display frame
  for (byte i = 0; i < NB_ROW; i++) {   
    for (byte j = 0; j < NB_COL; j++) {
      if (i < columnHeight[j]) digitalWrite(colPin[j], COL_ON);
      else digitalWrite(colPin[j], COL_OFF);
    }
    digitalWrite(rowPin[i], ROW_ON);
    delayMicroseconds(brightness[i]);
    digitalWrite(rowPin[i], ROW_OFF);
    delayMicroseconds(255-brightness[i]);
  }
  
  // Manage frame
  if (timer >= nextTime) {
    updateFrameFlag = true;
    frame++;
    // End of sequence
    if (frame == nbFrame) {
      resetTimerFlag = true;
      frame = 0;
      counter++;
      if (timer < timeout) {
        timeout -= timer;
        timer = 0;
      }
    }
  }

  // Manage timeout & countout
  if ((timer >= timeout) || (counter >= countout)) {
    updateFrameFlag = true;
    resetTimerFlag = true;
    frame = 0;
    counter = 0;
    switch (mode) {
      case REST_MODE:
        mode = SPEEDUP_MODE;
        nbFrame = SPEEDUP_NB_FRAME;
        timeout = -1;
        if (isMaster) countout = SPEEDUP_COUNTOUT_MASTER;
        else countout = SPEEDUP_COUNTOUT_SLAVE;
        break;
      case SPEEDUP_MODE:
        mode = FINAL_MODE;
        masterFlag = PULSE_FLAG;
        nbFrame = FINAL_NB_FRAME;
        countout = FINAL_COUNTOUT_MASTER;
        timeout = -1;
        break;
      case FINAL_MODE:
        mode = OFF_MODE;
        timeout = OFF_TIMEOUT;
        break;
      case OFF_MODE:
        mode = REST_MODE;
        nbFrame = REST_NB_FRAME;
        timeout = -1;
        countout = REST_COUNTOUT;
        break;
      default:
        mode = OFF_MODE;
        timeout = -1;
        break;
    }
  }

  // Manage synchro
  if (isMaster) {
    if (masterFlag == PULSE_FLAG) {
      if (timer > PULSE_DELAY) {
        masterFlag = END_FLAG;
        digitalWrite(AUX, LOW);
        digitalWrite(SYNC, HIGH);
      }
    }
    else if (masterFlag == END_FLAG) {
      if (timer > PULSE_DELAY + PULSE_TIME) {
        masterFlag = INIT_FLAG;
        digitalWrite(SYNC, LOW);
      }
    }
    if (mode == OFF_MODE) {
      digitalWrite(AUX, HIGH);
    }
  }
  else {
    if (digitalRead(SYNC) == HIGH) {
      // got pulse from flux capacitor -> play final mode
      updateFrameFlag = true;
      resetTimerFlag = true;
      frame = FINAL_STARTING_FRAME_SLAVE; //advance starting frame
      counter = 0;
      mode = FINAL_MODE;
      nbFrame = FINAL_NB_FRAME;
      timeout = FINAL_TIMEOUT_SLAVE; // adjust to end of flash
      countout = -1;
    }
  }

}
