#include "led_controller.h"
#include "network_manager.h"

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
    telnetPrintln("LED strip initialized");
}

void ledSetAll(TrafficLevel level) {
    Color color = getColorForLevel(level);
    for (int i = 0; i < LED_COUNT; i++) {
        strip.setPixelColor(i, strip.Color(color.r, color.g, color.b));
    }
    strip.show();
}

void ledSetRoute(int routeIndex, TrafficLevel level) {
    if (routeIndex < 0 || routeIndex >= 5) {
        telnetPrintln("Error: Invalid route index");
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
    telnetPrintln("Running LED startup sequence...");
    
    // Rainbow chase
    for (int j = 0; j < 255; j += 5) {
        for (int i = 0; i < LED_COUNT; i++) {
            strip.setPixelColor(i, strip.ColorHSV((i * 65536 / LED_COUNT + j * 256), 255, 255));
        }
        strip.show();
        delay(10);
    }
    
    // Flash green to indicate ready
    for (int i = 0; i < 3; i++) {
        ledSetAll(TRAFFIC_NORMAL);
        delay(200);
        ledClear();
        delay(200);
    }
    
    telnetPrintln("LED mapping:");
    telnetPrintln("  Route 1 (A14 Eastbound)      -> NeoPixels 1-3   (indices 0-2)");
    telnetPrintln("  Route 2 (A14 Kettering)      -> NeoPixels 4-6   (indices 3-5)");
    telnetPrintln("  Route 3 (A14 Westbound)      -> NeoPixels 7-9   (indices 6-8)");
    telnetPrintln("  Route 4 (A43)                -> NeoPixels 10-12 (indices 9-11)");
    telnetPrintln("  Route 5 (A45)                -> NeoPixels 13-15 (indices 12-14)");
}

void ledShowOTAProgress(unsigned int progress) {
    int ledsToLight = map(progress, 0, 100, 0, LED_COUNT);
    strip.clear();
    for (int i = 0; i < ledsToLight; i++) {
        strip.setPixelColor(i, strip.Color(COLOR_BLUE.r, COLOR_BLUE.g, COLOR_BLUE.b));
    }
    strip.show();
}

void ledShowOTASuccess() {
    for (int i = 0; i < 3; i++) {
        ledClear();
        delay(200);
        ledSetAll(TRAFFIC_NORMAL);
        delay(200);
    }
}

void ledShowOTAError() {
    for (int i = 0; i < 5; i++) {
        ledClear();
        delay(200);
        ledSetAll(TRAFFIC_HEAVY);
        delay(200);
    }
}