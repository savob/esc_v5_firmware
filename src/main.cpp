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
  buzz(500, 1000);
  LEDBlinkBlocking(500, 2);
  
  // Turn off LED for actual enabling process
  LEDOff();
  enableMotor(maxDuty);
}

void loop() {
  LEDOn(); // LED is usually on when operational

#ifdef ALLOW_UART_COMMS
  if (Serial.available()) uartCommands(); // Check for commands over UART
#endif
}
