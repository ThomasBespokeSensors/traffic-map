#include "time_manager.h"
#include "config.h"
#include <time.h>
#include "logger.h"


void timeInit() {
    logPrintln("Setting up time synchronization...");
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
    
    struct tm timeinfo;
    int retries = 0;
    while (!getLocalTime(&timeinfo) && retries < 10) {
        logPrint(".");
        delay(1000);
        retries++;
    }
    
    if (retries < 10) {
        logPrintln("\nTime synchronized!");
        char timeString[64];
        strftime(timeString, sizeof(timeString), "Current time: %A, %B %d %Y %H:%M:%S", &timeinfo);
        logPrintln(timeString);
    } else {
        logPrintln("\nFailed to get time - will retry later");
    }
}

bool isWithinActiveHours() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        logPrintln("Warning: Failed to get time, assuming active hours");
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
        logPrint("Sleeping until 7 AM (");
        logPrint(minutesUntil);
        logPrintln(" minutes)");
        return minutesUntil * 60UL * 1000UL;
    }
    
    // Before active hours (before 7 AM)
    if (currentHour < ACTIVE_START_HOUR) {
        int hoursUntil = ACTIVE_START_HOUR - currentHour;
        int minutesUntil = hoursUntil * 60 - currentMinute;
        logPrint("Sleeping until 7 AM (");
        logPrint(minutesUntil);
        logPrintln(" minutes)");
        return minutesUntil * 60UL * 1000UL;
    }
    
    return 0;  // Within active hours
}

bool getCurrentTimeString(char* buffer, size_t bufferSize) {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return false;
    }
    
    strftime(buffer, bufferSize, "%H:%M:%S", &timeinfo);
    return true;
}