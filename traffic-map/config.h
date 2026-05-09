#pragma once

#include <Arduino.h>
#include <vector>

// ============================================
// Hardware Configuration
// ============================================
#define LED_PIN         14
#define LED_COUNT       15      // 3 LEDs per route × 5 routes
#define LED_BRIGHTNESS  50      // 0-255

// ============================================
// Timing Configuration
// ============================================
#define UPDATE_INTERVAL_MS      (30UL * 60UL * 1000UL)   // 30 minutes
#define MIN_UPDATE_INTERVAL_MS  (2UL * 60UL * 1000UL)    // 2 minutes minimum
#define MAX_UPDATES_PER_HOUR    12

// Active hours (24-hour format)
#define ACTIVE_START_HOUR  7    // 7 AM
#define ACTIVE_END_HOUR    23   // 11 PM

// ============================================
// NTP Configuration
// ============================================
#define NTP_SERVER          "pool.ntp.org"
#define GMT_OFFSET_SEC      0           // UK is GMT
#define DAYLIGHT_OFFSET_SEC 3600        // UK uses BST (GMT+1) in summer

// ============================================
// Raspberry Pi Server Configuration
// ============================================
#define RPI_SERVER_IP   "192.168.1.185"  // Your Raspberry Pi IP
#define RPI_SERVER_PORT 5000
#define FIRMWARE_VERSION "1.0.0"
#define FIRMWARE_FILENAME "traffic-map.ino.bin"

// ============================================
// Data Structures
// ============================================
struct LatLng {
    double lat;
    double lng;
};

struct RouteConfig {
    String name;
    LatLng origin;
    LatLng destination;
    std::vector<LatLng> intermediates;
    int normalLimitSec;
    int heavyLimitSec;
};

enum TrafficLevel {
    TRAFFIC_NORMAL = 0,
    TRAFFIC_MODERATE = 1,
    TRAFFIC_HEAVY = 2,
    TRAFFIC_ERROR = -1
};

// ============================================
// Route Definitions
// ============================================
inline std::vector<RouteConfig> createRoutes() {
    std::vector<RouteConfig> routes;
    
    // Route 1: A14 Westbound (to Market Harborough)
    // LEDs 0-2 (NeoPixels 1-3)
    routes.push_back({
        "Route 1: A14 Westbound (to Market Harborough)",
        {52.390410, -0.749658},
        {52.415523, -0.813886},
        {},
        5 * 60,
        8 * 60
    });

    // Route 2: A43
    // LEDs 3-5 (NeoPixels 4-6)
    routes.push_back({
        "Route 2: A43",
        {52.389625, -0.748810},
        {52.240881, -0.843565},
        {{52.339604, -0.786098}, {52.274583, -0.846307}},
        22 * 60,
        35 * 60
    });

    // Route 3: A14 Kettering (to A43)
    // LEDs 6-8 (NeoPixels 7-9)
    routes.push_back({
        "Route 3: A14 Kettering (to A43)",
        {52.374605, -0.684420},
        {52.389625, -0.748810},
        {},
        4 * 60,
        6 * 60
    });

    // Route 4: A45
    // LEDs 9-11 (NeoPixels 10-12)
    routes.push_back({
        "Route 4: A45",
        {52.370772, -0.712938},
        {52.240881, -0.843565},
        {{52.317039, -0.719304}, {52.282573, -0.712843}},
        23 * 60,
        36 * 60
    });
    
    // Route 5: A14 Eastbound (to Twywell junction)
    // LEDs 12-14 (NeoPixels 13-15)
    routes.push_back({
        "Route 5: A14 Eastbound (to Twywell)",
        {52.375899, -0.682826},
        {52.3837205, -0.6239144},
        {},  // no intermediates
        4 * 60,   // Normal < 4 minutes
        6 * 60    // Heavy > 6 minutes
    });
    
    return routes;
}