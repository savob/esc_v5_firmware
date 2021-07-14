#include "motor.h"

volatile byte sequenceStep = 0; // Stores step in spinning sequence

volatile bool motorUpslope = true; // Used for PWM
volatile bool test2 = false;

// PWM variables
volatile byte period = 249;           // MUST be less than 254
volatile byte duty = 100;
volatile byte nDuty = 149;  // Used to store the "inverse" duty, to reduce repeated operations withinm the interupt to calculate this
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
  TCA0.SPLIT.LPER = 0xFF; // Count all the way down from 255 on all timers
  TCA0.SPLIT.HPER = 0xFF; 
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

// Motor PWM interupts
// Used to create a software version of phase correct PWM
ISR(TIMER2_COMPA_vect) {
  // Hit the half period mark

  motorUpslope = !(motorUpslope); // Invert "direction"

  // Set duty according to if we ar on the "upslope" or not
  if (motorUpslope) {
    OCR2B = nDuty;
  }
  else {
    OCR2B = duty;
    PORTB = motorPortSteps[sequenceStep]; // Set motor stage, used primarily for 100% duty cycle
  }
}

ISR(TIMER2_COMPB_vect) {
  // Toggle output
  // On when ticking up, off when going down

  if (motorUpslope) PORTB = motorPortSteps[sequenceStep];
  else PORTB = 0;
}

void setPWMDuty(byte deisredDuty) { // Set the duty of the motor PWM
  // Checks if input is at either extreme, clamps accordingly
  if (deisredDuty < endClampThreshold) {
    // Low extreme, disable motor
    duty = 0;
    disableMotor();
  }
  else if (deisredDuty >= (period - endClampThreshold)) {
    // High extreme, clamp to on
    duty = period;

    // Disable PWM duty interrupt
    TIMSK2 = 0x02;
    // COMPA is used to update outputs to current motor step
  }
  else {
    // Normal range
    duty = deisredDuty;
    nDuty = period - duty; // Set downwards cycle as well

    // Enable PWM interrupts
    TIMSK2 = 0x06;  // Interrupt for COMP A and B matches
  }
}

bool enableMotor(byte startDuty) { // Enable motor with specified starting duty, returns false if duty is too low

  // Return false if duty too low, keep motor disabled
  if (startDuty < endClampThreshold) {
    disableMotor();
    return (false);
  }

  // Configure half-bridge driver pins
  DDRB  |= 0x3F;  // Set pins to be driven
  PORTB &= 0xC0;  // Reset outputs

  if (motorStatus == false) windUpMotor(); // Wind up motor if not already spinning

  //Timer 2 setup for software "phase correct" PWM
  // CTC, 8x prescaling, no output on pins
  TCCR2A = 0x02;
  TCCR2B = 0x02;
  TIMSK2 = 0x06;  // Interrupt for COMP A and B matches

  OCR2A = period; // Set PWM parameters
  OCR2B = duty;
  TCNT2 = 0;      // Reset counter

  ///////////////////////////
  // BEMF Configuration
  // Enable analog comparator interrupt flag, start by looking for falling edge
  ACSR = 0x08;

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

  // Disable PWM timer
  TCA0.SPLIT.CTRLA = 0;
  TCA0.SPLIT.CTRLB = 0; // No control over output

  // Set all outputs to low
  PORTC.OUTCLR = PIN3_bm | PIN4_bm | PIN5_bm;
  PORTB.OUTCLR = PIN0_bm | PIN1_bm | PIN5_bm;

  // Disable Analog Comparator (BEMF)
  AC1.CTRLA = 0; 
  AC1.INTCTRL = 0; // Disable analog comparator interrupt

  duty = 0;
  motorStatus = false;
}


// The ISR vector of the Analog comparator
ISR (ANALOG_COMP_vect) {

  // We hold until the comparator output matches 10 times to be sure
  // Use ACO as our check bit for comparison value
  //    for (byte i = 0; i < 10; i++) {
  //
  //      // Check config register for if we're looking for a rising edge
  //      if ((ACSR & 0x03) == 0x03) {
  //        // If we're looking a rising edge, wait until a steady low is found
  //        if (!(ACSR & B00100000)) i = 0; // ACO = 0 (Analog Comparator Output = 0)
  //      }
  //      else {
  //        //If looking for falling
  //        if ((ACSR & B00100000))  i = 0; // ACO = 1 (Analog Comparator Output = 1)
  //      }
  //    }

  OCR1A = 2 * TCNT1;
}

ISR(TIMER1_COMPA_vect) {
  // Set next step
  sequenceStep++;                    // Increment step by 1, next part in the sequence of 6
  sequenceStep %= 6;                 // If step > 5 (equal to 6) then step = 0 and start over

  PORTB = motorPortSteps[sequenceStep]; // Set motor
  
  switch (sequenceStep) {
    case 0:
      BEMF_C_FALLING();
      //Serial.println("AH BL CF");
      break;
    case 1:
      BEMF_B_RISING();
      //Serial.println("AH CL BR");
      break;
    case 2:
      BEMF_A_FALLING();
      //Serial.println("BH CL AF");
      break;
    case 3:
      BEMF_C_RISING();
      //Serial.println("BH AL CR");
      break;
    case 4:
      BEMF_B_FALLING();
      //Serial.println("CH AL BF");
      break;
    case 5:
      BEMF_A_RISING();
      //Serial.println("CH BL AR");
      break;
  }
  
  OCR1A = 65535; // Set to max
}


// Buzzer function.
// Buzzes with a period specified in microseconds for a duration of certain milliseconds
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
void AH_BL() {
  PORTB = B00100100;
}
void AH_CL() {
  PORTB = B00100001;
}
void BH_CL() {
  PORTB = B00001001;
}
void BH_AL() {
  PORTB = B00011000;
}
void CH_AL() {
  PORTB = B00010010;
}
void CH_BL() {
  PORTB = B00000110;
}

/* Since we are comparing to relative the positive terminal, our edges have to be reversed

  E.g. If A is rising it will go from being less than the zero to more.
  Since A is connected to the negative terminal, the comparator output will go from 1 to 0

  So although we are looking for a rising edge, our microcontroller will see a falling one

  NOTE THAT ALL THESE WILL ENABLE THE COMPARATOR INTERRUPT!
*/
void BEMF_A_RISING() {
  ADMUX = 0;                // Select A0 as comparator negative input
  ACSR  = 0x0A;             // Set interrupt on falling edge
}
void BEMF_A_FALLING() {
  ADMUX = 0;                // Select A0 as comparator negative input
  ACSR  = 0x0B;             // Set interrupt on rising edge
}
void BEMF_B_RISING() {
  ADMUX = 1;                // Select A1 as comparator negative input
  ACSR  = 0x0A;             // Set interrupt on falling edge
}
void BEMF_B_FALLING() {
  ADMUX = 1;                // Select A1 as comparator negative input
  ACSR  = 0x0B;             // Set interrupt on rising edge
}
void BEMF_C_RISING() {
  ADMUX = 2;                // Select A2 as comparator negative input
  ACSR  = 0x0A;             // Set interrupt on falling edge
}
void BEMF_C_FALLING() {
  ADMUX = 2;                // Select A2 as comparator negative input
  ACSR  = 0x0B;             // Set interrupt on rising edge
}