#include "motor.h"

volatile byte sequenceStep = 0; // Stores step in spinning sequence
volatile voidFunctionPointer motorSteps[6]; // Stores the functions to copmmute in the current commutation order
volatile voidFunctionPointer bemfSteps[6]; // Stores the functions to set the BEMF in the current commutation order

// PWM variables
volatile byte maxDuty = 249;           // MUST be less than 254
volatile byte duty = 100;
byte endClampThreshold = 15;           // Stores how close you need to be to either extreme before the PWM duty is clamped to that extreme

// Other variables
volatile byte cyclesPerRotation = 2;
volatile byte cycleCount = 0;

// Control scheme
volatile ctrlSchemeEnum controlScheme = ctrlSchemeEnum::PWM;

// RPM variables
volatile unsigned int currentRPM = 1000;
volatile unsigned int targetRPM = 0;
volatile unsigned long lastRotationMicros = 0;

// Other motor configuration
bool reverse = false;
volatile bool motorStatus = false; // Stores if the motor is disabled (false) or not

// SPin up constants/variables
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

  // Setup the commutation steps based on direction
  if (reverse == false) {
    // AH_BL, AH_ CL, BH_CL, BH_AL, CH_AL, CH_BL
    motorSteps[0] = AHBL;
    motorSteps[1] = AHCL;
    motorSteps[2] = BHCL;
    motorSteps[3] = BHAL;
    motorSteps[4] = CHAL;
    motorSteps[5] = CHBL;

    bemfSteps[0] = cFallingBEMF;
    bemfSteps[1] = bRisingBEMF;
    bemfSteps[2] = aFallingBEMF;
    bemfSteps[3] = cRisingBEMF;
    bemfSteps[4] = bFallingBEMF;
    bemfSteps[5] = aRisingBEMF;
#ifdef UART_COMMS_DEBUG
  Serial.println("Motor is spinning normally.");
#endif
  }
  else {
    motorSteps[0] = AHBL;
    motorSteps[1] = CHBL;
    motorSteps[2] = CHAL;
    motorSteps[3] = BHAL;
    motorSteps[4] = BHCL;
    motorSteps[5] = AHCL;

    bemfSteps[0] = cRisingBEMF;
    bemfSteps[1] = aFallingBEMF;
    bemfSteps[2] = bRisingBEMF;
    bemfSteps[3] = cFallingBEMF;
    bemfSteps[4] = aRisingBEMF;
    bemfSteps[5] = bFallingBEMF;
#ifdef UART_COMMS_DEBUG
  Serial.println("Motor spinning set to be reversed");
#endif
  }

  //==============================================
  // Set up output pins
  PORTMUX.CTRLC = PORTMUX_TCA05_bm | PORTMUX_TCA04_bm | PORTMUX_TCA03_bm | PORTMUX_TCA02_bm; // Multiplexed outputs
  PORTC.DIRSET = PIN3_bm | PIN4_bm | PIN5_bm; // Set pins to be outputs
  PORTB.DIRSET = PIN0_bm | PIN1_bm | PIN5_bm;
  
  //==============================================
  // Set up PWM
  TCA0.SPLIT.CTRLA = TCA_SPLIT_CLKSEL_DIV16_gc | TCA_SPLIT_ENABLE_bm; // Enable the split timer with prescaler of 16
  TCA0.SPLIT.LPER = maxDuty; // Set upper duty limit
  TCA0.SPLIT.HPER = maxDuty; 
  TCA0.SPLIT.CTRLESET = TCA_SPLIT_CMD_RESTART_gc | 0x03; // Reset both timers
  
  //==============================================
  // Analog comparator setting
  AC1.INTCTRL = 0; // Disable analog comparator interrupt
  AC1.MUXCTRLA = AC_MUXNEG_PIN0_gc; // Set negative reference to be Zero pin (PA5)
  AC1.CTRLA = AC_ENABLE_bm | AC_HYSMODE_25mV_gc; // Enable the AC with a 25mV hysteresis

  CPUINT.LVL1VEC = TCB0_INT_vect_num; // Elevates the communtation interrupt to be prioritized

  disableMotor(); // Ensure motor is disabled at start

