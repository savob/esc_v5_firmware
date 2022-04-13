#include <Arduino.h>
#include <avr/interrupt.h>
#include <i2c.h>
#include <motor.h>
#include <uartcomms.h>
#include <led.h>

void setup() {
  LEDSetup(); // Set up LED first to indicate it is powered
  setupMotor();
  i2cSetup(); 

#ifdef ALLOW_UART_COMMS
  uartSetup(); // Needs to go first to potential printing of debugging statements
#endif
  
  // Alert user set up is complete
  LEDBlinkBlocking(500, 4); // Lights before buzing to get code on before motor goes
  //buzz(500, 1000);
  //LEDBlinkBlocking(500, 2);
  
  //enableMotor(maxDuty/2);
  
  /*
  //allFloat();
  delay(100);

  for (int i = 0; i < 6; i++) {
    sequenceStep = i % 6;
    motorSteps[sequenceStep]();
    delayMicroseconds(800);
  }
  disableMotor();
  */
  
  //delay(20000);
 
  PORTA.DIRSET = PIN3_bm; // PWM IN pin
  PORTB.DIRSET = PIN3_bm; // RX for AC
  PORTB.DIRSET = PIN2_bm; // TX for general
  PORTA.OUTSET = PIN3_bm; // PWM IN pin used for output
  PORTB.OUTSET = PIN2_bm; // TX pin used for output

  // Set to alternating on A 
  bemfSteps[0] = aRisingBEMF;
  bemfSteps[1] = aFallingBEMF;
  bemfSteps[2] = aRisingBEMF;
  bemfSteps[3] = aFallingBEMF;
  bemfSteps[4] = aRisingBEMF;
  bemfSteps[5] = aFallingBEMF;
  aRisingBEMF();
  

  AC1.CTRLA = AC_ENABLE_bm | AC_HYSMODE_50mV_gc | AC_OUTEN_bm; // Enable the AC with hysteresis, output on RX

  TCB0.INTCTRL = TCB_CAPT_bm; // Enable timer interrupt
  TCB1.INTCTRL = TCB_CAPT_bm; // Enable timer interrupt
  
  // Output Event Channel state
  //PORTMUX.CTRLA = PORTMUX_EVOUT1_bm; 
  //EVSYS.ASYNCUSER9 = EVSYS_ASYNCUSER9_ASYNCCH0_gc; // Output Channel 0 on TX

  TCB1.CCMP = 10000;
}

void loop() {
  //LEDOn(); // LED is usually on when operational

  //disableMotor();

#ifdef ALLOW_UART_COMMS
  if (Serial.available()) uartCommands(); // Check for commands over UART
#endif
}
