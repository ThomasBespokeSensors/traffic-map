#pragma once

#include <Arduino.h>

/**
 * @brief Initialize logging system
 */
void loggerInit();

/**
 * @brief Log a message (prints to Serial and sends to server)
 */
void logPrint(const String& message);
void logPrintln(const String& message);

void logPrint(int value);
void logPrintln(int value);

void logPrint(unsigned int value);
void logPrintln(unsigned int value);

void logPrint(long value);
void logPrintln(long value);

void logPrint(unsigned long value);
void logPrintln(unsigned long value);

void logPrint(float value);
void logPrintln(float value);

void logPrint(double value);
void logPrintln(double value);

void logPrintln();
void logPrint();