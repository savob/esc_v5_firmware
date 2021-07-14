#include <Arduino.h>
#include <avr/interrupt.h>
#include <i2c.h>
#include <motor.h>

void setup() {

  setupMotor();
  i2cSetup(); 

}

void loop() {

  if (test2) { // Debugging printer
    test2 = false;
    //Serial.println(sequenceStep);

    delay(5);
  }

  disableMotor();
}