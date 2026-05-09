#pragma once

#include <Arduino.h>

/**
 * @brief Setup NTP time synchronization
 */
void timeInit();

/**
 * @brief Check if current time is within active hours
 * @return true if within active hours, false otherwise
 */
bool isWithinActiveHours();

/**
 * @brief Get milliseconds until active hours begin
 * @return milliseconds to wait
 */
unsigned long getTimeUntilActiveHours();

/**
 * @brief Get current time as a formatted string
 * @param buffer Buffer to write time string to
 * @param bufferSize Size of buffer
 * @return true if successful, false if time not available
 */
bool getCurrentTimeString(char* buffer, size_t bufferSize);