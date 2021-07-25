#ifndef ESC_UARTCOMMS_HEADER
#define ESC_UARTCOMMS_HEADER

#include <Arduino.h>
#include "motor.h"

extern const uint32_t UART_BAUDRATE; // Baudrate used for UART

/** @name uartSetup
   *  @brief Sets up UART (Serial) interface
   */
void uartSetup();

/** @name uartCommnads
   *  @brief Handles commands recieved over UART in form "C(-XXX)" 
   *  where C is the command ID and XXX is the payload (optional, 
   *  but must be seperated by a dash if present)
   */
void uartCommands();

#endif