/*
  # Author : Watchara Pongsri
  # [github/X-c0d3] https://github.com/X-c0d3/
  # Web Site: https://www.rockdevper.com
*/

#ifndef WIFIMAN_H
#define WIFIMAN_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <WiFiClientSecureAxTLS.h>

#include "settings.h"

bool shouldSaveConfig = true;
void saveConfigCallback() {
    Serial.println("Should save config");
    shouldSaveConfig = true;
}

char socketio_server[40];
char socketio_port[6];
char line_api_key[60];
char firebase_host[60];
char firebase_api_key[60];

void wifiReset() {
    Serial.println("Erase settings and restart ...");
    delay(1000);

    WiFi.disconnect(true);  // erases store credentially
    // wifiManager.resetSettings();
    SPIFFS.format();  // erases stored values
    ESP.restart();
}

void setup_Wifi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    if (WiFi.getMode() & WIFI_AP) {
        WiFi.softAPdisconnect(true);
    }

    Serial.println();
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    // setup_IpAddress();

    Serial.println();
    Serial.print("WIFI Connected ");
    String ip = WiFi.localIP().toString();
    Serial.println(ip.c_str());
}

long rssi;
long wifiSignal() {
    // WifiSignalStrength
    // Convert to scale -48 to 0 eg. map(rssi, -100, 0, 0, -48);
    // I used -100 instead of -120 because <= -95 is unusable
    // Negative number so we can draw n pixels from the bottom in black
    rssi = WiFi.RSSI();  // eg. -63
    if (rssi < -99) {
        rssi = -99;
    }
    // Serial.println("WifiSignal: " + String(rssi) + "db");
    return rssi;
}

int digits(int x) {
    return ((bool)x * (int)log10(abs(x)) + 1);
}

#endif