#pragma once

#include <Adafruit_NeoPixel.h>
#include "config.h"

/**
 * @brief Initialize the LED strip
 */
void ledInit();

/**
 * @brief Set all LEDs to a specific traffic level color
 */
void ledSetAll(TrafficLevel level);

/**
 * @brief Set LEDs for a specific route
 * @param routeIndex Route index (0-4)
 * @param level Traffic level to display
 */
void ledSetRoute(int routeIndex, TrafficLevel level);

/**
 * @brief Clear all LEDs (turn off)
 */
void ledClear();

/**
 * @brief Show startup animation sequence
 */
void ledStartupSequence();

/**
 * @brief Show OTA update progress
 * @param progress Progress percentage (0-100)
 */
void ledShowOTAProgress(unsigned int progress);

/**
 * @brief Show OTA success animation
 */
void ledShowOTASuccess();

/**
 * @brief Show OTA error animation
 */
void ledShowOTAError();