#include <WiFi.h>
#include "config.h"
#include "secrets.h"
#include "led_controller.h"
#include "traffic_api.h"
#include "time_manager.h"
#include "rate_limiter.h"

// Global state
std::vector<RouteConfig> routes;
unsigned long lastUpdate = 0;

// ============================================
// Helper Functions
// ============================================

String formatDuration(int seconds) {
    int mins = seconds / 60;
    int secs = seconds % 60;
    return String(mins) + "m " + String(secs) + "s";
}

void connectWiFi() {
    Serial.print("Connecting to WiFi");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nWiFi connection failed!");
    }
}

void checkAllRoutes() {
    Serial.println("\n========================================");
    Serial.println("Traffic Status Update");
    
    char timeString[32];
    if (getCurrentTimeString(timeString, sizeof(timeString))) {
        Serial.print("Time: ");
        Serial.println(timeString);
    }
    
    Serial.println("========================================");

    for (size_t i = 0; i < routes.size(); i++) {
        const auto& route = routes[i];
        int duration;
        
        Serial.print("Checking: ");
        Serial.println(route.name);
        
        TrafficLevel level = checkTrafficLevel(
            route.origin,
            route.destination,
            route.intermediates,
            route.normalLimitSec,
            route.heavyLimitSec,
            duration
        );

        // Update LEDs for this route
        ledSetRoute(i, level);

        // Print status
        int ledStart = i * 3;
        Serial.print("  LEDs: ");
        Serial.print(ledStart);
        Serial.print("-");
        Serial.print(ledStart + 2);
        Serial.print(" | Status: ");
        Serial.print(trafficLevelToString(level));
        
        if (duration > 0) {
            Serial.print(" (");
            Serial.print(formatDuration(duration));
            Serial.print(")");
        }
        Serial.println();

        // Small delay between API calls
        delay(200);
    }
    
    Serial.println("========================================\n");
}

// ============================================
// Setup
// ============================================

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n=================================");
    Serial.println("Traffic Map Display Starting...");
    Serial.println("=================================\n");
    
    // Initialize LED strip
    ledInit();
    
    // Connect to WiFi
    connectWiFi();
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Cannot proceed without WiFi. Restarting in 5 seconds...");
        delay(5000);
        ESP.restart();
    }
    
    // Setup time synchronization
    timeInit();
    
    // Load routes configuration
    routes = createRoutes();
    
    // Show startup sequence
    ledStartupSequence();
    
    Serial.println("\n=================================");
    Serial.println("Setup complete!");
    Serial.println("Active hours: " + String(ACTIVE_START_HOUR) + " AM - " + String(ACTIVE_END_HOUR) + " PM");
    Serial.print("Update interval: ");
    Serial.print(UPDATE_INTERVAL_MS / 60000);
    Serial.println(" minutes");
    Serial.println("=================================\n");
    
    // Do first update if within active hours
    if (isWithinActiveHours()) {
        checkAllRoutes();
        recordUpdate();
    } else {
        Serial.println("Outside active hours - LEDs off");
        ledClear();
    }
}

// ============================================
// Main Loop
// ============================================

void loop() {
    // Check if we're in active hours
    if (!isWithinActiveHours()) {
        ledClear();
        // Sleep but check hourly in case time was wrong
        delay(60UL * 60UL * 1000UL);  // 1 hour
        return;
    }
    
    // Check if it's time to update
    unsigned long currentTime = millis();
    if (currentTime - lastUpdate >= UPDATE_INTERVAL_MS) {
        if (rateLimitCheck()) {
            checkAllRoutes();
            recordUpdate();
            lastUpdate = currentTime;
        }
    }
    
    // Small delay to prevent watchdog issues
    delay(100);
}