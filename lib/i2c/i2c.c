#include "i2c.h"

byte i2cAddress = 10;      // I2C address. Starts with this default, then adds offset according to soldering pads
byte currentInstruction = 0;


void i2cRecieve(int howMany) {
  currentInstruction = Wire.read();

  Serial.print(F("Recieved: "));
  Serial.println(currentInstruction);

  // Check for KILL ORDER
  if (currentInstruction == 0) {
    disableMotor();

    while (true) {
      // Trap ESC in loop until reboot
    }
  }

  switch (currentInstruction) {
    case 1:
      // Reverse, read only
      Serial.print(F("Reverse: "));
      Serial.println(reverse);
      break;
    case 2:
      // Duty, set if another byte present
      if (Wire.available()) {
        enableMotor(Wire.read());
      }
      Serial.print(F("Duty: "));
      Serial.println(duty);
      break;
    case 3:
      // Current RPM, read only
      Serial.print(F("Current RPM: "));
      Serial.println(currentRPM);
      break;
    case 4:
      // Control scheme
      if (Wire.available()) {
        controlScheme = Wire.read();
      }
      Serial.print(F("Control Scheme: "));
      Serial.println(controlScheme);
      break;
    case 5:
      // Target RPM
      if (Wire.available()) {
        targetRPM = readWordWire(); // Read
      }
      Serial.print(F("Target RPM: "));
      Serial.println(targetRPM);
      break;
    case 6:
      // Number of cycles in a rotation
      if (Wire.available()) {
        cyclesPerRotation = Wire.read();
      }
      Serial.print(F("Cycles per rotation: "));
      Serial.println(cyclesPerRotation);
      break;
    case 7:
      if (Wire.available()) motorStatus = Wire.read();

      // Enable or disable motor
      if (motorStatus) enableMotor(endClampThreshold + 1); // Set motor to minimum
      else disableMotor();

      Serial.print(F("motorStatus: "));
      Serial.println(motorStatus);
      break;
  }

  // Clear buffer of any other fluff
  while (Wire.available()) {
    Wire.read();
  }
}

void i2cRequest() {
  byte part1, part2;

  switch (currentInstruction) {
    case 0:
      Wire.write(motorStatus);
      break;
    case 1:
      Wire.write(reverse);
      break;
    case 2:
      Wire.write(duty);
      break;
    case 3:
      // RPM
      sendWordWire(currentRPM);
      break;
    case 4:
      // Control scheme
      Wire.write(controlScheme);
      break;
    case 5:
      // Target RPM
      sendWordWire(targetRPM);
      break;
    case 6:
      // Cycles in a rotation
      Wire.write(cyclesPerRotation);
      break;
    case 7:
      // Cycles in a rotation
      Wire.write(motorStatus);
      break;
  }

}
void sendWordWire(word dataValue) {
  // Writes a word to wire interface
  // High byte first
  byte part1 = dataValue % 256;
  byte part2 = dataValue / 256;
  Wire.write(part2);
  Wire.write(part1);
}
word readWordWire() {
  byte part1 = Wire.read();
  byte part2 = Wire.read();
  word dataRecieved;
  dataRecieved = part1 * 256;
  dataRecieved += part2;
  return (dataRecieved);
}