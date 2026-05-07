#include "config.h"
#include "secrets.h"
#include "network_manager.h"
#include "led_controller.h"
#include "traffic_api.h"

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

void checkAllRoutes() {
    telnetPrintln("\n========================================");
    telnetPrintln("Traffic Status Update");
    
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        char timeString[64];
        strftime(timeString, sizeof(timeString), "%H:%M:%S", &timeinfo);
        telnetPrint("Time: ");
        telnetPrintln(timeString);
    }
    
    telnetPrintln("========================================");

    for (size_t i = 0; i < routes.size(); i++) {
        const auto& route = routes[i];
        int duration;
        
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
        telnetPrintln("");
        telnetPrint(route.name);
        int ledStart = i * 3 + 1;
        telnetPrint(" [LEDs ");
        telnetPrint(String(ledStart));
        telnetPrint("-");
        telnetPrint(String(ledStart + 2));
        telnetPrintln("]");
        
        telnetPrint("  Status: ");
        telnetPrint(trafficLevelToString(level));
        
        if (duration > 0) {
            telnetPrint(" (");
            telnetPrint(formatDuration(duration));
            telnetPrint(")");
        }
        telnetPrintln("");

        // Small delay between API calls
        delay(200);
    }
    
    telnetPrintln("========================================\n");
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
    if (!networkInit()) {
        Serial.println("Cannot proceed without WiFi. Restarting...");
        delay(5000);
        ESP.restart();
    }
    
    // Setup time synchronization
    networkSetupTime();
    
    // Setup OTA updates
    networkSetupOTA();
    
    // Setup Telnet
    networkSetupTelnet();
    
    // Initialize traffic API
    trafficApiInit();
    
    // Load routes configuration
    routes = createRoutes();
    
    // Show startup sequence
    ledStartupSequence();
    
    telnetPrintln("\n=================================");
    telnetPrintln("Setup complete!");
    telnetPrintln("Active hours: " + String(ACTIVE_START_HOUR) + " AM - " + String(ACTIVE_END_HOUR) + " PM");
    telnetPrint("Update interval: ");
    telnetPrint(String(UPDATE_INTERVAL_MS / 60000));
    telnetPrintln(" minutes");
    telnetPrintln("=================================\n");
    
    // Do first update if within active hours
    if (isWithinActiveHours()) {
        checkAllRoutes();
        recordUpdate();
    } else {
        telnetPrintln("Outside active hours - LEDs off");
        ledClear();
    }
}

// ============================================
// Main Loop
// ============================================

void loop() {
    // Handle network tasks (OTA, Telnet) - MUST BE FIRST
    networkHandle();
    
    // Check if we're in active hours
    if (!isWithinActiveHours()) {
        ledClear();
        unsigned long sleepTime = getTimeUntilActiveHours();
        // Sleep but wake periodically to handle OTA
        unsigned long sleepChunk = min(sleepTime, 60000UL);  // Max 1 minute chunks
        delay(sleepChunk);
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