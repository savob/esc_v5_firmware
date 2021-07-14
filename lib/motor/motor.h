#include <Arduino.h>

extern volatile byte sequenceStep; // Stores step in spinning sequence

extern volatile bool motorUpslope; // Used for PWM
extern volatile bool test2;

// PWM variables
extern volatile byte period;           // MUST be less than 254
extern volatile byte duty;
extern volatile byte nDuty;  // Used to store the "inverse" duty, to reduce repeated operations withinm the interupt to calculate this
extern byte endClampThreshold;           // Stores how close you need to be to either extreme before the PWM duty is clamped to that extreme

// AH_BL, AH_ CL, BH_CL, BH_AL, CH_AL, CH_BL
extern volatile byte motorPortSteps[6];

// Other variables
extern volatile byte cyclesPerRotation;
extern volatile byte cycleCount;

extern volatile unsigned int currentRPM;
extern volatile unsigned int targetRPM;
extern volatile byte controlScheme;

extern bool reverse;
extern volatile bool motorStatus; // Stores if the motor is disabled (false) or not

extern const unsigned int spinUpStartPeriod;
extern const unsigned int spinUpEndPeriod;
extern const byte stepsPerIncrement;
extern const unsigned int spinUpPeriodDecrement;

// Buzzer period limits
extern const unsigned int maxBuzzPeriod;
extern const unsigned int minBuzzPeriod;



////////////////////////////////////////////////////////////
// Function declarations

/** @name Set PWM Duty
   *  @brief Set the PWM duty of the motor
   *  @param deisredDuty Desired duty
   */
void setPWMDuty(byte deisredDuty);

/** @name Motor enable
   *  @brief Use this to enable the motor
   *  @param startDuty Motor duty to start with
   *  @return Returns true if the motor was enabled without issue
   */
bool enableMotor(byte startDuty);

/** @name Motor windup
   *  @brief Winds up the motor to start spinning
   */
void windUpMotor();

/** @name Motor disable
   *  @brief Disables motor
   */
void disableMotor();

/** @name Buzz
   *  @brief Use this for a buzzes for user awareness. Motor needs to be disabled to work.
   *  @param  periodMicros Period of buzz tone
   *  @param  durationMillis Duration of buzz overall
   */
void buzz(int periodMicros, int durationMillis);

void AH_BL();
void AH_CL();
void BH_CL();
void BH_AL();
void CH_AL();
void CH_BL();

/* Since we are comparing to relative the positive terminal, our edges have to be reversed

  E.g. If A is rising it will go from being less than the zero to more.
  Since A is connected to the negative terminal, the comparator output will go from 1 to 0

  So although we are looking for a rising edge, our microcontroller will see a falling one

  NOTE THAT ALL THESE WILL ENABLE THE COMPARATOR INTERRUPT!
*/
void BEMF_A_RISING();
void BEMF_A_FALLING();
void BEMF_B_RISING();
void BEMF_B_FALLING();
void BEMF_C_RISING();
void BEMF_C_FALLING();