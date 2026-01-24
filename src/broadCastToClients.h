/*
  # Author : Watchara Pongsri
  # [github/X-c0d3] https://github.com/X-c0d3/
  # Web Site: https://www.rockdevper.com
*/

#ifndef BROADCAST_TO_CLIENTS_H
#define BROADCAST_TO_CLIENTS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SocketIoClient.h>

#include "lineNotify.h"
#include "settings.h"
#include "utility.h"

void publishToSocketIO(SocketIoClient& webSocket, InverterData data) {
    // https://arduinojson.org/v6/assistant/
    StaticJsonDocument<1024> root;
    root["deviceName"] = DEVICE_NAME;
    root["deviceId"] = getChipId();
    root["lastUpdated"] = NowString();
    root["ipAddress"] = WiFi.localIP().toString();

    JsonObject deviceState = root.createNestedObject("deviceState");
    data.toJson(deviceState);

    String output;
    serializeJsonPretty(root, output);

    // Publish to socket.io server
    if (ENABLE_SOCKETIO)
        webSocket.emit(SOCKETIO_CHANNEL, output.c_str());

    if (ENABLE_DEBUG_MODE)
        Serial.print(output);
}

#endif