#ifdef UART_COMMS_DEBUG
  Serial.println("\n=======\nMotor setup complete\n=======\n");
#endif
}

void windUpMotor() {
  // Forces the motor to spin up to a decent speed before running motor normally.

  if (motorStatus == true) return; // Not to be run when already spun up

#ifdef UART_COMMS_DEBUG
  Serial.print("Motor spin-up starting...");
#endif

  int period = spinUpStartPeriod;

  while (period > spinUpEndPeriod) {

    for (byte i = 0; i < stepsPerIncrement; i++) {
      motorSteps[sequenceStep]();
      delayMicroseconds(period);

      sequenceStep++;
      sequenceStep %= 6;
    }
    period -= spinUpPeriodDecrement;
  }

#ifdef UART_COMMS_DEBUG
  Serial.println("DONE!");
#endif
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

#ifdef UART_COMMS_DEBUG
  Serial.print("ESC duty: ");
  Serial.println(duty);
#endif
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
  TCA0.SPLIT.CTRLA = TCA_SPLIT_CLKSEL_DIV1_gc | TCA_SPLIT_ENABLE_bm; // Enable the timer with prescaler of 16

  // Enable analog comparator
  AC1.CTRLA = AC_ENABLE_bm | AC_HYSMODE_25mV_gc; // Enable the AC with a 25mV hysteresis

  /* Timer B0 setup for phase changes
    
    Monitors the analog comparator to see when the zero crossing is, records it
    then switches to periodic mode to then trigger a commutation an equivalent
    period later.

    Uses TCA0's clock, and initially in Fequency mode to grab zero crossing.
  */
  TCB0.CTRLA = TCB_CLKSEL_CLKTCA_gc | TCB_ENABLE_bm;
  TCB0.CTRLB = TCB_CNTMODE_FRQ_gc;
  TCB0.INTCTRL = TCB_CAPT_bm; // Enable timer interrupt

  // Link TCB0 to analog comparator's output
  TCB0.EVCTRL = TCB_CAPTEI_bm | TCB_EDGE_bm; // Enable event capture input (AC), on falling edge
  EVSYS.ASYNCCH0 = EVSYS_ASYNCCH0_AC1_OUT_gc; // Use comparator as async channel 0 source
  EVSYS.ASYNCUSER0 = EVSYS_ASYNCUSER0_ASYNCCH0_gc; // Use async channel 0 as input for TCB0
  // TODO: set up the proper BEMF test here based on commutation step

  // Set output PWM
  motorStatus = true;     // Needs to be set first, so it can be returned to 0 if the duty is too small to enable it in setPWMmotor().
  setPWMDuty(startDuty);  // Set duty for motor

#ifdef UART_COMMS_DEBUG
  Serial.println("\n! MOTOR ENABLED !\n");
#endif

  return (true);
}
void disableMotor() {

  //allFloat(); // Coast to a stop
  allLow(); // Brake to a stop

  // Disable Analog Comparator (BEMF)
  AC1.CTRLA = 0; 

  duty = 0;
  motorStatus = false;

#ifdef UART_COMMS_DEBUG
  Serial.println("\n! MOTOR DISABLED !\n");
#endif
}

