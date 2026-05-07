#pragma once

#include <Arduino.h>
#include "config.h"

/**
 * @brief Initialize the traffic API (must be called after WiFi is connected)
 */
void trafficApiInit();

/**
 * @brief Check traffic level and get duration for a route
 * 
 * @param origin Starting coordinates
 * @param destination Ending coordinates
 * @param intermediates Optional waypoints
 * @param lowerTimeSec Threshold for NORMAL → MODERATE
 * @param upperTimeSec Threshold for MODERATE → HEAVY
 * @param outDurationSec Output parameter for actual duration
 * @return TrafficLevel Current traffic status
 */
TrafficLevel checkTrafficLevel(
    const LatLng& origin,
    const LatLng& destination,
    const std::vector<LatLng>& intermediates,
    int lowerTimeSec,
    int upperTimeSec,
    int& outDurationSec
);

/**
 * @brief Convert traffic level to human-readable string
 */
const char* trafficLevelToString(TrafficLevel level);