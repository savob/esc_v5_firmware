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

void LEDSetTo(bool setTo) {
  if (setTo == true) {
    LEDOn();
  }
  else {
    LEDOff();
  }

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

  LEDOn(); // Leaves LED on
  return;
}

// TODO: Add a non-blocking LED function, likely using TCB1
// Problem is that currently CLK_PER is same as CPU (20 MHz) so even if prescaled by 2, it will overflow every 6.55ms.
// Maybe set it overflow ever 5ms and just count as needed or check millis()?