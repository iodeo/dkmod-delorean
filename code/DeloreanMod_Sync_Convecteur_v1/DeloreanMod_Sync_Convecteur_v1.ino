
/* --------------------------------
 *  State & timing definition
 */
#define INIT 0 // reinit at end of program
#define START 1 // bulbs flux steadily
#define SPEEDUP 2 // bulbs flux quicker and quicker
#define FINAL 3 // bulbs breath with flash

#define START_TIME 200
#define SPEEDUP_TIME 16000
#define FINAL_TIME 20000


/* --------------------------------
 *  Sync definition
 */
#define SYNC PIN_PB3
#define SYNC_PULSE_START_TIME 19000
#define SYNC_PULSE_END_TIME 19150
#define SYNC_WAIT 1
#define SYNC_PULSE 2
#define SYNC_END 3


/* --------------------------------
 *  Bulb lighting
 *    > use 3 pins pwm drive based on http://www.technoblogy.com/show?LE0
 *    > two modes: running (default) & breathing (final)
 */
#define NB_BULB_PINS 3
uint8_t bulbPins[NB_BULB_PINS] = {PIN_PB0, PIN_PB1, PIN_PB4}; // From outer leds to inner leds

volatile uint8_t* bulbPwmRegs[NB_BULB_PINS] = {&OCR0A, &OCR0B, &OCR1B}; // Bulb pwm reg defs

void pwmWrite (uint8_t pinIndex, uint8_t pwm) { 
  // pwm: 0 (OFF) - 255 (max intensity)
  *bulbPwmRegs[pinIndex] = 255 - pwm;
}

#define NB_STEP 20        // Number of sequence steps
#define INIT_PERIOD 150   // Target duration (ms) of 1 cycle
#define DECREASE_PERCENT 5 // speedup mode accelaration (the higher the quicker)
#define CYCLE_COUNT 2  // accelaration steps (the higher the slower)
#define MIN_PERIOD 50 // speedup mode maximum speed (the smaller the quicker)

#define INIT_DELAY (INIT_PERIOD / NB_STEP) // Delay between steps
#define MIN_DELAY (MIN_PERIOD / NB_STEP) // Delay between steps

