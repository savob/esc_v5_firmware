#include "pwmin.h"
#include "motor.h"

// Most PWM variables are locally scoped

const byte PWMInPinMask = PIN3_bm;

volatile unsigned long PWMTimeOutMark = 0; // Records when to alert control timeout with millis()
const unsigned int PWMTimeOutPeriod = 1000; // Tolerated timeout for PWM waves in ms

const unsigned int PWMPeriodMax = 2000; // Maximum expected PWM period
const unsigned int PWMPeriodMin = 1000; // Minimum expected PWM period


void pwmInputSetup() {
  PORTA.DIRCLR = PWMInPinMask; // Set to input
  PORTA.PIN3CTRL = PORT_ISC_BOTHEDGES_gc; // Set interrupt on both edges
}

// All interrupts on a port are handled by one interrupt
ISR(PORTA_PORT_vect) {

  static unsigned long lastPWMRise = 0;
  static unsigned long lastPWMFall = 0;
  static unsigned int lastPWMDutyPeriod = 0;

  unsigned long currentMicros = micros(); // Record micros at start
  unsigned int temp = 0;

  // Check which edge occured based on current input state of pin
  if ((PORTA.IN & PWMInPinMask) == 0) {
    // Falling edge just occured

    lastPWMDutyPeriod = currentMicros - lastPWMRise;
    lastPWMFall = currentMicros;

    // Use this period to control the motor
    temp = constrain(lastPWMDutyPeriod, PWMPeriodMin, PWMPeriodMax);
    temp = map(temp, PWMPeriodMin, PWMPeriodMax, 0, maxDuty);

    setPWMDuty(temp);

  }
  else {
    // Rising edge just happened
    lastPWMRise = currentMicros;

    PWMTimeOutMark = millis() + PWMTimeOutPeriod; // Record next timeout threshold
  }

  
  PORTA.INTFLAGS = PWMInPinMask; // Clear interrupt at completion
}

bool checkPWMTimeOut() {
  bool timedOut = false;

  // Check for time out
  if (millis() > PWMTimeOutMark) {
    
    // Check it is not 0, since we may not have recieved first signal yet
    if (PWMTimeOutMark != 0) {
      timedOut = true;
    }
  }

  return timedOut;
}

/* TODO: Add code to calibrate it to a device's period if it isn't perfectly between 1 and 2 ms
      - Will definitely need to use EEPROM to store these values between boots
*/