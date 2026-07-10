/*
  # Author : Watchara Pongsri
  # [github/X-c0d3] https://github.com/X-c0d3/
  # Web Site: https://www.rockdevper.com
*/

#include <HardwareSerial.h>
#include <ModbusMaster.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <arduino-timer.h>

#include "HuaweiSun2000Client.h"
#include "broadCastToClients.h"
#include "utility.h"
#include "wifiMan.h"
#define TX_PIN 1
#define RX_PIN 3

// Config RS485 adapter (Reader)
#define SLAVE_ID 1
#define BAUD_RATE 9600
#define WDT_TIMEOUT 60

#if USE_MODBUS_TCP
HuaweiSun2000Client inverter(MODBUS_TCP_HOST, MODBUS_TCP_PORT, SLAVE_ID);
#else
HardwareSerial hwSerial(1);
HuaweiSun2000Client inverter(hwSerial, SLAVE_ID, BAUD_RATE);
#endif
WiFiClient mqttWifiClient;
PubSubClient mqttClient(mqttWifiClient);

auto timer = timer_create_default();  // create a timer with default settings
bool screenOn = true;

float gridImportBaseline = -1.0f;
int lastResetDay = -1;
unsigned long lastTouchTime = 0;
const unsigned long timeout = 2 * 60 * 1000;  // 2min  Screen Sleep (Power Saving)

// callback for drwaing JPG into TFT_eSPI
unsigned long lastMqttReconnectAttempt = 0;
const unsigned long mqttReconnectInterval = 5000;

void mqttReconnect() {
    if (mqttClient.connected())
        return;
    if (millis() - lastMqttReconnectAttempt < mqttReconnectInterval)
        return;
    lastMqttReconnectAttempt = millis();

    String clientId = "ESP32-" + getChipId();
    if (mqttClient.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
        Serial.println("MQTT connected");
    } else {
        Serial.print("MQTT connect failed, rc=");
        Serial.println(mqttClient.state());
    }
}

void event(const char* payload, size_t length) {
    // Serial.println("Socket.io Event Received:");
    // Serial.println(String(payload).substring(0, length));
}

bool getDeviceInfo(void*) {
    Serial.println("<< GetDeviceInfo >>");
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

    InverterData obj = inverter.getDeviceInfo();

    // Daily grid import = reg37121_now - reg37121_at_midnight
    {
        time_t now = time(nullptr);
        struct tm* t = localtime(&now);
        if (t->tm_mday != lastResetDay) {
            gridImportBaseline = obj.gridImportEnergy;
            lastResetDay = t->tm_mday;
            Serial.printf("Daily grid import baseline reset: %.2f kWh\n", gridImportBaseline);
        }
        obj.dailyGridImport = (gridImportBaseline >= 0)
            ? max(0.0f, obj.gridImportEnergy - gridImportBaseline)
            : 0.0f;
    }

    unsigned long startTime = micros();
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

    Serial.printf("Grid export energy (cumulative): %.2f kWh\n", obj.gridExportEnergy);
    Serial.printf("Grid import energy (cumulative): %.2f kWh\n", obj.gridImportEnergy);
    Serial.printf("Daily grid import: %.2f kWh\n", obj.dailyGridImport);
    Serial.printf("Daily energy: %.2f Kwh\n", obj.dailyEnergyYield);
    Serial.printf("Total yield: %s Kwh\n", toCustomFixed(obj.accumulatedEnergy, 2).c_str());
    Serial.printf("Daily Revenue: %.2f THB/day\n", obj.dailyRevenue);

    Serial.println("------------------------");
    Serial.printf("PV   : %s\n", formatPower(obj.pv_power * 1000, 3).c_str());
    Serial.printf("Grid : %s  (+export / -import)\n", formatPower(obj.grid_power * 1000, 3).c_str());
    Serial.printf("Load : %s\n", formatPower(obj.load_power * 1000, 3).c_str());

    Serial.println("------------------------");
    Serial.println("--- Solar Panels Details ---");
    Serial.printf("- PV1 Voltage: %.0f V / Current: %.1f A (string power: ~ %.0f w)\n", obj.pv1_voltage, obj.pv1_current, obj.pv1_voltage * obj.pv1_current);
    Serial.printf("- PV2 Voltage: %.0f V / Current: %.1f A (string power: ~ %.0f w)\n", obj.pv2_voltage, obj.pv2_current, obj.pv2_voltage * obj.pv2_current);
    Serial.printf("- PV3 Voltage: %.0f V / Current: %.1f A (string power: ~ %.0f w)\n", obj.pv3_voltage, obj.pv3_current, obj.pv3_voltage * obj.pv3_current);

    yield();
    publishToMqtt(mqttClient, obj);

    unsigned long elapsedTime = micros() - startTime;
    Serial.println(" -------------- ElapsedTime " + formatDuration(elapsedTime) + " -------------- Last Update: " + DateNowString());

    return true;  // repeat? true
}

void setup() {
    Serial.begin(115200);
#if !USE_MODBUS_TCP
    hwSerial.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
#endif

    pinMode(LED_BUILTIN, OUTPUT);
    setup_Wifi();
    setupTimeZone();

    if (inverter.connect()) {
        Serial.println("Connected to SUN2000");
    } else {
        Serial.println("Connection failed");
    }

    if (WiFi.status() == WL_CONNECTED) {
        if (ENABLE_MQTT) {
            mqttClient.setServer(MQTT_BROKER_HOST, MQTT_BROKER_PORT);
            mqttClient.setBufferSize(4096);
            mqttClient.setKeepAlive(60);
            mqttReconnect();
        }

        timer.every(3000, getDeviceInfo);
    }
    wdt_enable(WDT_TIMEOUT);
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        if (ENABLE_MQTT) {
            mqttReconnect();
            mqttClient.loop();
        }
        wdt_reset();
    }

    timer.tick();
}