/*
  # Author : Watchara Pongsri
  # [github/X-c0d3] https://github.com/X-c0d3/
  # Web Site: https://www.rockdevper.com
*/

#include <HardwareSerial.h>
#include <ModbusMaster.h>
#include <SPIFFS.h>
#include <SocketIoClient.h>
#include <TJpg_Decoder.h>
#include <arduino-timer.h>

#include "HuaweiSun2000Client.h"
#include "broadCastToClients.h"
#include "esp_task_wdt.h"
#include "utility.h"
#include "wifiMan.h"
#define TX_PIN 21
#define RX_PIN 20
#define ESP32C3_LED 8  // Built-in LED

// Config RS485 adapter (Reader)
#define SLAVE_ID 1
#define BAUD_RATE 9600
#define WDT_TIMEOUT 30

TFT_eSPI tft = TFT_eSPI();
HardwareSerial hwSerial(1);
HuaweiSun2000Client inverter(hwSerial, SLAVE_ID, BAUD_RATE);
SocketIoClient webSocket;

auto timer = timer_create_default();  // create a timer with default settings
bool screenOn = true;
unsigned long lastTouchTime = 0;
const unsigned long timeout = 3 * 60 * 1000;  // 3min  Screen Sleep (Power Saving)

String mainLogo = "/main-background1.jpg";

// callback for drwaing JPG into TFT_eSPI
bool jpgRender(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    tft.pushImage(x, y, w, h, bitmap);
    return true;
}

void event(const char* payload, size_t length) {
    // Serial.println("Socket.io Event Received:");
    // Serial.println(String(payload).substring(0, length));
}

uint16_t hexToRGB565(const char* hex) {
    uint32_t rgb = strtoul(hex + 1, NULL, 16);
    uint8_t r = (rgb >> 16) & 0xFF;
    uint8_t g = (rgb >> 8) & 0xFF;
    uint8_t b = rgb & 0xFF;

    return tft.color565(r, g, b);
}

