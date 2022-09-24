#ifndef ESC_MOTOR_HEADER
#define ESC_MOTOR_HEADER

#include <Arduino.h>

// PWM variables
extern const byte maxDuty;       // Upper limit to PWM (MUST be less than 256)
extern volatile byte duty;       // Current PWM duty
extern const byte minDuty;       // Stores minimum allowed duty

// Commutation Constants
extern volatile byte cyclesPerRotation;

// Control Scheme Variables
enum ctrlSchemeEnum: byte {PWM = 0, RPM = 1}; // Enumerator used for unambiguous control scheme setting
extern volatile ctrlSchemeEnum controlScheme; // Determines whether the ESC uses PWM (0) or RPM (1) control

// RPM related
extern volatile unsigned int targetRPM;

// Motor rotation variables and constants
extern volatile bool reverse;
extern volatile bool motorStatus; // Stores if the motor is disabled (false) or not

////////////////////////////////////////////////////////////
// Function declarations

/** @name setPWMDuty
   *  @brief Set the PWM duty of the motor
   *  @param deisredDuty Desired duty
   */
void setPWMDuty(byte deisredDuty);

/** @name enableMotor
   *  @brief Use this to enable the motor
   *  @param startDuty Motor duty to start with
   *  @return Returns true if the motor was enabled without issue. False if duty selected was too low or motor is already spinning.
   */
bool enableMotor(byte startDuty);

/** @name windUpMotor
   *  @brief Winds up the motor to start spinning
   */
void windUpMotor();

/** @name setupMotor
   *  @brief Setup microcontroller to operate the motor
   */
void setupMotor();

/** @name disableMotor
   *  @brief Disables motor immediately
   */
void disableMotor();

/** @name buzz
   *  @brief Use this for a buzzes for user awareness. Motor needs to be disabled to work.
   *  @param  periodMicros Period of buzz tone in microseconds
   *  @param  durationMillis Duration of buzz overall in milliseconds
   */
void buzz(int periodMicros, int durationMillis);

/** @name allFloat
   *  @brief Sets all motor half-bridges to float, motor coasts to a stop.
   */
void allFloat();

/** @name allLow
   *  @brief Pulls all half-bridges low, braking the motor to a sudden stop.
   */
void allLow();    // Set all outputs to float (braking)

/** @name getCurrentRPM
   *  @brief Extrapolate current RPM based on the half-step duration
   *  @return Extrapolated RPM as an unsigned int
   */
unsigned int getCurrentRPM(); // Returns current RPM

/** @name setToBuzz
   *  @brief Use this to set the system to buzz outside an interrupt. Motor needs to be disabled to work.
   *  @param  periodMicros Period of buzz tone in microseconds
   *  @param  durationMillis Duration of buzz overall in milliseconds
   */
void setToBuzz(unsigned int period, unsigned int duration);

/** @name runInterruptBuzz
   *  @brief Runs a buzz that was called for within an interrupt using "setToBuzz"
   */
void runInterruptBuzz();

#endif