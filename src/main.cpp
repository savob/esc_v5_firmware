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
  //LEDBlinkBlocking(500, 6);
  //buzz(500, 2500);

  // Enable PWM timers
  TCA0.SPLIT.CTRLESET = TCA_SPLIT_CMD_RESTART_gc | 0x03; // Reset both timers
  TCA0.SPLIT.CTRLA = TCA_SPLIT_CLKSEL_DIV16_gc | TCA_SPLIT_ENABLE_bm; // Enable the timer with prescaler of 16

  AC1.CTRLA = AC_ENABLE_bm | AC_HYSMODE_50mV_gc | AC_OUTEN_bm; // Enable the AC with a 25mV hysteresis, output on RX
  TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;
  TCB0.CTRLB = TCB_CNTMODE_FRQ_gc; 
  TCB0.INTCTRL = TCB_CAPT_bm; // Enable timer interrupt

  // Link TCB0 to analog comparator's output
  TCB0.EVCTRL = TCB_CAPTEI_bm | TCB_EDGE_bm | TCB_FILTER_bm; // Enable event capture input (AC), on falling edge
  EVSYS.ASYNCCH0 = EVSYS_ASYNCCH0_AC1_OUT_gc; // Use comparator as async channel 0 source
  EVSYS.ASYNCUSER0 = EVSYS_ASYNCUSER0_ASYNCCH0_gc; // Use async channel 0 as input for TCB0

  PORTA.DIRSET = PIN3_bm; // PWM IN pin
  PORTB.DIRSET = PIN3_bm; // RX for AC
  //PORTA.OUTSET = PIN3_bm; // PWM IN pin used for output

  CPUINT.LVL1VEC = TCB0_INT_vect_num; // Elevates the communtation interrupt to be prioritized

  // Set to alternating on A 
  bemfSteps[0] = aRisingBEMF;
  bemfSteps[1] = aFallingBEMF;
  bemfSteps[2] = aRisingBEMF;
  bemfSteps[3] = aFallingBEMF;
  bemfSteps[4] = aRisingBEMF;
  bemfSteps[5] = aFallingBEMF;
  aRisingBEMF();

  TCB1.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm; // Needs to match TCB0!
  TCB1.CTRLB = TCB_CNTMODE_SINGLE_gc;
  EVSYS.ASYNCUSER11 = EVSYS_ASYNCUSER11_ASYNCCH0_gc; // Use ADC as input for TCB1
  TCB1.EVCTRL = TCB_CAPTEI_bm | TCB_EDGE_bm | TCB_FILTER_bm;

  TCB1.INTCTRL = TCB_CAPT_bm; // Enable timer interrupt

  TCB1.CCMP = 1000;
}

void loop() {
  LEDOn(); // LED is usually on when operational

  //disableMotor();

#ifdef ALLOW_UART_COMMS
  if (Serial.available()) uartCommands(); // Check for commands over UART
#endif
}
