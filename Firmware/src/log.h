#pragma once
#include <Arduino.h>
// #include <usb_msc.h>
#define LOG_UART USBSerial.printf
#define LOG_INIT USBSerial.begin