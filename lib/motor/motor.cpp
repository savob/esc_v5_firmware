#include "motor.h"
#include "led.h"

volatile byte sequenceStep = 0; // Stores step in spinning sequence
volatile voidFunctionPointer motorSteps[6]; // Stores the functions to copmmute in the current commutation order
volatile voidFunctionPointer bemfSteps[6]; // Stores the functions to set the BEMF in the current commutation order

// PWM variables
volatile byte maxDuty = 249;           // MUST be less than 254
volatile byte duty = 100;
const byte endClampThreshold = 15;           // Stores how close you need to be to either extreme before the PWM duty is clamped to that extreme

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
volatile bool reverse = false;
volatile bool motorStatus = false; // Stores if the motor is disabled (false) or not

// Spin up constants/variables
const unsigned int spinUpStartPeriod = 5000;    // Starting period for each motor step (microseconds)
const unsigned int spinUpEndPeriod = 500;       // Final step period for motor
const byte stepsPerIncrement = 6;               // Number of steps before period is decremented
const unsigned int spinUpPeriodDecrement = 10;  // How much the period is decremented each cycle

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
  Serial.println("Motor spinning set for normal direction.");
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
  Serial.println("Motor spinning set for reverse direction.");
#endif
  }

  //==============================================
  // Set up output pins
  PORTMUX.CTRLC = PORTMUX_TCA04_bm | PORTMUX_TCA03_bm | PORTMUX_TCA02_bm; // Multiplexed PWM outputs
  PORTA.DIRSET = PIN5_bm;
  PORTC.DIRSET = PIN3_bm | PIN4_bm; 
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
  AC1.MUXCTRLA = AC_MUXNEG_PIN1_gc; // Set negative reference to be Zero pin (PB7)
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

  // Reset motor to base state
  allLow();
  setPWMDuty(maxDuty); // Need maximum PWM for wind up

  LEDOff(); // Used to indicate wind up start

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

  LEDOn();
}

