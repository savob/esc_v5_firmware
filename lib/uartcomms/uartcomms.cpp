#include "uartcomms.h"

const uint32_t UART_BAUDRATE = 115200;

void uartSetup() {
  // Just need to start serial port
  Serial.begin(UART_BAUDRATE);
}

void uartCommands() {
  byte currentUARTInstruction = Serial.parseInt();

  // Check for KILL ORDER
  if (currentUARTInstruction == 0) {
    disableMotor();

    while (true) {
      // Trap ESC in loop until reboot
    }
  }

  switch (currentUARTInstruction) {
    case 1:
      // Reverse status
      if (reverse == true) Serial.println("ESC is spinning in reverse.");
      else Serial.println("ESC is spinning normally.");
      break;
    case 2:
      // Duty, set if another byte present
      if (Serial.available()) {
        Serial.read(); // Remove dash
        enableMotor(Serial.parseInt());
      }
      // Feedback
      Serial.print("Current PWM duty: ");
      Serial.println(duty);
      break;
    case 3:
      // Current RPM, read only
      Serial.print("Current RPM: ");
      Serial.println(currentRPM);
      break;
    case 4:
      // Control scheme
      if (Serial.available()) {
        Serial.read(); // Remove dash
        controlScheme = Serial.parseInt();
      }

      // Feedback to user
      if (controlScheme == 0) Serial.println("PWM control");
      else Serial.println("RPM-control");

      break;
    case 5:
      // Target RPM
      if (Serial.available()) {
        Serial.read(); // Remove dash
        targetRPM = Serial.parseInt(); // Read in an integer
      }
      // Feedback
      Serial.print("Target RPM: ");
      Serial.println(targetRPM);
      break;
    case 6:
      // Number of cycles in a rotation
      if (Serial.available()) {
        Serial.read(); // Remove dash
        cyclesPerRotation = Serial.parseInt();
      }
      // Feedback
      Serial.print("Cycles in roataion: ");
      Serial.println(cyclesPerRotation);
      break;
    case 7:
      if (Serial.available()) motorStatus = Serial.parseInt();

      // Enable or disable motor
      if (motorStatus) {
        enableMotor(endClampThreshold + 1); // Set motor to minimum

        Serial.println("\n! MOTOR ENABLED !\n");
        Serial.print("Current PWM duty: ");
        Serial.println(duty);
      }
      else {
        disableMotor();

        Serial.println("\n! MOTOR DISABLED !\n");
      }
      break;
  }

  // Clear buffer of any other fluff
  while (Serial.available()) {
    Serial.read();
  }
}