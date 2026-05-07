#include "network_manager.h"
#include "led_controller.h"
#include "secrets.h"
#include "config.h"
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <time.h>

// Telnet server
static WiFiServer telnetServer(TELNET_PORT);
static WiFiClient telnetClient;

// Rate limiting variables
static unsigned long lastUpdateTime = 0;
static unsigned long updatesThisHour = 0;
static unsigned long hourStartTime = 0;

// ============================================
// Helper Functions
// ============================================

void telnetPrint(const String& message) {
    Serial.print(message);
    if (telnetClient && telnetClient.connected()) {
        telnetClient.print(message);
    }
}

void telnetPrintln(const String& message) {
    Serial.println(message);
    if (telnetClient && telnetClient.connected()) {
        telnetClient.println(message);
    }
}

// ============================================
// WiFi Setup
// ============================================

bool networkInit() {
    telnetPrintln("\nConnecting to WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        telnetPrint(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        telnetPrintln("\nWiFi connected!");
        telnetPrint("IP address: ");
        telnetPrintln(WiFi.localIP().toString());
        return true;
    } else {
        telnetPrintln("\nWiFi connection failed!");
        return false;
    }
}

// ============================================
// Time Management
// ============================================

void networkSetupTime() {
    telnetPrintln("Setting up time synchronization...");
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
    
    struct tm timeinfo;
    int retries = 0;
    while (!getLocalTime(&timeinfo) && retries < 10) {
        telnetPrint(".");
        delay(1000);
        retries++;
    }
    
    if (retries < 10) {
        telnetPrintln("\nTime synchronized!");
        char timeString[64];
        strftime(timeString, sizeof(timeString), "Current time: %A, %B %d %Y %H:%M:%S", &timeinfo);
        telnetPrintln(timeString);
    } else {
        telnetPrintln("\nFailed to get time - will retry later");
    }
}

bool isWithinActiveHours() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        telnetPrintln("Warning: Failed to get time, assuming active hours");
        return true;
    }
    
    int hour = timeinfo.tm_hour;
    return (hour >= ACTIVE_START_HOUR && hour < ACTIVE_END_HOUR);
}

unsigned long getTimeUntilActiveHours() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return 60UL * 60UL * 1000UL;  // Default 1 hour
    }
    
    int currentHour = timeinfo.tm_hour;
    int currentMinute = timeinfo.tm_min;
    
    // Past active hours (after 11 PM)
    if (currentHour >= ACTIVE_END_HOUR) {
        int hoursUntil = (24 - currentHour) + ACTIVE_START_HOUR;
        int minutesUntil = hoursUntil * 60 - currentMinute;
        return minutesUntil * 60UL * 1000UL;
    }
    
    // Before active hours (before 7 AM)
    if (currentHour < ACTIVE_START_HOUR) {
        int hoursUntil = ACTIVE_START_HOUR - currentHour;
        int minutesUntil = hoursUntil * 60 - currentMinute;
        return minutesUntil * 60UL * 1000UL;
    }
    
    return 0;  // Within active hours
}

// ============================================
// Rate Limiting
// ============================================

bool rateLimitCheck() {
    unsigned long now = millis();
    
    // Reset counter every hour
    if (now - hourStartTime >= 60UL * 60UL * 1000UL) {
        updatesThisHour = 0;
        hourStartTime = now;
        telnetPrintln("Rate limit counter reset for new hour");
    }
    
    // Check max updates per hour
    if (updatesThisHour >= MAX_UPDATES_PER_HOUR) {
        telnetPrintln("Rate limit reached - waiting for next hour");
        return false;
    }
    
    // Check minimum interval
    if (now - lastUpdateTime < MIN_UPDATE_INTERVAL_MS) {
        telnetPrintln("Too soon since last update - rate limiting");
        return false;
    }
    
    return true;
}

void recordUpdate() {
    lastUpdateTime = millis();
    updatesThisHour++;
    
    telnetPrint("Updates this hour: ");
    telnetPrint(String(updatesThisHour));
    telnetPrint("/");
    telnetPrintln(String(MAX_UPDATES_PER_HOUR));
}

// ============================================
// OTA Updates
// ============================================

void networkSetupOTA() {
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    
    #ifdef OTA_PASSWORD
    ArduinoOTA.setPassword(OTA_PASSWORD);
    #endif
    
    ArduinoOTA.onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        telnetPrintln("Starting OTA update: " + type);
        ledSetAll(TRAFFIC_ERROR);  // Blue during update
    });
    
    ArduinoOTA.onEnd([]() {
        telnetPrintln("\nOTA Update complete!");
        ledShowOTASuccess();
    });
    
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        unsigned int percentage = (progress / (total / 100));
        Serial.printf("OTA Progress: %u%%\r", percentage);
        ledShowOTAProgress(percentage);
    });
    
    ArduinoOTA.onError([](ota_error_t error) {
        telnetPrint("OTA Error: ");
        if (error == OTA_AUTH_ERROR) telnetPrintln("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) telnetPrintln("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) telnetPrintln("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) telnetPrintln("Receive Failed");
        else if (error == OTA_END_ERROR) telnetPrintln("End Failed");
        ledShowOTAError();
    });
    
    ArduinoOTA.begin();
    telnetPrintln("OTA Ready");
}

// ============================================
// Telnet Server
// ============================================

void networkSetupTelnet() {
    telnetServer.begin();
    telnetServer.setNoDelay(true);
    telnetPrintln("Telnet server started on port " + String(TELNET_PORT));
    telnetPrint("Connect with: telnet ");
    telnetPrintln(WiFi.localIP().toString());
}

static void handleTelnet() {
    if (telnetServer.hasClient()) {
        if (telnetClient && telnetClient.connected()) {
            telnetClient.stop();
        }
        telnetClient = telnetServer.available();
        telnetClient.println("\n=== Traffic Map Telnet Console ===");
        telnetClient.print("Connected at: ");
        telnetClient.println(WiFi.localIP());
        telnetClient.println("===================================\n");
        telnetClient.flush();
    }
}

// ============================================
// Network Handler
// ============================================

void networkHandle() {
    ArduinoOTA.handle();
    handleTelnet();
}