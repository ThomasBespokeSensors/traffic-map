#include "traffic.h"
#include "secrets.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

TrafficLevel checkTrafficLevelWithDuration(
    const LatLng& origin,
    const LatLng& destination,
    const std::vector<LatLng>& intermediates,
    int lowerTimeSec,
    int upperTimeSec,
    int& outDurationSec
) {
    outDurationSec = -1;

    if (WiFi.status() != WL_CONNECTED) {
        return TRAFFIC_ERROR;
    }

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    const char* url =
        "https://routes.googleapis.com/directions/v2:computeRoutes";

    if (!http.begin(client, url)) {
        return TRAFFIC_ERROR;
    }

    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Goog-Api-Key", GOOGLE_API_KEY);
    http.addHeader(
        "X-Goog-FieldMask",
        "routes.duration,routes.distanceMeters"
    );

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

    int httpCode = http.POST(body);

    if (httpCode != HTTP_CODE_OK) {
        http.end();
        return TRAFFIC_ERROR;
    }

    String response = http.getString();
    http.end();

    StaticJsonDocument<2048> resp;
    DeserializationError err = deserializeJson(resp, response);

    if (err) {
        return TRAFFIC_ERROR;
    }

    if (!resp.containsKey("routes") || resp["routes"].size() == 0) {
        return TRAFFIC_ERROR;
    }

    const char* durationStr = resp["routes"][0]["duration"];

    if (!durationStr) {
        return TRAFFIC_ERROR;
    }

    String durStr = String(durationStr);
    durStr.replace("s", "");
    int trafficTimeSec = durStr.toInt();
    
    outDurationSec = trafficTimeSec;

    if (trafficTimeSec < lowerTimeSec)
        return TRAFFIC_NORMAL;
    else if (trafficTimeSec < upperTimeSec)
        return TRAFFIC_MODERATE;
    else
        return TRAFFIC_HEAVY;
}

TrafficLevel checkTrafficLevel(
    const LatLng& origin,
    const LatLng& destination,
    const std::vector<LatLng>& intermediates,
    int lowerTimeSec,
    int upperTimeSec
) {
    int duration;
    return checkTrafficLevelWithDuration(origin, destination, intermediates, 
                                         lowerTimeSec, upperTimeSec, duration);
}