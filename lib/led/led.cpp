#include "led.h"

const byte LEDpin = PIN_PB6; // LED pin number, primarily used for testing

void LEDSetup() {
  PORTB.DIRSET = PIN6_bm;
  
  LEDBlinkBlocking(200, 5); // Blink to show it is set up
  return;
}

void LEDOn() {
  PORTB.OUTSET = PIN6_bm;

  return;
}

void LEDOff() {
  PORTB.OUTCLR = PIN6_bm;

  return;
}

void LEDToggle() {
  PORTB.OUTTGL = PIN6_bm;

  return;
}

void LEDBlinkBlocking(int period, int count) {
  // Since we're toggling the LED we need the half periods
  period = period / 2;
  count = count *2;

  LEDOff(); // Start off
  for (int i = 0; i < count; i++) {
    LEDToggle();
    delay(period);
  }

  return;
}