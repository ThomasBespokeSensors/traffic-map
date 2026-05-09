#pragma once

#include <Arduino.h>

/**
 * @brief Check for firmware updates and install if available
 * @return true if update found and successful, false otherwise
 */
bool checkForUpdate();

/**
 * @brief Get current firmware version
 */
String getFirmwareVersion();