void drawDashboard(InverterData obj) {
    tft.setTextSize(2);
    tft.setTextColor(TFT_YELLOW);
    // PV Power
    drawFixedText(tft, 60, 20, 100, 18, formatPower(obj.pv_power * 1000, 3), TFT_BLUE, hexToRGB565("#b7d3f8"), 1);
    // Home Power (Load)
    drawFixedText(tft, 280, 20, 100, 18, formatPower((obj.activePower + obj.grid_power) * 1000, 3), TFT_RED, hexToRGB565("#b7d3f8"), 1);
    // Grid Power
    drawFixedText(tft, 260, 300, 100, 18, formatPower(obj.grid_power * 1000, 3), TFT_BLACK, hexToRGB565("#e1e2e6"), 1);

    tft.setTextSize(1);
    if (obj.pv1_voltage > 0) {
        tft.setTextColor(TFT_GREENYELLOW);
        tft.setCursor(10, 50);
        tft.print("PV1");
        drawFixedText(tft, 10, 60, 135, 10,
                      String(obj.pv1_voltage, 0) + "V / " + String(obj.pv1_current, 1) + "A / ~" + String(obj.pv1_voltage * obj.pv1_current, 0) + " W",
                      TFT_PINK, hexToRGB565("#2a3342"), 1, ML_DATUM);
    }

    if (obj.pv2_voltage > 0) {
        tft.setTextColor(TFT_GREENYELLOW);
        tft.setCursor(10, 75);
        tft.print("PV2");
        drawFixedText(tft, 10, 85, 135, 10,
                      String(obj.pv2_voltage, 0) + "V / " + String(obj.pv2_current, 1) + "A / ~" + String(obj.pv2_voltage * obj.pv2_current, 0) + " W",
                      TFT_PINK, hexToRGB565("#2a3342"), 1, ML_DATUM);
    }

    if (obj.pv3_voltage > 0) {
        tft.setTextColor(TFT_GREENYELLOW);
        tft.setCursor(10, 100);
        tft.print("PV3");
        drawFixedText(tft, 10, 110, 135, 10,
                      String(obj.pv3_voltage, 0) + "V / " + String(obj.pv3_current, 1) + "A / ~" + String(obj.pv3_voltage * obj.pv3_current, 0) + " W",
                      TFT_PINK, hexToRGB565("#2a3342"), 1, ML_DATUM);
    }

    tft.setTextColor(TFT_BLACK);
    tft.drawString(obj.model.c_str(), 138, 170);
    tft.drawString("v" + String(VERSION), 440, 10);

    tft.setTextColor(TFT_GREENYELLOW);
    tft.setCursor(165, 220);
    tft.print("Temp");
    drawFixedText(tft, 165, 230, 30, 10, String(obj.temperature, 0) + " C\xF8", TFT_BLACK, hexToRGB565("#b8b9bb"), 1);

    drawFixedText(tft, 350, 70, 130, 10, obj.status.c_str(), TFT_BLACK, hexToRGB565("#c8ddf8"), 1, MR_DATUM);
    drawFixedText(tft, 360, 80, 120, 10, "Grid Code:" + String(obj.grid_code), TFT_BLACK, hexToRGB565("#c8ddf8"), 1, MR_DATUM);
    drawFixedText(tft, 350, 90, 130, 10, "Smart Meter: " + String(obj.meterStatus), TFT_BLACK, hexToRGB565("#c8ddf8"), 1, MR_DATUM);
    drawFixedText(tft, 360, 100, 120, 10, "Type: " + String(obj.meterType), TFT_RED, hexToRGB565("#c8ddf8"), 1, MR_DATUM);
    drawFixedText(tft, 360, 110, 120, 10, "FW: " + String(obj.softwareVersion), TFT_RED, hexToRGB565("#c8ddf8"), 1, MR_DATUM);

    drawFixedText(tft, 360, 160, 120, 10, "Active Power: " + String(obj.activePower) + " kWh", TFT_YELLOW, hexToRGB565("#a8a9ab"), 1, MR_DATUM);
    drawFixedText(tft, 360, 170, 120, 10, "Grid: " + String(obj.gridVolt, 0) + " V | " + String(obj.gridFrequency, 0) + " Hz", TFT_WHITE, hexToRGB565("#a8a9ab"), 1, MR_DATUM);
    drawFixedText(tft, 360, 180, 120, 10, String(obj.gridCurrent, 2) + " A | ~" + String(obj.gridVolt * obj.gridCurrent, 0) + " W", TFT_WHITE, hexToRGB565("#a8a9ab"), 1, MR_DATUM);
    drawFixedText(tft, 360, 190, 120, 10, "Power factor: " + String(obj.gridPowerFactor, 2), TFT_YELLOW, hexToRGB565("#a8a9ab"), 1, MR_DATUM);

    tft.setTextColor(TFT_GREENYELLOW);
    tft.setCursor(410, 220);
    tft.print("Efficiency");
    drawFixedText(tft, 470, 230, 10, 10, String(obj.efficiency, 0) + " % ", TFT_WHITE, hexToRGB565("#858992"), 1, MR_DATUM);

    // IP
    drawFixedText(tft, 360, 245, 120, 10, "IP: " + String(obj.ip), TFT_WHITE, hexToRGB565("#858992"), 1, MR_DATUM);

    tft.setTextColor(TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(390, 290);
    tft.print("Revenue today");
    tft.setTextColor(TFT_BLACK);
    tft.setTextSize(1);
    tft.drawString("Daily Yield today", 120, 285);
    drawFixedText(tft, 390, 303, 80, 15, String(obj.dailyRevenue, 2) + " THB", TFT_RED, hexToRGB565("#e1e2e6"), 2);
    drawFixedText(tft, 10, 295, 120, 25, String(obj.dailyEnergyYield, 2) + " kWh ", TFT_ORANGE, hexToRGB565("#e1e2e6"), 4);
}

bool getDeviceInfo(void*) {
    Serial.println(" ---------------------------------------------------------");

    InverterData obj = inverter.getDeviceInfo();

    Serial.printf("Model: %s (%.0fKw)\n", obj.model.c_str(), obj.inverterPowerRate);
    Serial.printf("SN: %s\n", obj.serialNo.c_str());
    Serial.printf("FW Version: %s\n", obj.softwareVersion.c_str());
    Serial.printf("Status: %s\n", obj.status.c_str());
    Serial.printf("Type: %s\n", obj.meterType.c_str());
    Serial.printf("SmartMeter status: %s\n", obj.meterStatus.c_str());
    Serial.printf("Grid code : %s\n", obj.grid_code.c_str());
    Serial.printf("IP : %s\n", obj.ip.c_str());

    Serial.println("------------------------");
    Serial.printf("Active Power : %.3f kW\n", obj.activePower);
    Serial.printf("Grid Voltage: %.0f V, Current %.2f A (~ %.0f w)\n", obj.gridVolt, obj.gridCurrent, obj.gridVolt * obj.gridCurrent);
    Serial.printf("Grid Frequency: %.0f Hz\n", obj.gridFrequency);
    Serial.printf("Power factor: %.2f\n", obj.gridPowerFactor);
    Serial.printf("Efficiency: %.0f %%\n", obj.efficiency);
    Serial.printf("Temperature: %.1f °C\n", obj.temperature);

    Serial.printf("Positive active energy: %.2f kWh\n", obj.positiveActivePower);
    Serial.printf("Negative active energy: %s kWh\n", toCustomFixed(obj.reverseActivePower, 2));
    Serial.printf("Daily energy: %.2f Kwh\n", obj.dailyEnergyYield);
    Serial.printf("Total yield: %s Kwh\n", toCustomFixed(obj.accumulatedEnergy, 2));
    Serial.printf("Daily Revenue: %.2f THB/day\n", obj.dailyRevenue);

    Serial.println("------------------------");
    Serial.printf("PV : %s\n", formatPower(obj.pv_power * 1000, 3));
    Serial.printf("Grid : %s (Import from grid)\n", formatPower(obj.grid_power * 1000, 3));
    Serial.printf("Load : %s\n", formatPower((obj.activePower + obj.grid_power) * 1000, 3));

    Serial.println("------------------------");
    Serial.println("--- Solar Panels Details ---");
    Serial.printf("- PV1 Voltage: %.0f V / Current: %.1f A (string power: ~ %.0f w)\n", obj.pv1_voltage, obj.pv1_current, obj.pv1_voltage * obj.pv1_current);
    Serial.printf("- PV2 Voltage: %.0f V / Current: %.1f A (string power: ~ %.0f w)\n", obj.pv2_voltage, obj.pv2_current, obj.pv2_voltage * obj.pv2_current);
    Serial.printf("- PV3 Voltage: %.0f V / Current: %.1f A (string power: ~ %.0f w)\n", obj.pv3_voltage, obj.pv3_current, obj.pv3_voltage * obj.pv3_current);

    drawDashboard(obj);
    publishToSocketIO(webSocket, obj);

    digitalWrite(ESP32C3_LED, !digitalRead(ESP32C3_LED));
    return true;  // repeat? true
}

void setup() {
    Serial.begin(115200);
    hwSerial.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);  // Turn off the screen backlight
    pinMode(ESP32C3_LED, OUTPUT);

    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed!");
        return;
    }

    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_TRANSPARENT);
    tft.setSwapBytes(true);

    // Connect WIFI
    drawFixedText(tft, 240, 150, 40, 10, "Connecting WIFI .... ", TFT_WHITE, TFT_TRANSPARENT, 2);
    setup_Wifi();
    drawFixedText(tft, 240, 150, 40, 10, "Configuring the System Time Zone", TFT_WHITE, TFT_TRANSPARENT, 2);
    setupTimeZone();

    TJpgDec.setCallback(jpgRender);

    File file = SPIFFS.open(mainLogo);
    if (!file) {
        Serial.println("Failed to open " + mainLogo);
        return;
    }
    // Load Image
    TJpgDec.drawFsJpg(0, 0, mainLogo);

    if (inverter.connect()) {
        Serial.println("Connected to SUN2000");
    } else {
        Serial.println("Connection failed");
    }

    if (WiFi.status() == WL_CONNECTED) {
        if (ENABLE_SOCKETIO) {
            webSocket.begin(SOCKETIO_HOST, SOCKETIO_PORT);
            webSocket.on(SOCKETIO_CHANNEL, event);
        }

        esp_task_wdt_init(WDT_TIMEOUT, true);  // true = reset chip
        esp_task_wdt_add(NULL);

        timer.every(2000, getDeviceInfo);
    }
    lastTouchTime = millis();
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        webSocket.loop();
        esp_task_wdt_reset();
    }

    uint16_t x, y;
    if (tft.getTouch(&x, &y)) {
        if (!screenOn) {
            // touch detected, turn on the screen backlight
            digitalWrite(TFT_BL, HIGH);
            screenOn = true;
            Serial.println("Screen Waked Up!");
        }
        lastTouchTime = millis();  // reset the timer
    }

    // check if the screen has been on for X minutes
    if (screenOn && (millis() - lastTouchTime > timeout)) {
        digitalWrite(TFT_BL, LOW);  // Turn off the screen backlight
        screenOn = false;
        Serial.println("Screen Sleep (Power Saving)");
    }

    timer.tick();
}