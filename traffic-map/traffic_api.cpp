#include "traffic_api.h"
#include "secrets.h"
#include "network_manager.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

void trafficApiInit() {
    // Currently nothing to initialize, but kept for future expansion
}

TrafficLevel checkTrafficLevel(
    const LatLng& origin,
    const LatLng& destination,
    const std::vector<LatLng>& intermediates,
    int lowerTimeSec,
    int upperTimeSec,
    int& outDurationSec
) {
    outDurationSec = -1;
    
    if (WiFi.status() != WL_CONNECTED) {
        telnetPrintln("Error: WiFi not connected");
        return TRAFFIC_ERROR;
    }

    WiFiClientSecure client;
    client.setInsecure();
    
    HTTPClient http;
    const char* url = "https://routes.googleapis.com/directions/v2:computeRoutes";
    
    if (!http.begin(client, url)) {
        telnetPrintln("Error: Failed to begin HTTP connection");
        return TRAFFIC_ERROR;
    }

    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Goog-Api-Key", GOOGLE_API_KEY);
    http.addHeader("X-Goog-FieldMask", "routes.duration,routes.distanceMeters");

    // Build JSON request
    StaticJsonDocument<1024> doc;
    doc["origin"]["location"]["latLng"]["latitude"]  = origin.lat;
    doc["origin"]["location"]["latLng"]["longitude"] = origin.lng;
    doc["destination"]["location"]["latLng"]["latitude"]  = destination.lat;
    doc["destination"]["location"]["latLng"]["longitude"] = destination.lng;

    JsonArray viasArray = doc.createNestedArray("intermediates");
    for (const auto& p : intermediates) {
        JsonObject via = viasArray.createNestedObject();
        via["location"]["latLng"]["latitude"]  = p.lat;
        via["location"]["latLng"]["longitude"] = p.lng;
    }

    doc["travelMode"] = "DRIVE";
    doc["routingPreference"] = "TRAFFIC_AWARE";

    String body;
    serializeJson(doc, body);

    // Make request
    int httpCode = http.POST(body);

    if (httpCode != HTTP_CODE_OK) {
        telnetPrint("HTTP Error: ");
        telnetPrintln(String(httpCode));
        http.end();
        return TRAFFIC_ERROR;
    }

    String response = http.getString();
    http.end();

    // Parse response
    StaticJsonDocument<2048> resp;
    DeserializationError err = deserializeJson(resp, response);

    if (err) {
        telnetPrint("JSON parse error: ");
        telnetPrintln(String(err.c_str()));
        return TRAFFIC_ERROR;
    }

    if (!resp.containsKey("routes") || resp["routes"].size() == 0) {
        telnetPrintln("Error: No routes in response");
        return TRAFFIC_ERROR;
    }

    const char* durationStr = resp["routes"][0]["duration"];
    if (!durationStr) {
        telnetPrintln("Error: No duration in response");
        return TRAFFIC_ERROR;
    }

    // Parse duration (format: "XXXs")
    String durStr = String(durationStr);
    durStr.replace("s", "");
    int trafficTimeSec = durStr.toInt();
    
    outDurationSec = trafficTimeSec;

    // Determine traffic level
    if (trafficTimeSec < lowerTimeSec)
        return TRAFFIC_NORMAL;
    else if (trafficTimeSec < upperTimeSec)
        return TRAFFIC_MODERATE;
    else
        return TRAFFIC_HEAVY;
}

const char* trafficLevelToString(TrafficLevel level) {
    switch (level) {
        case TRAFFIC_NORMAL:   return "NORMAL";
        case TRAFFIC_MODERATE: return "MODERATE";
        case TRAFFIC_HEAVY:    return "HEAVY";
        case TRAFFIC_ERROR:    return "ERROR";
        default:               return "UNKNOWN";
    }
}