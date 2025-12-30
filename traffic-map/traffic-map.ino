#include <WiFi.h>
#include <Adafruit_NeoPixel.h>
#include "traffic.h"
#include "secrets.h"

#define LED_PIN    14
#define LED_COUNT  15  // 3 LEDs per route × 5 routes

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);

const unsigned long INTERVAL_MS = 10UL * 60UL * 1000UL;  // 10 minutes
unsigned long lastRun = 0;

// ---- Route definitions ----
std::vector<RouteConfig> routes;

// LED colors
const uint32_t COLOR_GREEN  = strip.Color(0, 255, 0);
const uint32_t COLOR_YELLOW = strip.Color(255, 255, 0);
const uint32_t COLOR_RED    = strip.Color(255, 0, 0);
const uint32_t COLOR_BLUE   = strip.Color(0, 0, 255);
const uint32_t COLOR_OFF    = strip.Color(0, 0, 0);

const char* trafficToString(TrafficLevel t) {
    switch (t) {
    case TRAFFIC_NORMAL:   return "NORMAL";
    case TRAFFIC_MODERATE: return "MODERATE";
    case TRAFFIC_HEAVY:    return "HEAVY";
    case TRAFFIC_ERROR:    return "ERROR";
    default:               return "UNKNOWN";
    }
}

uint32_t trafficToColor(TrafficLevel t) {
    switch (t) {
    case TRAFFIC_NORMAL:   return COLOR_GREEN;
    case TRAFFIC_MODERATE: return COLOR_YELLOW;
    case TRAFFIC_HEAVY:    return COLOR_RED;
    case TRAFFIC_ERROR:    return COLOR_BLUE;
    default:               return COLOR_OFF;
    }
}

String formatDuration(int seconds) {
    int mins = seconds / 60;
    int secs = seconds % 60;
    return String(mins) + "m " + String(secs) + "s";
}

void connectWiFi() {
    Serial.print("Connecting to WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void setupRoutes() {
    // Route 1: A14 Eastbound (to Twywell junction)
    // LEDs 0-2 (NeoPixels 1-3)
    routes.push_back({
        "Route 1: A14 Eastbound (to Twywell)",
        {52.375899, -0.682826},
        {52.3837205, -0.6239144},
        {},  // no intermediates
        4 * 60,   // Normal < 4 minutes
        6 * 60    // Heavy > 6 minutes
    });

    // Route 2: A14 Kettering (to A43)
    // LEDs 3-5 (NeoPixels 4-6)
    routes.push_back({
        "Route 2: A14 Kettering (to A43)",
        {52.374605, -0.684420},
        {52.389625, -0.748810},
        {},
        4 * 60,
        6 * 60
    });

    // Route 3: A14 Westbound (to Market Harborough)
    // LEDs 6-8 (NeoPixels 7-9)
    routes.push_back({
        "Route 3: A14 Westbound (to Market Harborough)",
        {52.390410, -0.749658},
        {52.415523, -0.813886},
        {},
        5 * 60,
        8 * 60
    });

    // Route 4: A43
    // LEDs 9-11 (NeoPixels 10-12)
    routes.push_back({
        "Route 4: A43",
        {52.389625, -0.748810},
        {52.240881, -0.843565},
        {{52.339604, -0.786098}, {52.274583, -0.846307}},
        22 * 60,
        35 * 60
    });

    // Route 5: A45
    // LEDs 12-14 (NeoPixels 13-15)
    routes.push_back({
        "Route 5: A45",
        {52.370772, -0.712938},
        {52.240881, -0.843565},
        {{52.317039, -0.719304}, {52.282573, -0.712843}},
        23 * 60,
        36 * 60
    });
}

void setRouteLEDs(int routeIndex, uint32_t color) {
    // Each route has 3 LEDs
    int startLED = routeIndex * 3;
    
    for (int i = 0; i < 3; i++) {
        strip.setPixelColor(startLED + i, color);
    }
}

void displayStartupSequence() {
    Serial.println("\nLED Startup Sequence...");
    
    // Show each route's LEDs in sequence
    for (int route = 0; route < 5; route++) {
        setRouteLEDs(route, COLOR_BLUE);
        strip.show();
        delay(300);
    }
    
    // All off
    strip.clear();
    strip.show();
    delay(500);
    
    Serial.println("LED mapping:");
    Serial.println("  Route 1 (A14 Eastbound)      -> NeoPixels 1-3   (LED indices 0-2)");
    Serial.println("  Route 2 (A14 Kettering)      -> NeoPixels 4-6   (LED indices 3-5)");
    Serial.println("  Route 3 (A14 Westbound)      -> NeoPixels 7-9   (LED indices 6-8)");
    Serial.println("  Route 4 (A43)                -> NeoPixels 10-12 (LED indices 9-11)");
    Serial.println("  Route 5 (A45)                -> NeoPixels 13-15 (LED indices 12-14)");
    Serial.println();
}

void checkAllRoutes() {
    Serial.println("\n========================================");
    Serial.println("Traffic Status Update");
    Serial.println("========================================");

    for (size_t i = 0; i < routes.size(); i++) {
        const auto& route = routes[i];
        int duration;
        
        TrafficLevel level = checkTrafficLevelWithDuration(
            route.origin,
            route.destination,
            route.intermediates,
            route.normalLimitSec,
            route.heavyLimitSec,
            duration
        );

        // Update LEDs for this route
        uint32_t color = trafficToColor(level);
        setRouteLEDs(i, color);
        strip.show();

        // Print status
        Serial.println();
        Serial.print(route.name);
        int ledStart = i * 3 + 1;  // Convert to 1-indexed NeoPixel numbers
        Serial.print(" [NeoPixels ");
        Serial.print(ledStart);
        Serial.print("-");
        Serial.print(ledStart + 2);
        Serial.println("]");
        
        Serial.print("  Status: ");
        Serial.print(trafficToString(level));
        
        if (duration > 0) {
            Serial.print(" (");
            Serial.print(formatDuration(duration));
            Serial.print(")");
        }
        Serial.println();

        // Small delay between API calls to avoid rate limiting
        delay(200);
    }
    
    Serial.println("========================================\n");
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Initialize NeoPixels
    strip.begin();
    strip.setBrightness(50);
    strip.show();

    Serial.println("\n=== Traffic Map NeoPixel Display ===");
    Serial.println("15 NeoPixels: 3 LEDs per route");
    
    setupRoutes();
    displayStartupSequence();
    connectWiFi();
    
    Serial.println("\nMonitoring 5 routes...");
    Serial.println("Colors: GREEN = Normal, YELLOW = Moderate, RED = Heavy, BLUE = Error\n");
    
    // Do first check immediately on startup
    checkAllRoutes();
    lastRun = millis();  // Reset the timer after first check
}

void loop() {
    unsigned long now = millis();

    if (now - lastRun >= INTERVAL_MS) {
        lastRun = now;
        checkAllRoutes();
    }

    // other tasks can run here
}