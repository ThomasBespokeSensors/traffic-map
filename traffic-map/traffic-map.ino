#include <WiFi.h>
#include "config.h"
#include "secrets.h"
#include "led_controller.h"
#include "traffic_api.h"
#include "time_manager.h"
#include "rate_limiter.h"
#include "logger.h"
#include "ota_updater.h"

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
    logPrint("Connecting to WiFi");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        logPrint(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        logPrintln("\nWiFi connected!");
        logPrint("IP address: ");
        logPrintln(WiFi.localIP());
    } else {
        logPrintln("\nWiFi connection failed!");
    }
}

void checkAllRoutes() {
    logPrintln("\n========================================");
    logPrintln("Traffic Status Update");
    
    char timeString[32];
    if (getCurrentTimeString(timeString, sizeof(timeString))) {
        logPrint("Time: ");
        logPrintln(timeString);
    }
    
    logPrintln("========================================");

    for (size_t i = 0; i < routes.size(); i++) {
        const auto& route = routes[i];
        int duration;
        
        logPrint("Checking: ");
        logPrintln(route.name);
        
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
        logPrint("  LEDs: ");
        logPrint(ledStart);
        logPrint("-");
        logPrint(ledStart + 2);
        logPrint(" | Status: ");
        logPrint(trafficLevelToString(level));
        
        if (duration > 0) {
            logPrint(" (");
            logPrint(formatDuration(duration));
            logPrint(")");
        }
        logPrintln();

        // Small delay between API calls
        delay(200);
    }
    
    logPrintln("========================================\n");
}

// ============================================
// Setup
// ============================================

void setup() {
    Serial.begin(115200);
    delay(1000);

    loggerInit();
    
    logPrintln("\n\n=================================");
    logPrintln("Traffic Map Display Starting...");
    logPrintln("=================================\n");
    
    // Initialize LED strip
    ledInit();
    
    // Connect to WiFi
    connectWiFi();
    
    if (WiFi.status() != WL_CONNECTED) {
        logPrintln("Cannot proceed without WiFi. Restarting in 5 seconds...");
        delay(5000);
        ESP.restart();
    }
    
    // Setup time synchronization
    timeInit();
    
    // Load routes configuration
    routes = createRoutes();
    
    // Show startup sequence
    ledStartupSequence();
    
    logPrintln("\n=================================");
    logPrintln("Setup complete!");
    logPrintln("Active hours: " + String(ACTIVE_START_HOUR) + " AM - " + String(ACTIVE_END_HOUR) + " PM");
    logPrint("Update interval: ");
    logPrint(UPDATE_INTERVAL_MS / 60000);
    logPrintln(" minutes");
    logPrintln("=================================\n");
    
    // Do first update if within active hours
    if (isWithinActiveHours()) {
        checkAllRoutes();
        recordUpdate();
    } else {
        logPrintln("Outside active hours - LEDs off");
        ledClear();
    }
}

// ============================================
// Main Loop
// ============================================

void loop() {
    // Check for OTA updates once per day at startup or every 24 hours
    static unsigned long lastOTACheck = 0;
    static bool firstRun = true;
    
    unsigned long now = millis();
    
    // Check on first run (after 30 seconds to ensure stable connection)
    if (firstRun && now > 30000) {
        firstRun = false;
        checkForUpdate();
        lastOTACheck = now;
    }
    
    // Check every 24 hours
    if (now - lastOTACheck >= 24UL * 60UL * 60UL * 1000UL) {
        lastOTACheck = now;
        checkForUpdate();
    }

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