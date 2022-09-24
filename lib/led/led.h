// WhY dO YOu nEEd a LeD lIBraRy?
// beCAuSe i cAn

#ifndef ESC_LED_HEADER
#define ESC_LED_HEADER

#include <Arduino.h>

////////////////////////////////////////////////////////////
// Function declarations

/** @name LEDSetup
   *  @brief Set up the built in status LED for use. LED remains on afterwards.
   *  @note LED will be left on when complete 
   */
void LEDSetup();

/** @name LEDOn
   *  @brief Turns on built in LED
   */
void LEDOn();

/** @name LEDOff
   *  @brief Turns off built in LED
   */
void LEDOff();

/** @name LEDToggle
   *  @brief Toggles (switches) the built in LED's state
   */
void LEDToggle();

/** @name LEDSetTo
   *  @brief Set's the LED to match the state of passed in variable
   *  @param setTo The desired state for the LED (LED goes ON if TRUE)
   */
void LEDSetTo(bool setTo);

/** @name LEDBlinkBlocking
   *  @brief Blink the built in LED a set number of times, blocking (pausing) the rest of the code execution. LED remains on afterwards.
   *  @param period Period of each blink in milliseconds
   *  @param count Number of consecutive blinks
   *  @note LED will be left on when complete
   */
void LEDBlinkBlocking(int period, int count);

/** @name nonBlockingLEDBlink
   *  @brief Blink the built in LED a set number of times, without blocking other code. LED remains on afterwards. Repeatedly called to update LED.
   *  @note Should be used at the end of the main control loop
   */
void nonBlockingLEDBlink();

/** @name setNonBlockingBlink
   *  @brief Set the built in LED to blink  a set number of times, without blocking (pausing) the rest of the code execution.
   *  @param period Period of each blink in milliseconds
   *  @param count Number of consecutive blinks
   *  @note LED will be left on when complete
   */
void setNonBlockingBlink (unsigned int period, unsigned int count);

#endif