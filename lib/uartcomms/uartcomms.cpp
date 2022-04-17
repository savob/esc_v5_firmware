#include "uartcomms.h"

const uint32_t UART_BAUDRATE = 115200;

void uartSetup() {
  // Just need to start serial port
  Serial.begin(UART_BAUDRATE);

  Serial.println("\n==========================\nNEW ESC UART SESSION\n==========================\n");
}

void uartCommands() {
  byte currentUARTInstruction = Serial.parseInt();

  // Check for KILL ORDER
  if (currentUARTInstruction == 0) {
    disableMotor();

    Serial.println("DISABLING MOTOR PERMANENTLY");
    while (true) {
      // Trap ESC in loop until reboot
    }
  }

  delay(10);

  if (currentUARTInstruction == 1) {
    // Reverse status
    if (reverse == true) Serial.println("ESC is spinning in reverse.");
    else Serial.println("ESC is spinning normally.");
  }
  else if (currentUARTInstruction == 2) {
    // Duty, set if another byte present
    if (Serial.available()) {
      Serial.read(); // Remove dash

      byte inputDuty = Serial.parseInt();
      if (motorStatus == true) {
        setPWMDuty(inputDuty);
      }
      else {
        enableMotor(inputDuty);
      }
    }
    // Feedback
    Serial.printf("Current PWM duty: %d\n", duty);
  }
  else if (currentUARTInstruction == 3) {
    // Current RPM, read only
    Serial.printf("Current RPM: %d\n", getCurrentRPM());
  }
  else if (currentUARTInstruction == 4) {
    // Control scheme
    if (Serial.available()) {
      Serial.read(); // Remove dash
      controlScheme = ctrlSchemeEnum(Serial.parseInt());
    }

    // Feedback to user
    if (controlScheme == ctrlSchemeEnum::PWM) Serial.println("PWM control");
    else Serial.println("RPM-control");

  }
  else if (currentUARTInstruction == 5) {
    // Target RPM
    if (Serial.available()) {
      Serial.read(); // Remove dash
      targetRPM = Serial.parseInt(); // Read in an integer
    }
    // Feedback
    Serial.print("Target RPM: ");
    Serial.println(targetRPM);
  }
  else if (currentUARTInstruction == 6) {
    // Number of cycles in a rotation
    if (Serial.available()) {
      Serial.read(); // Remove dash
      cyclesPerRotation = Serial.parseInt();
    }
    // Feedback
    Serial.printf("Cycles in a rotaion: %d (%d steps)\n", cyclesPerRotation, cyclesPerRotation * 6);      
  }
  else if (currentUARTInstruction == 7) {
    byte desiredDuty;
    if (Serial.available()) desiredDuty = Serial.parseInt();

    // Enable or disable motor
    if (desiredDuty > minDuty) {
      enableMotor(desiredDuty);

      Serial.println("\n! MOTOR ENABLED !\n");
      Serial.printf("Current PWM duty: %d\n", duty);
    }
    else {
      disableMotor();

      Serial.println("\n! MOTOR DISABLED !\n");
    }
  }
  else if (currentUARTInstruction == 8) {
    // Blink LED
    // Expects parameters split by a letter. E.g. 8a2000a5 will blink for 2000ms, five times

    delay(100);

    unsigned int blinkPeriod = Serial.parseInt();
    unsigned int blinkCount = Serial.parseInt();
    
    Serial.printf("Blinking LED for %d ms, %d times.\n", blinkPeriod, blinkCount);

    LEDBlinkBlocking(blinkPeriod, blinkCount);
      
  }
  else if (currentUARTInstruction == 9) {
    // Buzzing
    // Expects parameters split by a letter. E.g. 9a1000a500 will buzz with a period of 2000 us for 500 ms
    
    delay(100);

    unsigned int buzzPeriod = Serial.parseInt();
    unsigned int buzzDuration = Serial.parseInt();
    
    Serial.printf("Buzzing with period of %d us for %d ms.\n", buzzPeriod, buzzDuration);

    buzz(buzzPeriod, buzzDuration);
  }

  // Clear buffer of any other fluff
  while (Serial.available()) {
    Serial.read();
  }
}