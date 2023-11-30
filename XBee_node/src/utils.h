#include <Arduino.h>

int waitForByte(HardwareSerial &serial, int timeoutCnt = 1000);
void print_hex(char *buffer, int length);