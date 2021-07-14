#include <Arduino.h>
#include <avr/interrupt.h>
#include <i2c.h>
#include <motor.h>

void setup() {
  // Input / config pins
  DDRD  &= 0x03;  // Configure all input pins as inputs
  PORTD &= 0x03;  // Clear all input pullups
  PORTD |= 0xB8;  // Pull ups for all input solder pads (soldered ones short to GND)

  delay(1); // Allow outputs to settle before reading

  // Sets reverse to true if reverse pin is shorted
  reverse = ((PIND & 0x80) == 0);

  i2cSetup(); // Set up I2C inteface

  // Disable pull-up resistors
  MCUCR |= (1 << PUD);
  // We don't pull-ups at any other point, so it's best
  // disabled pins stay hi-Z regardless of PORTx values

  // Analog comparator setting
  ADCSRA = (0 << ADEN);     // Disable the ADC module
  ADCSRB = (1 << ACME);     // Enable MUX select for negative input of comparator
  ACSR   = (0 << ACBG);     // Select PD6 as positive side for comparison

  disableMotor(); // Ensure motor is disabled at start

}



void loop() {

  if (test2) { // Debugging printer
    test2 = false;
    //Serial.println(sequenceStep);

    delay(5);
  }

  disableMotor();
}