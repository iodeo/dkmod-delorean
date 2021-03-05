
/* 
 *  Pins definition
 */  

// comment out below to disable SYNC
#define SYNC PIN_PB5

// Segment i is lit when ((anodePins[i] == HIGH) && (cathodePins[i] == LOW))
const byte anodePins[15] = {
  // Digit 1 : a, b, c, d, e, f, g
  PIN_PB4, PIN_PB2, PIN_PB2, PIN_PB1, PIN_PB3, PIN_PB3, PIN_PB4,
  // Digit 2 : a, b, c, d, e, f, g, dot
  PIN_PB1, PIN_PB2, PIN_PB1, PIN_PB2, PIN_PB3, PIN_PB4, PIN_PB0, PIN_PB0
};
const byte cathodePins[15] = {
  // Digit 1 : a, b, c, d, e, f, g
  PIN_PB3, PIN_PB4, PIN_PB3, PIN_PB4, PIN_PB1, PIN_PB4, PIN_PB1,
  // Digit 2 : a, b, c, d, e, f, g, dot
  PIN_PB2, PIN_PB1, PIN_PB0, PIN_PB0, PIN_PB2, PIN_PB2, PIN_PB2, PIN_PB1
};


/*
 *  Display definition
 */
 
#define SWEEP_TIME 100 //µs
#define DOT_DISPLAY true //lit if true
byte digit[10] = {
  // 0 to 4
  0b1111110, 0b0110000, 0b1101101, 0b1111001, 0b0110011, 
  // 5 to 9
  0b1011011, 0b1011111, 0b1110000, 0b1111111, 0b1111011
};


/* 
 *  Machine states definition
 */
 
byte state = 0;
unsigned int vitesse = 0; // unit = mph * 100
#define SLOW_0MPH 0 // slow speed down to 00.
#define SPEED_UP 1 // speed up to 87.
#define SLOW_DOWN 2 // quick speed down to 00.
#define FINAL_88MPH 3 // reach 88.

#define SPEED_UP_TIMEOUT 30000 // never used
#define FINAL_TIMEOUT 10000
#define FINAL_LOOP_TIMER 3750

#define TIME_STEP 25 // speed refresh rate


/* 
 *  Setup
 */
 
void setup() {

  // Disable input pullup to avoid transient state
  MCUCR |= (1<<PUD); 
  // Disable the ADC : ADEN bit (bit 7) of ADCSRA register to zero.
  ADCSRA = ADCSRA & B01111111;
  // Disable analog comparator : ACD bit (bit 7) of ACSR register to one.
  ACSR = B10000000;
  
  pinMode(PIN_PB0, INPUT);
  pinMode(PIN_PB1, INPUT);
  pinMode(PIN_PB2, INPUT);
  pinMode(PIN_PB3, INPUT);
  pinMode(PIN_PB4, INPUT);
  
  // Pin configuration
#ifdef SYNC
  pinMode(SYNC, INPUT); // external pullup
#endif
  
}


/* 
 *  Global variables for main loop
 */

unsigned long timer; // use for final state delay
unsigned long timeout; // use for state management
unsigned long refreshTimer = 0; // use for speed update timing

bool timeoutFlag = false; // various timeout event
bool loopFlag = false; // go to next states in travel mode loop
bool refreshFlag = false; // refresh speed
byte phase = 0; // slow down phase


/* 
 *  Main loop
 */

