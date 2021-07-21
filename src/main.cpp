#include <Arduino.h>
#include <avr/interrupt.h>
#include <i2c.h>
#include <motor.h>

void setup() {

  setupMotor();
  i2cSetup(); 

}

void loop() {

  disableMotor();
}