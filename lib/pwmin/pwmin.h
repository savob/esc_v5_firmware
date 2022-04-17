#ifndef ESC_PWMIN_HEADER
#define ESC_PWMIN_HEADER

#include <Arduino.h>

/** @name pwmInputSetup
   *  @brief Sets up PWM input pin to operate
   */
void pwmInputSetup();

/** @name checkPWMTimeOut
  * @brief Checks if the PWM input has timed out (not been detected in a set period)
  * @return Returns true if PWM has timed out
  * */   
bool checkPWMTimeOut();

#endif