void loop() {

  // state machine management
  if (state == SLOW_0MPH) {
#ifdef SYNC    
    if (digitalRead(SYNC) == LOW) {
      state = SPEED_UP;
      timeout = millis();
    }
#else 
    state = SPEED_UP;
    timeout = millis();
#endif      
      
  }
  if (state == SPEED_UP) {
#ifdef SYNC    
    if (digitalRead(SYNC) == HIGH) {
      state = FINAL_88MPH;
      timer = millis();
      timeout = timer;
    }
#endif
    if (timeoutFlag) {
      timeoutFlag = false;
      state = FINAL_88MPH;
      timer = millis();
      timeout = timer;
    }
  }
  if (state == FINAL_88MPH) {
    if (timeoutFlag) {
      timeoutFlag = false;
      state = SLOW_0MPH;
    }
    if (loopFlag) {
      loopFlag = false;
      state = SLOW_DOWN;
      phase = 1;
    }
  }
  if (state == SLOW_DOWN) {
    if (loopFlag) {
      loopFlag = false;
      state = SPEED_UP;
      timeout = millis();
    }
  }

  // timing management
  if (millis() - refreshTimer > TIME_STEP) {
    refreshTimer = millis();
    refreshFlag = true;
  }
  
  if (state == SPEED_UP) {
    // check for timeout
    if (millis() - timeout > SPEED_UP_TIMEOUT) {
      timeoutFlag = true;
    }
  }
  if (state == FINAL_88MPH) {
#ifdef SYNC
    // watch for end of pulse
    if (digitalRead(SYNC) == HIGH) {
      timer = millis();
      // check for timeout
      if (millis() - timeout > FINAL_TIMEOUT) {
        timeoutFlag = true;
      }
    }
#endif
    // wait given time after end of pulse
    else {
      if (millis() - timer > FINAL_LOOP_TIMER) {
        loopFlag = true;
      }
    }
  }

  // speed management
  if (refreshFlag) {
    refreshFlag = false;
    switch (state) {
      case SLOW_0MPH:
        if (vitesse > 100) {
          // U(n+1) = U(n) - 0.005*[U(n)+150]
          unsigned int tmp;
          tmp = vitesse + 150;
          tmp *= 5; // 5*(8800+150) = 44750 < 2^16
          tmp /= 1000;
          vitesse -= tmp;
        }
        else if (vitesse != 0) vitesse = 0;
        break;
      case SPEED_UP:
        if (vitesse < 3500) {
          // U(n+1) = U(n) + 0.007*[U(n)+1500]
          unsigned int tmp;
          tmp = (vitesse + 1500);
          tmp *= 7; // 7*(3500+1500) = 35000 < 2^16
          tmp /= 1000;
          vitesse += tmp;
        }
        else if (vitesse < 8700) {
          // U(n+1) = U(n) + 0.008*[9015-U(n)]
          unsigned int tmp;
          tmp = (9015 - vitesse);
          tmp *= 8; // 8*(9015-3500) = 44120 < 2^16
          tmp /= 1000;
          vitesse += tmp;
        }
        else if (vitesse != 8700) vitesse = 8700;
        break;
      case FINAL_88MPH:
        // slow speed up in case other devices are shutdown
        if (vitesse < 8800) vitesse+=3;
        // 88mph otherwise
        else if (vitesse != 8800) vitesse = 8800;
        break;
      case SLOW_DOWN:
        if (phase == 1) {
          // U(n+1) = U(n) - 0.05*[U(n)-4400]
          unsigned int tmp;
          tmp = vitesse - 4400;
          tmp *= 5; // 5*(8800-4400) = 22000 < 2^16
          tmp /= 100;
          vitesse -= tmp;
          if (vitesse < 4920) phase++;
        }
        if (phase == 2) {
          // U(n+1) = U(n) + 20
          vitesse += 20;
          if (vitesse > 5500) phase++;
        }
        if (phase == 3) {
          // U(n+1) = U(n) - 0.08*[U(n)+1000]
          unsigned int tmp;
          tmp = vitesse + 1000;
          tmp *= 8; // 8*(5520+1000) = 52160 < 2^16
          tmp /= 100;
          vitesse -= tmp;
          if (vitesse < 100) phase++;
        }
        if (phase > 3) phase++;
        if (phase > 40) loopFlag = true;        
        break;
      default:
        //error state
        vitesse = 9900;
        break;
    }
  }

  displaySpeed(vitesse);

}


/* 
 *  Display functions
 */

void displaySpeed(unsigned int vitesse) {
  vitesse /= 100;
  if (vitesse > 99) vitesse = 99;
  displayDigit1(vitesse / 10); // display tens
  displayDigit2(vitesse % 10); // display units
#ifdef DOT_DISPLAY
  blinkSegment(14);
#endif
}

void displayDigit1(byte digit1) {
  for (byte i = 0; i < 7; i++) {
    if (digit1 > 0) { // zero not displayed on tens
      if (digit[digit1] & (0b1000000 >> i)) blinkSegment(i);
      else delayMicroseconds(SWEEP_TIME); // preserve display frequency
    }
    else delayMicroseconds(SWEEP_TIME);
  }
}

void displayDigit2(byte digit2) {
  for (byte i = 0; i < 7; i++) {
    if (digit[digit2] & (0b1000000 >> i)) blinkSegment(i + 7);
    else delayMicroseconds(SWEEP_TIME); // preserve display frequency
  }
}

void blinkSegment(byte i) {
  pinMode(anodePins[i], OUTPUT);
  pinMode(cathodePins[i], OUTPUT);
  digitalWrite(anodePins[i], HIGH);
  digitalWrite(cathodePins[i], LOW);
  delayMicroseconds(SWEEP_TIME);
  pinMode(anodePins[i], INPUT);
  pinMode(cathodePins[i], INPUT);
}
