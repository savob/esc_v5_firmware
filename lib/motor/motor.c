#include "motor.h"

volatile byte sequenceStep = 0; // Stores step in spinning sequence

volatile bool motorUpslope = true; // Used for PWM
volatile bool test2 = false;

// PWM variables
volatile byte maxDuty = 249;           // MUST be less than 254
volatile byte duty = 100;
byte endClampThreshold = 15;           // Stores how close you need to be to either extreme before the PWM duty is clamped to that extreme

// AH_BL, AH_ CL, BH_CL, BH_AL, CH_AL, CH_BL
volatile byte motorPortSteps[] = {B00100100, B00100001, B00001001, B00011000, B00010010, B00000110};

// Other variables
volatile byte cyclesPerRotation = 2;
volatile byte cycleCount = 0;

volatile unsigned int currentRPM = 1000;
volatile unsigned int targetRPM = 0;
volatile byte controlScheme = 0;

bool reverse = false;
volatile bool motorStatus = false; // Stores if the motor is disabled (false) or not

const unsigned int spinUpStartPeriod = 5000;
const unsigned int spinUpEndPeriod = 500;
const byte stepsPerIncrement = 6;
const unsigned int spinUpPeriodDecrement = 10;

// Buzzer period limits
const unsigned int maxBuzzPeriod = 2000;
const unsigned int minBuzzPeriod = 200;

void setupMotor() {
  //==============================================
  // Read in if we're reversing the motor (pin PA4 is shorted)
  PORTA.DIRCLR = PIN4_bm;
  PORTA.PIN4CTRL = PORT_PULLUPEN_bm; // Set it have a pull up
  delayMicroseconds(100); // Allow outputs to settle before reading
  reverse = ((PORTA.IN & PIN4_bm) == 0);

  //==============================================
  // Set up output pins
  PORTMUX.CTRLC = PORTMUX_TCA05_bm | PORTMUX_TCA04_bm | PORTMUX_TCA03_bm | PORTMUX_TCA02_bm; // Multiplexed outputs
  PORTC.DIRSET = PIN3_bm | PIN4_bm | PIN5_bm; // Set pins to be outputs
  PORTB.DIRSET = PIN0_bm | PIN1_bm | PIN5_bm;
  
  //==============================================
  // Set up PWM
  TCA0.SPLIT.LPER = maxDuty; // Set upper duty limit
  TCA0.SPLIT.HPER = maxDuty; 
  TCA0.SPLIT.CTRLESET = TCA_SPLIT_CMD_RESTART_gc | 0x03; // Reset both timers
  TCA0.SPLIT.CTRLA = TCA_SPLIT_CLKSEL_DIV16_gc | TCA_SPLIT_ENABLE_bm; // Enable the timer with prescaler of 16

  //==============================================
  // Analog comparator setting
  AC1.INTCTRL = 0; // Disable analog comparator interrupt
  AC1.MUXCTRLA = AC_MUXNEG0_bm; // Set negative reference to be Zero pin (PA5)
  AC1.CTRLA = AC_ENABLE_bm | AC_HYSMODE_25mV_gc; // Enable the AC with a 25mV hysteresis

  disableMotor(); // Ensure motor is disabled at start
}

void windUpMotor() {
  // Forces the motor to spin up to a decent speed before running motor normally.

  if (motorStatus == true) return; // Not to be run when already spun up

  int period = spinUpStartPeriod;

  while (period > spinUpEndPeriod) {

    for (byte i = 0; i < stepsPerIncrement; i++) {
      PORTB = motorPortSteps[sequenceStep];
      delayMicroseconds(period);

      sequenceStep++;
      sequenceStep %= 6;
    }
    period -= spinUpPeriodDecrement;
  }
}


void setPWMDuty(byte deisredDuty) { // Set the duty of the motor PWM
  
  // Checks if input is too low and disables if needed
  if (deisredDuty < endClampThreshold) {
    disableMotor();
    return;
  }

  // Check and clamp if near the high extreme
  if (deisredDuty >= (maxDuty - endClampThreshold)) duty = maxDuty; 
  else duty = deisredDuty;

  // Assign duty to all outputs
  TCA0.SPLIT.LCMP0 = duty;
  TCA0.SPLIT.LCMP1 = duty;
  TCA0.SPLIT.LCMP2 = duty;
  TCA0.SPLIT.HCMP0 = duty;
  TCA0.SPLIT.HCMP1 = duty;
  TCA0.SPLIT.HCMP2 = duty;
  TCA0.SPLIT.CTRLESET = TCA_SPLIT_CMD_RESTART_gc | 0x03; // Reset both timers to syncronize them
}