ISR(TCB0_INT_vect) {
  TCB0.INTFLAGS = 1; // Clear interrupt flag

  // See which mode the timer is in
  if (TCB0.CTRLB == TCB_CNTMODE_INT_gc) {
    // If we're in the periodic mode we need to commute the motor

    sequenceStep++;                    // Increment step by 1, next part in the sequence of 6
    sequenceStep %= 6;                 // If step > 5 (equal to 6) then step = 0 and start over

    motorSteps[sequenceStep]();
    bemfSteps[sequenceStep]();

    TCB0.CTRLB = TCB_CNTMODE_FRQ_gc;   // Set timer to catch zero crossing

    // Check where we are in completing a rotation to monitor RPM
    if (sequenceStep == 0) {
      cycleCount++;

      // Check if we completed a rotation
      if (cycleCount == cyclesPerRotation) {
        cycleCount = 0;

        currentRPM = 60000000 / (micros() - lastRotationMicros);
        lastRotationMicros = micros(); 
        // Not exactly ideal for accuracy to have this not recorded to something
        // before doing the math but I think any error is negligible overall
      }
    }

#ifdef UART_COMMS_DEBUG
    // Debug statements
    if (reverse == 0) {
      switch (sequenceStep) {
        case 0:
          Serial.println("AH BL CF");
          break;
        case 1:
          Serial.println("AH CL BR");
          break;
        case 2:
          Serial.println("BH CL AF");
          break;
        case 3:
          Serial.println("BH AL CR");
          break;
        case 4:
          Serial.println("CH AL BF");
          break;
        case 5:
          Serial.println("CH BL AR");
          break;
      }
    }
    else {
      switch (sequenceStep) {
        case 0:
          Serial.println("AH BL CR");
          break;
        case 1:
          Serial.println("CH BL AF");
          break;
        case 2:
          Serial.println("CH AL BR");
          break;
        case 3:
          Serial.println("BH AL CF");
          break;
        case 4:
          Serial.println("BH CL AR");
          break;
        case 5:
          Serial.println("AH CL BF");
          break;
      }
    }
#endif
  }
  else {
    // If we're in frequency capture mode (assumed since we're not periodic)

    TCB0.CTRLB = TCB_CNTMODE_INT_gc; // Set timer to commute in the equivalent time since the start of this phase
  }
}


// Buzzer function.
void buzz(int periodMicros, int durationMillis) { // Buzz with a period of
  if (motorStatus) return; // Will not run if motor is enabled and spinning.
  // It would [greatly] suck if the drone "buzzed" mid-flight

  // Ensure period is in allowed tolerances
  periodMicros = constrain(periodMicros, minBuzzPeriod, maxBuzzPeriod);

  /*
    Buzz works by powering one motor phase (AHBL) for 50 us then holding off
    for the remainder of half the period, then fires another phase (AHCL)
    for 50 us before holding for the rest of the period.
  */

  const unsigned int holdOn = 50;                         // Time the motor is pulled high (microseconds)
  int holdOff = (periodMicros / 2) - holdOn;              // Gets this holdoff period
  unsigned long endOfBuzzing = millis() + durationMillis; // Marks endpoint

  allLow(); // Ensure we start floating

  // Buzz for the duration
  while (millis() < endOfBuzzing) {
    PORTB.OUTSET = PIN5_bm;
    PORTC.OUTSET = PIN3_bm;
    delayMicroseconds(holdOn);
    allLow();
    delayMicroseconds(holdOff);

    PORTB.OUTSET = PIN5_bm;
    PORTC.OUTSET = PIN5_bm;
    delayMicroseconds(holdOn);
    allLow();
    delayMicroseconds(holdOff);
  }

  disableMotor(); // Redisable the motor entirely
}


