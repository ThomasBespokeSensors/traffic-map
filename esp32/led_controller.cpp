#include "led_controller.h"

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);

// Color definitions
struct Color {
    uint8_t r, g, b;
};

const Color COLOR_GREEN     = {0, 255, 0};      // Normal traffic
const Color COLOR_YELLOW    = {255, 255, 0};    // Moderate traffic
const Color COLOR_RED       = {255, 0, 0};      // Heavy traffic
const Color COLOR_BLUE      = {0, 0, 255};      // Error state
const Color COLOR_OFF       = {0, 0, 0};        // Off

static Color getColorForLevel(TrafficLevel level) {
    switch (level) {
        case TRAFFIC_NORMAL:   return COLOR_GREEN;
        case TRAFFIC_MODERATE: return COLOR_YELLOW;
        case TRAFFIC_HEAVY:    return COLOR_RED;
        case TRAFFIC_ERROR:    return COLOR_BLUE;
        default:               return COLOR_OFF;
    }
}

void ledInit() {
    strip.begin();
    strip.setBrightness(LED_BRIGHTNESS);
    strip.clear();
    strip.show();
    Serial.println("LED strip initialized");
}

void ledSetRoute(int routeIndex, TrafficLevel level) {
    if (routeIndex < 0 || routeIndex >= 5) {
        Serial.println("Error: Invalid route index");
        return;
    }
    
    Color color = getColorForLevel(level);
    int startLED = routeIndex * 3;
    
    for (int i = 0; i < 3; i++) {
        strip.setPixelColor(startLED + i, strip.Color(color.r, color.g, color.b));
    }
    strip.show();
}

void ledClear() {
    strip.clear();
    strip.show();
}

void ledStartupSequence() {
    Serial.println("Running LED startup sequence...");
    
    // Light up each route in blue sequentially
    for (int route = 0; route < 5; route++) {
        int startLED = route * 3;
        for (int i = 0; i < 3; i++) {
            strip.setPixelColor(startLED + i, strip.Color(COLOR_BLUE.r, COLOR_BLUE.g, COLOR_BLUE.b));
        }
        strip.show();
        delay(300);
    }
    
    // All off
    ledClear();
    delay(500);
    
    // Flash green 3 times to indicate ready
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < LED_COUNT; j++) {
            strip.setPixelColor(j, strip.Color(COLOR_GREEN.r, COLOR_GREEN.g, COLOR_GREEN.b));
        }
        strip.show();
        delay(200);
        ledClear();
        delay(200);
    }
    
    Serial.println("LED mapping:");
    Serial.println("  Route 1 (A14 Westbound)      -> NeoPixels 1-3   (indices 0-2)");
    Serial.println("  Route 2 (A43)                -> NeoPixels 4-6   (indices 3-5)");
    Serial.println("  Route 3 (A14 Kettering)      -> NeoPixels 7-9   (indices 6-8)");
    Serial.println("  Route 4 (A45)                -> NeoPixels 10-12 (indices 9-11)");
    Serial.println("  Route 5 (A14 Eastbound)      -> NeoPixels 13-15 (indices 12-14)");
}