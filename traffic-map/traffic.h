#pragma once

#include <vector>
#include <Arduino.h>

struct LatLng {
    double lat;
    double lng;
};

enum TrafficLevel {
    TRAFFIC_NORMAL = 0,
    TRAFFIC_MODERATE = 1,
    TRAFFIC_HEAVY = 2,
    TRAFFIC_ERROR = -1
};

struct RouteConfig {
    String name;
    LatLng origin;
    LatLng destination;
    std::vector<LatLng> intermediates;
    int normalLimitSec;
    int heavyLimitSec;
};

struct RouteStatus {
    String name;
    TrafficLevel level;
    int durationSec;
};

/**
 * @brief Checks traffic level on a fixed route
 *
 * @param origin Start coordinate
 * @param destination End coordinate
 * @param intermediates Optional via points (can be empty)
 * @param lowerTimeSec Threshold for NORMAL → MODERATE
 * @param upperTimeSec Threshold for MODERATE → HEAVY
 * @return TrafficLevel
 */
TrafficLevel checkTrafficLevel(
    const LatLng& origin,
    const LatLng& destination,
    const std::vector<LatLng>& intermediates,
    int lowerTimeSec,
    int upperTimeSec
);

/**
 * @brief Checks traffic level and returns duration
 */
TrafficLevel checkTrafficLevelWithDuration(
    const LatLng& origin,
    const LatLng& destination,
    const std::vector<LatLng>& intermediates,
    int lowerTimeSec,
    int upperTimeSec,
    int& outDurationSec
);