/* NOTE: The high sides should never be set to HIGH.

  They should always be LOW or overriden by PWM so the state doesn't matter.
  I use this assumption to potentially simplify this code, by using OUTSET
  or OUTCLR to adjust the lows but leave the highs to be enabled by the PWM
  override when needed. This is only used when A needs to be driven low.

  This could prove to be an optimization in the future to put all the highs
  on one PORT (probably PORTB) and leave the lows on the other (PORTC) so
  they can be manipulated with simpler code. 
  
  This will need a hardware revision.
*/ 
void AHBL() {
  // Set up PWM pin(s) for high side
  TCA0.SPLIT.CTRLB = TCA_SPLIT_LCMP2EN_bm;

  // Set pin for low side and leave others cleared
  PORTB.OUTCLR = PIN0_bm | PIN1_bm;
  PORTC.OUT = PIN3_bm;
}
void AHCL() {
  // Set up PWM pin(s) for high side
  TCA0.SPLIT.CTRLB = TCA_SPLIT_LCMP2EN_bm;

  // Set pin for low side and leave others cleared
  PORTB.OUTCLR = PIN0_bm | PIN1_bm;
  PORTC.OUT = PIN5_bm;
}
void BHCL() {
  // Set up PWM pin(s) for high side
  TCA0.SPLIT.CTRLB = TCA_SPLIT_LCMP0EN_bm;

  // Set pin for low side and leave others cleared
  PORTB.OUTCLR = PIN5_bm | PIN1_bm;
  PORTC.OUT = PIN5_bm;
}
void BHAL() {
  // Set up PWM pin(s) for high side
  TCA0.SPLIT.CTRLB = TCA_SPLIT_LCMP0EN_bm;

  // Set pin for low side and leave others cleared
  PORTB.OUTSET = PIN1_bm;
  PORTC.OUT = 0;
}
void CHAL() {
  // Set up PWM pin(s) for high side
  TCA0.SPLIT.CTRLB = TCA_SPLIT_HCMP1EN_bm;

  // Set pin for low side and leave others cleared
  PORTB.OUTSET = PIN1_bm;
  PORTC.OUT = 0;
}
void CHBL() {
  // Set up PWM pin(s) for high side
  TCA0.SPLIT.CTRLB = TCA_SPLIT_HCMP1EN_bm;

  // Set pin for low side and leave others cleared
  PORTB.OUTCLR = PIN0_bm | PIN1_bm;
  PORTC.OUT = PIN3_bm;
}
void allFloat() {
  // Disable PWM timer
  TCA0.SPLIT.CTRLA = 0;
  TCA0.SPLIT.CTRLB = 0; // No control over output

  // Set all outputs to low (motor coasts)
  PORTC.OUTCLR = PIN3_bm | PIN4_bm | PIN5_bm;
  PORTB.OUTCLR = PIN0_bm | PIN1_bm | PIN5_bm;
}
void allLow() {
  // Disable PWM timer
  TCA0.SPLIT.CTRLA = 0;
  TCA0.SPLIT.CTRLB = 0; // No control over output

  // Set all outputs to low
  PORTC.OUTCLR = PIN3_bm | PIN4_bm | PIN5_bm;
  PORTB.OUTCLR = PIN0_bm | PIN1_bm | PIN5_bm;

  // Set all bridges to pull low (brakes the motor)
  PORTC.OUTSET = PIN3_bm | PIN5_bm;
  PORTB.OUTSET = PIN1_bm;
}

// Comparator functions

void aRisingBEMF() {
  AC1.MUXCTRLA = AC_MUXPOS_PIN1_gc;
  TCB0.EVCTRL = TCB_CAPTEI_bm; // Enable event capture input (AC), on rising edge
}
void aFallingBEMF() {
  AC1.MUXCTRLA = AC_MUXPOS_PIN1_gc;
  TCB0.EVCTRL = TCB_CAPTEI_bm | TCB_EDGE_bm; // Enable event capture input (AC), on falling edge
}
void bRisingBEMF() {
  AC1.MUXCTRLA = AC_MUXPOS_PIN0_gc;
  TCB0.EVCTRL = TCB_CAPTEI_bm; // Enable event capture input (AC), on rising edge
}
void bFallingBEMF() {
  AC1.MUXCTRLA = AC_MUXPOS_PIN0_gc;
  TCB0.EVCTRL = TCB_CAPTEI_bm | TCB_EDGE_bm; // Enable event capture input (AC), on falling edge
}
void cRisingBEMF() {
  AC1.MUXCTRLA = AC_MUXPOS_PIN3_gc;
  TCB0.EVCTRL = TCB_CAPTEI_bm; // Enable event capture input (AC), on rising edge
}
void cFallingBEMF() {
  AC1.MUXCTRLA = AC_MUXPOS_PIN3_gc;
  TCB0.EVCTRL = TCB_CAPTEI_bm | TCB_EDGE_bm; // Enable event capture input (AC), on falling edge
}