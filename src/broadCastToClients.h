/*
  # Author : Watchara Pongsri
  # [github/X-c0d3] https://github.com/X-c0d3/
  # Web Site: https://www.rockdevper.com
*/

#ifndef BROADCAST_TO_CLIENTS_H
#define BROADCAST_TO_CLIENTS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

#include "settings.h"
#include "utility.h"
#include "wifiMan.h"

String buildDeviceStatePayload(InverterData data) {
    // https://arduinojson.org/v6/assistant/
    StaticJsonDocument<1024> root;
    root["deviceName"] = DEVICE_NAME;
    root["deviceId"] = getChipId();
    root["lastUpdated"] = DateNowString();
    root["ipAddress"] = WiFi.localIP().toString();
    root["wifiSignal"] = wifiSignal();
    root["firmware_version"] = String(FIRMWARE_VERSION);
    root["firmware_lastupdate"] = String(FIRMWARE_LASTUPDATE);

    JsonObject deviceState = root.createNestedObject("deviceState");
    data.toJson(deviceState);

    String output;
    serializeJsonPretty(root, output);
    return output;
}

void publishToMqtt(PubSubClient& mqttClient, InverterData data) {
    unsigned long startTime = micros();
    String output = buildDeviceStatePayload(data);

    if (ENABLE_MQTT && mqttClient.connected()) {
        String topic = String(MQTT_TOPIC_BASE) + "/state";
        mqttClient.publish(topic.c_str(), output.c_str());
        unsigned long elapsedTime = micros() - startTime;
        Serial.print(">>> MQTT Publish ElapsedTime: ");
        Serial.println(formatDuration(elapsedTime));
    }

    if (ENABLE_DEBUG_MODE)
        Serial.print(output);
}

#endif