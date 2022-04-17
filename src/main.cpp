#include <Arduino.h>
#include <avr/interrupt.h>
#include <i2c.h>
#include <motor.h>
#include <uartcomms.h>
#include <led.h>

void setup() {
  LEDSetup(); // Set up LED first to indicate it is powered

#ifdef ALLOW_UART_COMMS
  uartSetup(); // Needs to go first to allow potential printing of debugging statements
#endif

  setupMotor();
  i2cSetup(); 
  
  // Alert user set up is complete
  LEDBlinkBlocking(500, 4); // Lights before buzing to get code on before motor goes
  buzz(500, 1000);
  LEDBlinkBlocking(500, 2);
  
  // Turn off LED for actual enabling process
  LEDOff();
  //enableMotor(100);
  //delay(10000);
  //disableMotor();

  LEDOn();
}

void loop() {

#ifdef ALLOW_UART_COMMS
  delay(10); // Let messages arrive
  if (Serial.available()) uartCommands(); // Check for commands over UART
#endif

  nonBlockingLEDBlink();
  runInterruptBuzz();
}
