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
  buzz(500, 1000);
  LEDBlinkBlocking(500, 6);
  
  enableMotor(maxDuty);
  delay(5000);
}

void loop() {
  LEDOn(); // LED is usually on when operational

  disableMotor();

#ifdef ALLOW_UART_COMMS
  if (Serial.available()) uartCommands(); // Check for commands over UART
#endif
}
