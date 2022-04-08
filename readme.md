# ESC V5 Firmware

My code to operate my custom electronic speed controllers. Written using PlatformIO in the Arduino framework for an ATtiny1617 chip.

Standard BEMF ESC, but is primarily designed to be controlled digitally over I2C.

This code is based on my previous work for my fourth version. This code however is not completely compatible with its hardware due to differening pin allocations for the MOSFET driver. Perhaps I will invest some time into some `#define` and `#ifdef` structures to make the code easy to switch between them.