// Bulb Running Leds Mode definition
#define DECAY_RUNNING 7 // Decay between LEDs (nb steps)
uint8_t runningPwms[NB_STEP] = {0, 0, 80, 80, 80, 80, 60, 40, 30, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // pwm levels from 0 (OFF) to 255 (max)

// Bulb Breathing Mode Definition (used in  FINAL state)
#define DECAY_BREATHING 6 // Decay between LEDs (nb steps)
uint8_t breathingPwms[NB_STEP] = {80, 85, 90, 95, 100, 110, 120, 130, 140, 145, 150, 140, 130, 120, 110, 105, 100, 95, 90, 85}; // pwm levels from 0 (OFF) to 255 (max)


/* --------------------------------
 *  Final Flash Sequence Definition
 *    > flashTiming : gives time at which leds flash (reference time is at start of flash mode)
 *    > flashDuration : gives the duration of the flash in millisecs
 *    > last one with 0 duration is required to wait end of flash before reinitialization
 */
#define BACKLIGHT PIN_PB2
#define BACKLIGHT_ON LOW
#define BACKLIGHT_OFF HIGH
#define NB_FLASH 17 // Number of sequence steps
/* long sequence
 *  uint16_t flashTiming[NB_FLASH] = {   0, 525, 775,1250,1450,1600,2235,2385,2550,2695,2775,2925,3005,3750,3950,4025,6900};
 *  uint16_t flashDuration[NB_FLASH] = {25,  25,  25,  25,  25,  25,  25,  25,  25,  25,  25,  25,  25,  25,  25,2870,   0};
 */
uint16_t flashTiming[NB_FLASH] = {   0, 260, 385,625,725,800,1115,1192,1275,1348,1388,1468,1502,1875,1975,2012,3000};
uint16_t flashDuration[NB_FLASH] = {25,  25,  25,  25,  25,  25,  25,  25,  20,  20,  25,  25,  25,  25,  25,2870,   0};


/* --------------------------------
 *  Global variables
 */
// system state and flag variables
uint8_t state = START;
bool initFlag = false;
bool endFlag = false;
byte syncState = SYNC_WAIT;
// variables to handle timing
uint32_t mainTime = 0;
uint32_t pwmTime = 0;
uint32_t flashTime = 0;
uint32_t flashPulseTime = 0;
uint16_t flashPulseWidth = 0;
// variable to handle sequences
uint8_t pwmStep = 0;
uint16_t pwmDelay = INIT_DELAY;
uint16_t cycleCount = 0;
uint8_t flashCount = 0;
bool isFlashing = false;


/* --------------------------------
 *  Program setup
 */
void setup() {

  // Configure bulb pins
  for (uint8_t i = 0; i < NB_BULB_PINS; i++) {
    digitalWrite(bulbPins[i], LOW);
    pwmWrite(i, 0);
    pinMode(bulbPins[i], OUTPUT);
  }
  // Configure counter/timer0 for fast PWM on PB0 and PB1
  TCCR0A = 3<<COM0A0 | 3<<COM0B0 | 3<<WGM00;
  TCCR0B = 0<<WGM02 | 3<<CS00; // Optional; already set
  // Configure counter/timer1 for fast PWM on PB4
  GTCCR = 1<<PWM1B | 3<<COM1B0;
  TCCR1 = 3<<COM1A0 | 7<<CS10;

  // Configure flash pin
  digitalWrite(BACKLIGHT, BACKLIGHT_OFF);
  pinMode(BACKLIGHT, OUTPUT);

  // Configure synchronization
  digitalWrite(SYNC, LOW);
  pinMode(SYNC, OUTPUT);
  
}


/* --------------------------------
 *  Program loop
 */
void loop() {
  
  uint16_t mainTimer = (uint16_t) (millis() - mainTime);
  
  // Synchronizing
  switch (syncState) {
    case SYNC_WAIT:
      if (mainTimer > SYNC_PULSE_START_TIME) {
        syncState = SYNC_PULSE;
        digitalWrite(SYNC,HIGH);
      }
      break;
    case SYNC_PULSE:
      if (mainTimer > SYNC_PULSE_END_TIME) {
        syncState = SYNC_END;
        digitalWrite(SYNC,LOW);
      }
      break;
    case SYNC_END:
      if (endFlag) {
        syncState = SYNC_WAIT;
      }
  }
  
  
  // State management
  switch (state) {
    case INIT:
      if (mainTimer > START_TIME) {
        // initiate START drive
        state = START;
        pwmStep = 0;
        pwmDelay = INIT_DELAY;
      }
      break;
    case START:
      if (mainTimer > SPEEDUP_TIME) {
        // initiate SPEEDUP drive
        state = SPEEDUP;
        cycleCount = 0;
        pwmDelay = INIT_DELAY;
      }
      break;
    case SPEEDUP:
      if (mainTimer > FINAL_TIME) {
        // initiate FINAL drive
        state = FINAL;
        pwmStep = 0;
        pwmDelay = INIT_DELAY;
        flashCount = 0;
        flashTime = millis();
      }
      break;
    case FINAL:
      if (endFlag) {
        endFlag = false;
        initFlag = true;
        mainTime = millis();
        state = INIT;
      }
      break;
    default:
      break;
  }
  
  // Flux capacitor drive
  switch (state) {
    case INIT:
      if (initFlag) {
        initFlag = false;
        for (uint8_t i = 0; i < NB_BULB_PINS; i++) pwmWrite(i, 0);
        digitalWrite(BACKLIGHT, BACKLIGHT_OFF);
      }
      break;
    case START:
      if (millis() - pwmTime > pwmDelay) {
        pwmTime = millis();
        pwmWrite(0, runningPwms[(pwmStep+2*DECAY_RUNNING)%NB_STEP]);
        pwmWrite(1, runningPwms[(pwmStep+DECAY_RUNNING)%NB_STEP]);
        pwmWrite(2, runningPwms[(pwmStep)%NB_STEP]);
        if (pwmStep < NB_STEP - 1) pwmStep++;
        else pwmStep = 0;
      }
      break;
    case SPEEDUP:
      if (millis() - pwmTime > pwmDelay) {
        pwmTime = millis();
        pwmWrite(0, runningPwms[(pwmStep+2*DECAY_RUNNING)%NB_STEP]);
        pwmWrite(1, runningPwms[(pwmStep+DECAY_RUNNING)%NB_STEP]);
        pwmWrite(2, runningPwms[(pwmStep)%NB_STEP]);
        if (pwmStep < NB_STEP - 1) pwmStep++;
        else {
          pwmStep = 0;
          if (cycleCount < CYCLE_COUNT - 1) cycleCount++;
          else {
            cycleCount = 0;
            if (pwmDelay > MIN_DELAY) {
              pwmDelay *= (100 - DECREASE_PERCENT);
              pwmDelay /= 100;
              if (pwmDelay < MIN_DELAY) pwmDelay = MIN_DELAY;
            }
          }
        }
      }
      break;
    case FINAL:
      if (millis() - pwmTime > pwmDelay) {
        pwmTime = millis();
        pwmWrite(0, breathingPwms[(pwmStep+2*DECAY_BREATHING)%NB_STEP]);
        pwmWrite(1, breathingPwms[(pwmStep+DECAY_BREATHING)%NB_STEP]);
        pwmWrite(2, breathingPwms[(pwmStep)%NB_STEP]);
        if (pwmStep < NB_STEP - 1) pwmStep++;
        else pwmStep = 0;
      }
      if (millis() - flashTime > flashTiming[flashCount]) {
        flashPulseTime = millis();
        flashPulseWidth = flashDuration[flashCount];
        if (flashPulseWidth != 0) {
          digitalWrite(BACKLIGHT, BACKLIGHT_ON);
          isFlashing = true;
        }
        if (flashCount < NB_FLASH-1) flashCount++;
        else endFlag = true;
      }
      if (isFlashing) {
        if (millis() - flashPulseTime > flashPulseWidth) {
          digitalWrite(BACKLIGHT, BACKLIGHT_OFF);
          isFlashing = false;
        }
      }
      break;
    default:
      break;
  }
}
