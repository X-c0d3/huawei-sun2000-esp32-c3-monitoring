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

// Config
#define SLAVE_ID 1
#define BAUD_RATE 9600
#define WDT_TIMEOUT 30

TFT_eSPI tft = TFT_eSPI();
HardwareSerial hwSerial(1);
HuaweiSun2000Client inverter(hwSerial, SLAVE_ID, BAUD_RATE);
SocketIoClient webSocket;

auto timer = timer_create_default();  // create a timer with default settings
const int ESP32C3_LED = 8;
String mainLogo = "/app-logo.jpg";

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
    uint32_t rgb = strtoul(hex + 1, NULL, 16);  // ข้าม '#'
    uint8_t r = (rgb >> 16) & 0xFF;
    uint8_t g = (rgb >> 8) & 0xFF;
    uint8_t b = rgb & 0xFF;

    return tft.color565(r, g, b);
}

void drawDashboard(InverterData obj) {
    tft.setTextSize(2);
    tft.setTextColor(TFT_YELLOW);
    // PV Power
    drawFixedText(tft, 70, 35, 100, 18, formatPower(obj.pv_power * 1000, 3), TFT_YELLOW, hexToRGB565("#3B4454"), 1);
    // Home Power (Load)
    drawFixedText(tft, 290, 35, 100, 18, formatPower((obj.activePower + obj.grid_power) * 1000, 3), TFT_GREENYELLOW, hexToRGB565("#3B4454"), 1);
    // Grid Power
    drawFixedText(tft, 270, 295, 100, 18, formatPower(obj.grid_power * 1000, 3), TFT_BLACK, hexToRGB565("#e5ecf4"), 1);

    tft.setTextSize(1);
    if (obj.pv1_voltage > 0) {
        tft.setTextColor(TFT_GREENYELLOW);
        tft.setCursor(10, 70);
        tft.print("PV1");
        // 4f5666
        drawFixedText(tft, 10, 80, 140, 10,
                      String(obj.pv1_voltage, 0) + "V / " + String(obj.pv1_current, 1) + "A / ~" + String(obj.pv1_voltage * obj.pv1_current, 0) + " W",
                      TFT_PINK, hexToRGB565("#4f5666"), 1);
    }

    if (obj.pv2_voltage > 0) {
        tft.setTextColor(TFT_GREENYELLOW);
        tft.setCursor(10, 95);
        tft.print("PV2");
        drawFixedText(tft, 10, 105, 140, 10,
                      String(obj.pv2_voltage, 0) + "V / " + String(obj.pv2_current, 1) + "A / ~" + String(obj.pv2_voltage * obj.pv2_current, 0) + " W",
                      TFT_PINK, hexToRGB565("#4f5666"), 1);
    }

    if (obj.pv3_voltage > 0) {
        tft.setTextColor(TFT_GREENYELLOW);
        tft.setCursor(10, 120);
        tft.print("PV3");
        drawFixedText(tft, 10, 130, 140, 10,
                      String(obj.pv3_voltage, 0) + "V / " + String(obj.pv3_current, 1) + "A / ~" + String(obj.pv3_voltage * obj.pv3_current, 0) + " W",
                      TFT_PINK, hexToRGB565("#4f5666"), 1);
    }

    tft.setTextColor(TFT_GREENYELLOW);
    tft.setCursor(165, 220);
    tft.print("Temp");
    drawFixedText(tft, 165, 230, 30, 10, String(obj.temperature, 0) + " C\xF8", TFT_WHITE, hexToRGB565("#525f6f"), 1);

    // Model
    drawFixedText(tft, 400, 80, 120, 10, "Grid Code:" + String(obj.grid_code), TFT_WHITE, hexToRGB565("#525f6f"), 1);
    drawFixedText(tft, 358, 90, 120, 10, "Smart Meter: " + String(obj.meterStatus), TFT_WHITE, hexToRGB565("#525f6f"), 1);
    drawFixedText(tft, 365, 100, 120, 10, "Type: " + String(obj.meterType), TFT_WHITE, hexToRGB565("#525f6f"), 1);
    drawFixedText(tft, 360, 110, 120, 10, "FW: " + String(obj.softwareVersion), TFT_WHITE, hexToRGB565("#525f6f"), 1);

    drawFixedText(tft, 138, 155, 130, 10, obj.status.c_str(), TFT_WHITE, hexToRGB565("#687785"), 1);
    tft.setTextColor(TFT_WHITE);
    tft.drawString(obj.model.c_str(), 138, 170);

    drawFixedText(tft, 345, 160, 100, 10, "Active Power: " + String(obj.activePower) + " kWh", TFT_WHITE, hexToRGB565("#525f6f"), 1);
    drawFixedText(tft, 360, 170, 100, 10, "Grid: " + String(obj.gridVolt, 0) + " V | " + String(obj.gridFrequency, 0) + " Hz", TFT_WHITE, hexToRGB565("#525f6f"), 1);
    drawFixedText(tft, 390, 180, 90, 10, String(obj.gridCurrent, 0) + " A | ~" + String(obj.gridVolt * obj.gridCurrent, 0) + " W", TFT_WHITE, hexToRGB565("#525f6f"), 1);
    drawFixedText(tft, 365, 190, 100, 10, "Power factor: " + String(obj.gridPowerFactor, 2), TFT_WHITE, hexToRGB565("#525f6f"), 1);

    tft.setTextColor(TFT_GREENYELLOW);
    tft.setCursor(405, 220);
    tft.print("Efficiency");
    drawFixedText(tft, 448, 230, 5, 10, String(obj.efficiency, 0) + " % ", TFT_WHITE, hexToRGB565("#525f6f"), 1);

    // IP
    tft.drawString("IP: " + WiFi.localIP().toString(), 360, 245);

    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    tft.setCursor(15, 220);
    tft.print("Revenue today");
    tft.setTextColor(TFT_BLACK);
    tft.setTextSize(1);
    tft.drawString("Daily Yield today", 15, 278);
    drawFixedText(tft, 10, 230, 100, 15, String(obj.dailyRevenue, 2) + " THB", TFT_ORANGE, hexToRGB565("#687785"), 2);
    drawFixedText(tft, 10, 290, 120, 25, String(obj.dailyEnergyYield, 2) + " kWh ", TFT_ORANGE, hexToRGB565("#e5ecf4"), 4);
}

