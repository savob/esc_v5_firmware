#include <Arduino.h>
#include <avr/interrupt.h>
#include <i2c.h>
#include <motor.h>
#include <uartcomms.h>
#include <led.h>

void setup() {
  LEDSetup(); // Set up LED first to indicate it is powered

#ifdef ALLOW_UART_COMMS
  uartSetup(); // Needs to go first to potential printing of debugging statements
#endif
  
  setupMotor();
  i2cSetup(); 

  LEDBlinkBlocking(1000, 3); // Alert that it is starting soon
}

void loop() {

  disableMotor();

#ifdef ALLOW_UART_COMMS
  if (Serial.available()) uartCommands(); // Check for commands over UART
#endif
}