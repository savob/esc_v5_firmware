#include "led.h"

const byte LEDpin = PIN_PB6; // LED pin number, primarily used for testing

unsigned long nonBlockToggleTime = 0;
unsigned int nonBlockTogglePeriod = 0;
unsigned int nonBlockTogglesLeft = 0;

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


void nonBlockingLEDBlink() {

  // Check if we are even blinking
  if (nonBlockTogglesLeft > 0) {

    // See if we have passed a point to blink
    if (nonBlockToggleTime < millis()) {

      LEDToggle();
      nonBlockToggleTime = millis() + nonBlockTogglePeriod;
      nonBlockTogglesLeft--;

      if (nonBlockTogglesLeft == 0) LEDOn(); // Turn on LED at finish
    }
  }
}

void setNonBlockingBlink (unsigned int period, unsigned int count) {
  LEDOff(); // Start with LED off

  nonBlockTogglesLeft = (count * 2) - 1; // Two toggles per count, exclude staring one
  nonBlockTogglePeriod = period;

  nonBlockToggleTime = millis() + period; // Record next toggle time
}