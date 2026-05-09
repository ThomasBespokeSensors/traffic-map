#include "rate_limiter.h"
#include "config.h"

static unsigned long lastUpdateTime = 0;
static unsigned long updatesThisHour = 0;
static unsigned long hourStartTime = 0;

bool rateLimitCheck() {
    unsigned long now = millis();
    
    // Reset counter every hour
    if (now - hourStartTime >= 60UL * 60UL * 1000UL) {
        updatesThisHour = 0;
        hourStartTime = now;
        Serial.println("Rate limit counter reset for new hour");
    }
    
    // Check max updates per hour
    if (updatesThisHour >= MAX_UPDATES_PER_HOUR) {
        Serial.println("Rate limit reached - waiting for next hour");
        return false;
    }
    
    // Check minimum interval
    if (now - lastUpdateTime < MIN_UPDATE_INTERVAL_MS) {
        Serial.println("Too soon since last update - rate limiting");
        return false;
    }
    
    return true;
}

void recordUpdate() {
    lastUpdateTime = millis();
    updatesThisHour++;
    
    Serial.print("Updates this hour: ");
    Serial.print(updatesThisHour);
    Serial.print("/");
    Serial.println(MAX_UPDATES_PER_HOUR);
}