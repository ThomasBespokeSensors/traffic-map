#include "ota_updater.h"
#include "config.h"
#include "logger.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>


String getFirmwareVersion() {
    return String(FIRMWARE_VERSION);
}

bool checkForUpdate() {
    logPrintln("Checking for firmware update...");
    
    HTTPClient http;
    String url = "http://" + String(RPI_SERVER_IP) + ":" + String(RPI_SERVER_PORT) + "/firmware/" + String(FIRMWARE_FILENAME);
    
    logPrintln("Update URL: " + url);
    
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        logPrintln("No firmware available (HTTP " + String(httpCode) + ")");
        http.end();
        return false;
    }
    
    int contentLength = http.getSize();
    if (contentLength <= 0) {
        logPrintln("Invalid firmware size");
        http.end();
        return false;
    }
    
    logPrintln("Firmware size: " + String(contentLength) + " bytes");
    logPrintln("Starting OTA update...");
    
    bool canBegin = Update.begin(contentLength);
    if (!canBegin) {
        logPrintln("ERROR: Not enough space for update");
        http.end();
        return false;
    }
    
    WiFiClient* stream = http.getStreamPtr();
    size_t written = Update.writeStream(*stream);
    
    if (written == contentLength) {
        logPrintln("Written: " + String(written) + " bytes successfully");
    } else {
        logPrintln("ERROR: Written only " + String(written) + "/" + String(contentLength) + " bytes");
        http.end();
        return false;
    }
    
    if (Update.end()) {
        if (Update.isFinished()) {
            logPrintln("✅ OTA update completed successfully!");
            logPrintln("Rebooting in 3 seconds...");
            http.end();
            delay(3000);
            ESP.restart();
            return true;
        } else {
            logPrintln("ERROR: Update not finished");
        }
    } else {
        logPrintln("ERROR: Update failed - Error #" + String(Update.getError()));
    }
    
    http.end();
    return false;
}