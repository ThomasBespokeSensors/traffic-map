#pragma once

#include <Arduino.h>

/**
 * @brief Check if update is allowed based on rate limiting
 * @return true if update is allowed, false otherwise
 */
bool rateLimitCheck();

/**
 * @brief Record that an update has occurred
 */
void recordUpdate();