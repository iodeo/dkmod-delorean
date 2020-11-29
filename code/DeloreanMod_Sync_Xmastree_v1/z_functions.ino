
byte getColumnHeight(byte mode, unsigned int frame, byte j) {
  switch (mode) {
    case REST_MODE:
      if (frame > REST_NB_FRAME) frame = REST_NB_FRAME - 1;
      return pgm_read_byte(&restSequence[frame][j]);
      break;
    case SPEEDUP_MODE:
      if (frame > SPEEDUP_NB_FRAME) frame = SPEEDUP_NB_FRAME - 1;
      return pgm_read_byte(&speedupSequence[frame][j]);
      break;
    case FINAL_MODE:
      if (frame > FINAL_NB_FRAME) frame = FINAL_NB_FRAME - 1;
      return pgm_read_byte(&finalSequence[frame][j]);
      break;
    default: // OFF_MODE
      return 0;
      break;
  }
}

unsigned long getTiming(byte mode, unsigned int frame) {
  switch (mode) {
    case REST_MODE:
      if (frame > REST_NB_FRAME) frame = REST_NB_FRAME - 1;
      return (unsigned long) pgm_read_word(&restTiming[frame]); 
      //use pgm_read_dword instead if progmem used unsigned long
      break;
    case SPEEDUP_MODE:
      return (unsigned long) pgm_read_word(&speedupTiming[frame]);
      break;
    case FINAL_MODE:
      if (frame > FINAL_NB_FRAME) frame = FINAL_NB_FRAME - 1;
      return (unsigned long) pgm_read_word(&finalTiming[frame]);
      break;
    default: // OFF_MODE
      return -1;
      break;
  }
}
