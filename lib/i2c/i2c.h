#ifndef ESC_I2C_HEADER
#define ESC_I2C_HEADER

#include <Wire.h>
#include "motor.h"
#include "uartcomms.h"

extern byte i2cAddress; // I2C address. Starts with a default, then adds offset according to soldering pads
extern byte currentI2CInstruction; // Current instruction recieved by ESC from controller

/** @name i2cSetup
   *  @brief Sets up I2C interface
   */
void i2cSetup(); // Initialize the device's I2C interface

/** @name i2cRecieve
   *  @brief Handles data recieved over I2C
   *  @param howMany Number of bytes recieved over I2C to handle
   */
void i2cRecieve(int howMany);

/** @name i2cRequest
   *  @brief Function to handle I2C requests
   */
void i2cRequest();

/** @name sendWordWire
   *  @brief Sends a word over I2C
   *  @param dataValue Word to send over I2C
   */
void sendWordWire(word dataValue);

/** @name readWordWire
   *  @brief Read a word from I2C
   *  @return Returns a word recieved over word
   */
word readWordWire();

#endif