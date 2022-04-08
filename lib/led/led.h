// WhY dO YOu nEEd a LeD lIBraRy?
// beCAuSe i cAn

#ifndef ESC_LED_HEADER
#define ESC_LED_HEADER

#include <Arduino.h>

extern const byte LEDpin; // LED pin number

////////////////////////////////////////////////////////////
// Function declarations

/** @name LEDSetup
   *  @brief Set up the built in status LED for use
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

/** @name LEDBlinkBlocking
   *  @brief Blink the built in LED a set number of times, blocking (pausing) the rest of the code execution.
   *  @param period Period of each blink in milliseconds
   *  @param count Number of consecutive blinks
   */
void LEDBlinkBlocking(int period, int count);


#endif