bool enableMotor(byte startDuty) { // Enable motor with specified starting duty, returns false if duty is too low

  // Return false if duty too low, keep motor disabled
  if (startDuty < endClampThreshold) {
    disableMotor();
    return (false);
  }

  if (motorStatus == false) windUpMotor(); // Wind up motor if not already spinning

  // Enable PWM timers
  TCA0.SPLIT.CTRLESET = TCA_SPLIT_CMD_RESTART_gc | 0x03; // Reset both timers
  TCA0.SPLIT.CTRLA = TCA_SPLIT_CLKSEL_DIV16_gc | TCA_SPLIT_ENABLE_bm; // Enable the timer with prescaler of 16

  // Enable analog comparator
  AC1.CTRLA = AC_ENABLE_bm | AC_HYSMODE_25mV_gc; // Enable the AC with a 25mV hysteresis

  /* Timer 1 setup for phase changes
    Since the zero crossing event happens halfway through the step I will set timer 1 to zero at
    the start of each phase. Once the zero crossing occurs I will read the current time value for
    timer 1, double it and set the top to that. Once it triggers at the top it'll step to the next
    motor setting and reset it's own counter. It will also set it's top to the max to avoid early
    triggers on the next step before the zero crossing event.

    Setting is CTC, 8x prescaler
  */
  TCCR1A = 0x00;
  TCCR1B = 0x0A;
  OCR1A = 65535;
  TIMSK1 = 0x02;
  TCNT1 = 0;      // Reset counter


  motorStatus = true;     // Needs to be set first, so it can be returned to 0 if the duty is too small to enable it in setPWMmotor().
  setPWMDuty(startDuty);  // Set duty for motor

  return (true);
}
void disableMotor() {

  allFloat(); // Set all outputs to float

  // Disable Analog Comparator (BEMF)
  AC1.CTRLA = 0; 

  duty = 0;
  motorStatus = false;
}

ISR(TIMER1_COMPA_vect) {
  // Set next step
  sequenceStep++;                    // Increment step by 1, next part in the sequence of 6
  sequenceStep %= 6;                 // If step > 5 (equal to 6) then step = 0 and start over

  PORTB = motorPortSteps[sequenceStep]; // Set motor
  
  switch (sequenceStep) {
    case 0:
      cFallingBEMF();
      //Serial.println("AH BL CF");
      break;
    case 1:
      bRisingBEMF();
      //Serial.println("AH CL BR");
      break;
    case 2:
      aFallingBEMF();
      //Serial.println("BH CL AF");
      break;
    case 3:
      cRisingBEMF();
      //Serial.println("BH AL CR");
      break;
    case 4:
      bFallingBEMF();
      //Serial.println("CH AL BF");
      break;
    case 5:
      aRisingBEMF();
      //Serial.println("CH BL AR");
      break;
  }
  
  OCR1A = 65535; // Set to max
}


// Buzzer function.
void buzz(int periodMicros, int durationMillis) { // Buzz with a period of
  if (motorStatus) return; // Will not run if motor is enabled and spinning.
  // It would [greatly] suck if the drone "buzzed" mid-flight

  // Ensure period is in allowed tolerances
  periodMicros = constrain(periodMicros, minBuzzPeriod, maxBuzzPeriod);

  // Buzz works by powering one motor phase for 50 us then holding off
  // for the remainder of half the period, then fires another phase for
  // 50 us before holding for the rest of the period

  int holdOff = (periodMicros / 2) - 50;                  // Gets this holdoff period
  unsigned long endOfBuzzing = millis() + durationMillis; // Marks endpoint

  // Set up motor port (we don't need the PWM stuff)
  PORTB &= 0xC0;  // Reset outputs
  DDRB  |= 0x3F;  // Set pins to be driven

  // Buzz for the duration
  while (millis() < endOfBuzzing) {
    PORTB = motorPortSteps[0];
    delayMicroseconds(50);
    PORTB = 0;
    delayMicroseconds(holdOff);

    PORTB = motorPortSteps[1];
    delayMicroseconds(50);
    PORTB = 0;
    delayMicroseconds(holdOff);
  }

  disableMotor(); // Redisable the motor entirely
}


// Update these to set PWM on high side once reworked
void AHBL() {
  PORTB = B00100100;
}
void AHCL() {
  PORTB = B00100001;
}
void BHCL() {
  PORTB = B00001001;
}
void BHAL() {
  PORTB = B00011000;
}
void CHAL() {
  PORTB = B00010010;
}
void CHBL() {
  PORTB = B00000110;
}
void allFloat() {
  // Disable PWM timer
  TCA0.SPLIT.CTRLA = 0;
  TCA0.SPLIT.CTRLB = 0; // No control over output

  // Set all outputs to low
  PORTC.OUTCLR = PIN3_bm | PIN4_bm | PIN5_bm;
  PORTB.OUTCLR = PIN0_bm | PIN1_bm | PIN5_bm;
}

// Comparator functions

void aRisingBEMF() {
  AC1.MUXCTRLA = 0x08;
}
void aFallingBEMF() {
  AC1.MUXCTRLA = 0x08;
}
void bRisingBEMF() {
  AC1.MUXCTRLA = 0x00;
}
void bFallingBEMF() {
  AC1.MUXCTRLA = 0x00;
}
void cRisingBEMF() {
  AC1.MUXCTRLA = 0x10;
}
void cFallingBEMF() {
  AC1.MUXCTRLA = 0x10;
}