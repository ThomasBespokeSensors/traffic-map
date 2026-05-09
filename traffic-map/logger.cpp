#include "logger.h"
#include "config.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

static void sendLogToServer(const String& message, bool newline);

void loggerInit() {
    logPrintln("Logger initialized");
    logPrint("Sending logs to: http://");
    logPrint(RPI_SERVER_IP);
    logPrint(":");
    logPrint(RPI_SERVER_PORT);
    logPrintln("/log");
}

void logPrint(const String& message) {
    Serial.print(message);
    sendLogToServer(message, false);
}

void logPrintln(const String& message) {
    Serial.println(message);
    sendLogToServer(message, true);
}

static void sendLogToServer(const String& message, bool newline) {

    // Optional:
    // Only send println messages to avoid spam
    if (!newline) {
        return;
    }

    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    HTTPClient http;

    String url =
        "http://" +
        String(RPI_SERVER_IP) +
        ":" +
        String(RPI_SERVER_PORT) +
        "/log";

    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(2000);

    StaticJsonDocument<256> doc;
    doc["message"] = message;

    String body;
    serializeJson(doc, body);

    int httpCode = http.POST(body);

    // silently ignore failures
    (void)httpCode;

    http.end();
}

void logPrint(int value) {
    logPrint(String(value));
}

void logPrintln(int value) {
    logPrintln(String(value));
}

void logPrint(unsigned int value) {
    logPrint(String(value));
}

void logPrintln(unsigned int value) {
    logPrintln(String(value));
}

void logPrint(long value) {
    logPrint(String(value));
}

void logPrintln(long value) {
    logPrintln(String(value));
}

void logPrint(unsigned long value) {
    logPrint(String(value));
}

void logPrintln(unsigned long value) {
    logPrintln(String(value));
}

void logPrint(float value) {
    logPrint(String(value));
}

void logPrintln(float value) {
    logPrintln(String(value));
}

void logPrint(double value) {
    logPrint(String(value));
}

void logPrintln(double value) {
    logPrintln(String(value));
}

void logPrint() {
    // do nothing
}

void logPrintln() {
}