#include <Wire.h>

extern byte i2cAddress; // I2C address. Starts with a default, then adds offset according to soldering pads
extern byte currentInstruction; // Current instruction recieved by ESC from controller

void i2cRecieve(int howMany);
void i2cRequest();
void sendWordWire(word dataValue);
word readWordWire();