#include "i2c.h"

byte i2cAddress = 10;      // I2C address. Starts with a default, then adds offset according to soldering pads
byte currentInstruction = 0;

void i2cSetup() {

#ifdef UART_COMMS_DEBUG
  Serial.print("I2C set up started...");
  Serial.println(i2cAddress);
#endif

  // Read I2C address settings from the pins PC0, PC1, and PC2
  // Set them as inputs with pullups enabled
  PORTC.DIRCLR = 0x07; 
  PORTC.PIN0CTRL = PORT_PULLUPEN_bm; 
  PORTC.PIN1CTRL = PORT_PULLUPEN_bm;
  PORTC.PIN2CTRL = PORT_PULLUPEN_bm;

  delayMicroseconds(100); // Minor delay for pullups to engage and settle on a value

  // Read the address from these pins
  byte temp = PORTC.IN & 0x07;  // Extract the 3 bits for setting
  temp ^= 0x07;       // Flips the bits (since I am shorting the pads I want to set as "1")
  i2cAddress += temp; // Add offest to default value

  PORTMUX.CTRLB = PORTMUX_TWI0_bm; // Multiplex the I2C/TWI to use the alternate pins

  // Start the I2C interface 
  Wire.begin(i2cAddress);
  Wire.onRequest(i2cRequest);
  Wire.onReceive(i2cRecieve);

#ifdef UART_COMMS_DEBUG
  Serial.print("COMPLETE.\nI2C Address (HEX): ");
  Serial.println(i2cAddress, HEX);
#endif
}

void i2cRecieve(int howMany) {
  currentInstruction = Wire.read();

#ifdef UART_COMMS_DEBUG
  Serial.print("Recieved I2C command type: ");
  Serial.println(currentInstruction);
#endif

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
      break;
    case 2:
      // Duty, set if another byte present
      if (Wire.available()) {
        enableMotor(Wire.read());
      }
      break;
    case 3:
      // Current RPM, read only
      break;
    case 4:
      // Control scheme
      if (Wire.available()) {
        controlScheme = Wire.read();
      }
      break;
    case 5:
      // Target RPM
      if (Wire.available()) {
        targetRPM = readWordWire(); // Read
      }
      break;
    case 6:
      // Number of cycles in a rotation
      if (Wire.available()) {
        cyclesPerRotation = Wire.read();
      }
      break;
    case 7:
      if (Wire.available()) motorStatus = Wire.read();

      // Enable or disable motor
      if (motorStatus) enableMotor(endClampThreshold + 1); // Set motor to minimum
      else disableMotor();
      
      break;
  }

  // Clear buffer of any other fluff
  while (Wire.available()) {
    Wire.read();
  }
}

void i2cRequest() {

#ifdef UART_COMMS_DEBUG
  Serial.print("Recieved I2C request type: ");
  Serial.println(currentInstruction);
#endif

  // Returns a variable based on the previous command
  
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