bool getDeviceInfo(void*) {
    Serial.println(" ------------------------------------------------------------------------------");

    InverterData obj = inverter.getDeviceInfo();

    Serial.printf("Model: %s (%.0fKw)\n", obj.model.c_str(), obj.inverterPowerRate);
    Serial.printf("SN: %s\n", obj.serialNo.c_str());
    Serial.printf("FW Version: %s\n", obj.softwareVersion.c_str());
    Serial.printf("Status: %s\n", obj.status.c_str());
    Serial.printf("Type: %s\n", obj.meterType.c_str());
    Serial.printf("SmartMeter status: %s\n", obj.meterStatus.c_str());
    Serial.printf("Grid code : %s\n", obj.grid_code.c_str());

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

    pinMode(ESP32C3_LED, OUTPUT);

    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed!");
        return;
    }

    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_TRANSPARENT);
    tft.setSwapBytes(true);

    TJpgDec.setCallback(jpgRender);

    File file = SPIFFS.open(mainLogo);
    if (!file) {
        Serial.println("Failed to open " + mainLogo);
        return;
    }
    // Load Image
    TJpgDec.drawFsJpg(0, 0, mainLogo);

    // Connect WIFI
    setup_Wifi();
    setupTimeZone();

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

        timer.every(2000, getDeviceInfo);

        esp_task_wdt_init(WDT_TIMEOUT, true);  // true = reset chip
        esp_task_wdt_add(NULL);
    }
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        webSocket.loop();
        esp_task_wdt_reset();
    }

    timer.tick();
}