void setPWMDuty(byte deisredDuty) { // Set the duty of the motor PWM
  
  // Checks if input is too low and disables if needed
  if (deisredDuty < endClampThreshold) {
    disableMotor();
#ifdef UART_COMMS_DEBUG
    Serial.println("Duty too low, disabling.");
#endif
    return;
  }

  // Check and clamp if near the high extreme
  if (deisredDuty >= (maxDuty - endClampThreshold)) {\
    duty = maxDuty;
#ifdef UART_COMMS_DEBUG
    Serial.println("Duty close to maximum, clamping it to max.");
#endif
  }
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

bool enableMotor(byte startDuty) { // Enable motor with specified starting duty, returns false if duty is too low or motor is already spinning

  // Return false if duty too low, keep motor disabled
  if (startDuty < endClampThreshold) {
    disableMotor();
#ifdef UART_COMMS_DEBUG
    Serial.println("Duty too low to enable.");
#endif
    return (false);
  }

  if (motorStatus == true) {
#ifdef UART_COMMS_DEBUG
    Serial.println("Motor already spinning.");
#endif
    return (false);
  }

  // Enable PWM timers
  TCA0.SPLIT.CTRLESET = TCA_SPLIT_CMD_RESTART_gc | 0x03; // Reset both timers
  TCA0.SPLIT.CTRLA = TCA_SPLIT_CLKSEL_DIV1_gc | TCA_SPLIT_ENABLE_bm; // Enable the timer with prescaler of 16

  windUpMotor(); // Needs to happen once PWM is activated so top side can be driven

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
  bemfSteps[sequenceStep](); // Set proper interrupt

  // Set output PWM
  motorStatus = true;
  setPWMDuty(startDuty);  // Set duty for motor

#ifdef UART_COMMS_DEBUG
  Serial.println("\n! MOTOR ENABLED !\n");
#endif

  return (true);
}
void disableMotor() {

  allFloat(); // Coast to a stop
  //allLow(); // Brake to a stop

  // Disable Analog Comparator (BEMF)
  AC1.CTRLA = 0; 

  duty = 0;
  motorStatus = false;

#ifdef UART_COMMS_DEBUG
  Serial.println("\n! MOTOR DISABLED !\n");
#endif
}

// Commutation Period Interrupt
ISR(TCB0_INT_vect) {
  TCB0.INTFLAGS = 1; // Clear interrupt flag
  
  // "Debouncing" if noisy
  if (TCB0.CCMP < 1000) {
    TCB0.CNT = TCB0.CCMP;     // Continue the count as if uninterrupted
    return;
  }

  TCB1.CCMP = TCB0.CCMP / 2;  // Record the half period

  sequenceStep++;             // Increment step by 1, next part in the sequence of 6
  sequenceStep %= 6;
  bemfSteps[sequenceStep]();

#ifdef UART_COMMS_DEBUG
  /* Debug statements
    Three character summaries (one for each phase), ordered A-B-C.
      H - Driven high
      L - Driven low
      R - Looking for rising edge (floating)
      F - Looking for falling edge (floating)
     
    Could reduce each phase's state to two bits to condense it under a byte, 
    thus waste less time sending, but it will be harder to read.
    */ 
  if (reverse == 0) {
    switch (sequenceStep) {
      case 0:
        Serial.println("HLF");
        break;
      case 1:
        Serial.println("HRL");
        break;
      case 2:
        Serial.println("FHL");
        break;
      case 3:
        Serial.println("LHR");
        break;
      case 4:
        Serial.println("LFH");
        break;
      case 5:
        Serial.println("RLH");
        break;
    }
  }
  else {
    switch (sequenceStep) {
      case 0:
        Serial.println("HLR");
        break;
      case 1:
        Serial.println("FLH");
        break;
      case 2:
        Serial.println("LRH");
        break;
      case 3:
        Serial.println("LHF");
        break;
      case 4:
        Serial.println("RHL");
        break;
      case 5:
        Serial.println("HFL");
        break;
    }
  }
#endif
}

// Commutation Interrupt
ISR(TCB1_INT_vect) {
  TCB1.INTFLAGS = 1; // Clear flag

  PORTA.OUTTGL = PIN3_bm; // Toggle for debugging

  //motorSteps[sequenceStep]();

#ifdef ESC_RPM_COUNT
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
#endif
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
    // Clear the state of all pins so only the phases of interest are driven
    //PORTA.OUTCLR = PIN5_bm; // Redunant since it will be set soon anyways
    PORTC.OUTCLR = PIN3_bm | PIN4_bm;
    PORTB.OUTCLR = PIN0_bm | PIN1_bm | PIN5_bm;
    PORTA.OUTSET = PIN5_bm; // AH
    PORTB.OUTSET = PIN1_bm; // BL
    delayMicroseconds(holdOn);
    allLow();
    delayMicroseconds(holdOff);

    //PORTA.OUTCLR = PIN5_bm;
    PORTC.OUTCLR = PIN3_bm | PIN4_bm;
    PORTB.OUTCLR = PIN0_bm | PIN1_bm | PIN5_bm;
    PORTA.OUTSET = PIN5_bm; // AH
    PORTB.OUTSET = PIN0_bm; // CL
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
  TCA0.SPLIT.CTRLB = TCA_SPLIT_HCMP2EN_bm;

  // Set pin for low side and leave others cleared
  PORTB.OUTCLR = PIN0_bm | PIN1_bm | PIN5_bm;
  PORTB.OUTSET = PIN1_bm;
}
void AHCL() {
  // Set up PWM pin(s) for high side
  TCA0.SPLIT.CTRLB = TCA_SPLIT_HCMP2EN_bm;

  // Set pin for low side and leave others cleared
  PORTB.OUTCLR = PIN0_bm | PIN1_bm | PIN5_bm;
  PORTB.OUTSET = PIN0_bm;
}
void BHCL() {
  // Set up PWM pin(s) for high side
  TCA0.SPLIT.CTRLB = TCA_SPLIT_HCMP0EN_bm;

  // Set pin for low side and leave others cleared
  PORTB.OUTCLR = PIN0_bm | PIN1_bm | PIN5_bm;
  PORTB.OUTSET = PIN0_bm;
}
void BHAL() {
  // Set up PWM pin(s) for high side
  TCA0.SPLIT.CTRLB = TCA_SPLIT_HCMP0EN_bm;

  // Set pin for low side and leave others cleared
  PORTB.OUTCLR = PIN0_bm | PIN1_bm | PIN5_bm;
  PORTB.OUTSET = PIN2_bm;
}
void CHAL() {
  // Set up PWM pin(s) for high side
  TCA0.SPLIT.CTRLB = TCA_SPLIT_HCMP1EN_bm;

  // Set pin for low side and leave others cleared
  PORTB.OUTCLR = PIN0_bm | PIN1_bm | PIN5_bm;
  PORTB.OUTSET = PIN2_bm;
}
void CHBL() {
  // Set up PWM pin(s) for high side
  TCA0.SPLIT.CTRLB = TCA_SPLIT_HCMP1EN_bm;

  // Set pin for low side and leave others cleared
  PORTB.OUTCLR = PIN0_bm | PIN1_bm | PIN5_bm;
  PORTB.OUTSET = PIN1_bm;
}
void allFloat() {
  TCA0.SPLIT.CTRLB = 0; // No PWM control over output

  // Set all outputs to low (motor coasts)
  PORTA.OUTCLR = PIN5_bm;
  PORTC.OUTCLR = PIN3_bm | PIN4_bm;
  PORTB.OUTCLR = PIN0_bm | PIN1_bm | PIN5_bm;
}
void allLow() {
  TCA0.SPLIT.CTRLB = 0; // No PWM control over output

  // Set all outputs to low
  PORTA.OUTCLR = PIN5_bm;
  PORTC.OUTCLR = PIN3_bm | PIN4_bm;

  // Set all bridges to pull low (brakes the motor)
  PORTB.OUTSET = PIN0_bm | PIN1_bm | PIN5_bm;
}

// Comparator functions

void aRisingBEMF() {
  AC1.MUXCTRLA = AC_MUXPOS_PIN1_gc | AC_MUXNEG_PIN1_gc;
  TCB0.EVCTRL = TCB_CAPTEI_bm; // Enable event capture input (AC), on rising edge
  TCB1.EVCTRL = TCB_CAPTEI_bm;
}
void aFallingBEMF() {
  AC1.MUXCTRLA = AC_MUXPOS_PIN1_gc | AC_MUXNEG_PIN1_gc;
  TCB0.EVCTRL = TCB_CAPTEI_bm | TCB_EDGE_bm; // Enable event capture input (AC), on falling edge
  TCB1.EVCTRL = TCB_CAPTEI_bm | TCB_EDGE_bm;
}
void bRisingBEMF() {
  AC1.MUXCTRLA = AC_MUXPOS_PIN0_gc | AC_MUXNEG_PIN1_gc;
  TCB0.EVCTRL = TCB_CAPTEI_bm; // Enable event capture input (AC), on rising edge
  TCB1.EVCTRL = TCB_CAPTEI_bm;
}
void bFallingBEMF() {
  AC1.MUXCTRLA = AC_MUXPOS_PIN0_gc | AC_MUXNEG_PIN1_gc;
  TCB0.EVCTRL = TCB_CAPTEI_bm | TCB_EDGE_bm; // Enable event capture input (AC), on falling edge
  TCB1.EVCTRL = TCB_CAPTEI_bm | TCB_EDGE_bm;
}
void cRisingBEMF() {
  AC1.MUXCTRLA = AC_MUXPOS_PIN3_gc | AC_MUXNEG_PIN1_gc;
  TCB0.EVCTRL = TCB_CAPTEI_bm; // Enable event capture input (AC), on rising edge
  TCB1.EVCTRL = TCB_CAPTEI_bm;
}
void cFallingBEMF() {
  AC1.MUXCTRLA = AC_MUXPOS_PIN3_gc | AC_MUXNEG_PIN1_gc;
  TCB0.EVCTRL = TCB_CAPTEI_bm | TCB_EDGE_bm; // Enable event capture input (AC), on falling edge
  TCB1.EVCTRL = TCB_CAPTEI_bm | TCB_EDGE_bm;
}