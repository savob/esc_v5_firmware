#define ALLOW_UART_COMMS  // Are we enabling UART?
#define UART_COMMS_DEBUG  // Do we want debug statements over UART

#include <Arduino.h>
#include <avr/interrupt.h>
#include <i2c.h>
#include <motor.h>
#ifdef ALLOW_UART_COMMS
#include <uartcomms.h>
#endif

void setup() {

#ifdef ALLOW_UART_COMMS
  uartSetup(); // Needs to go first to potential printing of debugging statements
#endif
  setupMotor();
  i2cSetup(); 

}

void loop() {

  disableMotor();

#ifdef ALLOW_UART_COMMS
  if (Serial.available()) uartCommands(); // Check for commands over UART
#endif
}