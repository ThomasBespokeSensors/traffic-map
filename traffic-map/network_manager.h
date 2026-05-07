#pragma once

#include <Arduino.h>

/**
 * @brief Initialize WiFi connection
 * @return true if connection successful, false otherwise
 */
bool networkInit();

/**
 * @brief Setup NTP time synchronization
 */
void networkSetupTime();

/**
 * @brief Setup OTA (Over-The-Air) updates
 */
void networkSetupOTA();

/**
 * @brief Setup Telnet server for remote serial monitoring
 */
void networkSetupTelnet();

/**
 * @brief Handle network tasks (must be called in loop)
 */
void networkHandle();

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
 * @brief Check if update is allowed based on rate limiting
 * @return true if update is allowed, false otherwise
 */
bool rateLimitCheck();

/**
 * @brief Record that an update has occurred (for rate limiting)
 */
void recordUpdate();

/**
 * @brief Print to both Serial and Telnet
 */
void telnetPrint(const String& message);
void telnetPrintln(const String& message);