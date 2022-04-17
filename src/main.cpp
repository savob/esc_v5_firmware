#include <Arduino.h>
#include <avr/interrupt.h>
#include <i2c.h>
#include <motor.h>
#include <uartcomms.h>
#include <led.h>
#include <pwmin.h>

#define USE_PWM_CONTROL // Use PWM input for controlling speed

void emergencyStop(); // Declared here to be used in loop()



void setup() {
  LEDSetup(); // Set up LED first to indicate it is powered

#ifdef ALLOW_UART_COMMS
  uartSetup(); // Needs to go first to allow potential printing of debugging statements
#endif

  setupMotor();
  i2cSetup();

#ifdef USE_PWM_CONTROL
  pwmInputSetup();
#endif
  
  // Alert user set up is complete
  LEDBlinkBlocking(250, 20); // Lights before buzing to get code on before motor goes

  LEDOn();
}

void loop() {

#ifdef ALLOW_UART_COMMS
  delay(10); // Let messages arrive
  if (Serial.available()) uartCommands(); // Check for commands over UART
#endif

  nonBlockingLEDBlink();
  runInterruptBuzz();

#ifdef USE_PWM_CONTROL
  if (checkPWMTimeOut() == true) {
    // If timed out 
    emergencyStop();
  }
  else {
    // Try to wind up if not timed out but motor is disabled
    if (motorStatus == false) enableMotor(duty);
  }
#endif
}

void emergencyStop() {
  disableMotor();
  cli(); // Disable global interrupts to prevent anything trying to raise motor

#ifdef UART_COMMS_DEBUG
  Serial.println("EMERGENCY STOPPED");
#endif

  while (1) {
    // Freeze
  }
}