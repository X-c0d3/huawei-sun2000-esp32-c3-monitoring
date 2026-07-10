/*
  # Author : Watchara Pongsri
  # [github/X-c0d3] https://github.com/X-c0d3/
  # Web Site: https://www.rockdevper.com
*/

#ifndef HUAWEI_SUN2000_CLIENT_H
#define HUAWEI_SUN2000_CLIENT_H
#include <ModbusMaster.h>
#include <WiFiClient.h>

#include "models/InverterData.h"
#include "settings.h"
#include "utility.h"

class HuaweiSun2000Client {
   private:
    ModbusMaster node;
    HardwareSerial* serialPort = nullptr;  // ใช้เมื่อเป็นโหมด RS485 (RTU)
    WiFiClient tcpClient;                  // ใช้เมื่อเป็นโหมด Modbus-TCP (ผ่าน Smart Dongle)
    IPAddress tcpHost;
    uint16_t tcpPort = 502;
    bool useTcp = false;
    uint8_t slaveId;
    uint32_t baudRate = 0;
    uint16_t regBuffer[125];

    // Field-tested against a real Smart Dongle: the very first Modbus
    // request right after a fresh TCP connect (or after the dongle has been
    // idle) almost always times out, even though the connection itself is
    // fine. A short retry on the SAME socket succeeds on the 2nd/3rd try.
    // No reconnect is needed between attempts, just a small delay.
    static const uint8_t MODBUS_TCP_MAX_RETRIES = 3;
    static const uint16_t MODBUS_TCP_RETRY_DELAY_MS = 500;

    // Single-attempt Modbus-TCP (MBAP) function code 0x03: read holding registers.
    // The dongle only forwards requests when "Modbus TCP > Unrestricted" is enabled on the inverter/app side.
    bool readHoldingRegistersTcpOnce(uint16_t address, uint16_t quantity) {
        if (!tcpClient.connected() && !tcpClient.connect(tcpHost, tcpPort)) {
            Serial.println("Modbus TCP connect failed");
            return false;
        }

        static uint16_t transactionId = 0;
        transactionId++;
        uint8_t request[12] = {
            (uint8_t)(transactionId >> 8), (uint8_t)(transactionId & 0xFF),
            0x00, 0x00,  // Protocol ID
            0x00, 0x06,  // Length: unitId + function code + address + quantity
            slaveId,
            0x03,
            (uint8_t)(address >> 8), (uint8_t)(address & 0xFF),
            (uint8_t)(quantity >> 8), (uint8_t)(quantity & 0xFF)};
        tcpClient.write(request, sizeof(request));

        uint8_t header[9];  // MBAP(7) + function code(1) + byte count(1)
        unsigned long start = millis();
        while (tcpClient.available() < (int)sizeof(header)) {
            if (millis() - start > API_TIMEOUT) {
                Serial.println("Modbus TCP response timeout");
                return false;  // don't stop() here — let the retry wrapper reuse the socket
            }
        }
        tcpClient.read(header, sizeof(header));

        if (header[7] & 0x80) {
            Serial.printf("Modbus TCP exception at %d: 0x%02X\n", address, header[8]);
            return false;
        }

        uint8_t byteCount = header[8];
        uint8_t data[250];
        start = millis();
        while ((uint16_t)tcpClient.available() < byteCount) {
            if (millis() - start > API_TIMEOUT) {
                Serial.println("Modbus TCP data timeout");
                return false;
            }
        }
        tcpClient.read(data, byteCount);

        for (uint16_t i = 0; i < quantity; i++) {
            regBuffer[i] = (data[i * 2] << 8) | data[i * 2 + 1];
        }
        return true;
    }

    // Retries the same open TCP connection a few times before giving up.
    // This absorbs the dongle's "cold start" timeout on the first request.
    bool readHoldingRegistersTcp(uint16_t address, uint16_t quantity) {
        for (uint8_t attempt = 1; attempt <= MODBUS_TCP_MAX_RETRIES; attempt++) {
            if (readHoldingRegistersTcpOnce(address, quantity)) {
                return true;
            }
            Serial.printf("Modbus TCP attempt %d/%d failed for register %d\n", attempt, MODBUS_TCP_MAX_RETRIES, address);

            if (attempt < MODBUS_TCP_MAX_RETRIES) {
                delay(MODBUS_TCP_RETRY_DELAY_MS);
            }
        }
        // All retries exhausted — the socket may be in a bad state, force a
        // fresh connect() on the next call instead of leaving it half-open.
        tcpClient.stop();
        return false;
    }

    bool readHoldingRegisters(uint16_t address, uint16_t quantity) {
        if (useTcp) {
            return readHoldingRegistersTcp(address, quantity);
        }

        uint8_t result = node.readHoldingRegisters(address, quantity);
        if (result != node.ku8MBSuccess) {
            Serial.printf("Read error at %d: 0x%02X\n", address, result);
            return false;
        }
        for (uint16_t i = 0; i < quantity; i++) {
            regBuffer[i] = node.getResponseBuffer(i);
        }
        return true;
    }

    float parseRegisterValue(uint16_t address, uint16_t quantity, float gain, bool isSigned = false) {
        if (!readHoldingRegisters(address, quantity)) {
            throw std::runtime_error("Modbus read failed");
        }

        uint32_t raw = 0;
        for (uint16_t i = 0; i < quantity; i++) {
            raw = (raw << 16) | regBuffer[i];
        }

        float value;
        if (isSigned) {
            if (quantity == 1) {
                value = (float)(int16_t)raw;
            } else {
                value = (float)(int32_t)raw;
            }
        } else {
            value = (float)raw;
        }

        return value / gain;
    }
    String parseRegisterString(uint16_t address, uint16_t quantity, bool trim = true, bool stopAtNull = true) {
        if (!readHoldingRegisters(address, quantity)) {
            Serial.printf("String read error at %d\n", address);
            return "-";
        }

        String str = "";
        for (uint16_t i = 0; i < quantity; i++) {
            uint16_t word = regBuffer[i];
            char hi = (word >> 8) & 0xFF;
            char lo = word & 0xFF;

            if (stopAtNull && hi == 0) break;
            str += hi;

            if (stopAtNull && lo == 0) break;
            str += lo;
        }

        if (trim) str.trim();
        return str;
    }

   public:
    // RS485 (Modbus-RTU)
    HuaweiSun2000Client(HardwareSerial& serial, uint8_t slave, uint32_t baud) : serialPort(&serial), slaveId(slave), baudRate(baud) {
        node.begin(slaveId, serial);
    }

    // Modbus-TCP via Smart Dongle (e.g. SDongleA-05 with "Modbus TCP: Unrestricted" enabled)
    HuaweiSun2000Client(const char* host, uint16_t port, uint8_t slave) : tcpPort(port), useTcp(true), slaveId(slave) {
        tcpHost.fromString(host);
    }

    bool connect() {
        if (useTcp) {
            bool ok = tcpClient.connect(tcpHost, tcpPort);
            Serial.println(ok ? "HuaweiSun2000Client connected (Modbus TCP)" : "HuaweiSun2000Client: Modbus TCP connect failed");
            return ok;
        }
        serialPort->begin(baudRate, SERIAL_8N1);
        Serial.println("HuaweiSun2000Client connected");
        return true;
    }
    void disconnect() {
        if (useTcp) {
            tcpClient.stop();
        } else {
            serialPort->end();
        }
    }

    float randomFloat(float minVal, float maxVal) {
        return minVal + (maxVal - minVal) * random(10000) / 10000.0;
    }
    InverterData getDeviceInfo() {
        unsigned long startTime = micros();
        InverterData data;
        try {
            data.ip = WiFi.localIP().toString();
            data.model = parseRegisterString(30000, 15);
            data.serialNo = parseRegisterString(30015, 10);
            data.inverterPowerRate = parseRegisterValue(30073, 2, 1000.0f);
            data.softwareVersion = parseRegisterString(31025, 15);
            data.status = getDeviceStatusDescription(parseRegisterValue(32089, 1, 1.0f));
            data.activePower = parseRegisterValue(32080, 2, 1000.0f, true);
            data.gridVolt = parseRegisterValue(37101, 2, 10.0f);
            data.gridCurrent = abs(parseRegisterValue(37107, 2, 100.0f, true));
            data.gridFrequency = parseRegisterValue(37118, 1, 100.0f);
            data.gridPowerFactor = parseRegisterValue(37117, 1, 1000.0f);
            data.efficiency = parseRegisterValue(32086, 1, 100.0f);
            data.temperature = parseRegisterValue(32087, 1, 10.0f);
            data.meterType = parseRegisterValue(37125, 2, 1.0f) == 0 ? "Single-phase" : "Three-phase";
            data.meterStatus = parseRegisterValue(37100, 1, 1.0f) == 0 ? "Offline" : "Normal";
            data.pv_power = parseRegisterValue(32064, 2, 1000.0f);
            data.grid_power = parseRegisterValue(37113, 2, 1000.0f, true);
            data.load_power = data.pv_power + data.grid_power;
            data.grid_code = getGridCode(parseRegisterValue(42000, 1, 1));
            data.dailyEnergyYield = parseRegisterValue(32114, 2, 100.0f);
            data.dailyRevenue = data.dailyEnergyYield * float(ELECTRICITY_PRICE);
            data.accumulatedEnergy = parseRegisterValue(32106, 2, 100.0f);
            data.positiveActivePower = parseRegisterValue(37119, 2, 100.0f);
            data.reverseActivePower = parseRegisterValue(37121, 2, 100.0f);
            data.pv1_voltage = parseRegisterValue(32016, 1, 10.0f);
            data.pv1_current = parseRegisterValue(32017, 1, 100.0f);
            data.pv2_voltage = parseRegisterValue(32018, 1, 10.0f);
            data.pv2_current = parseRegisterValue(32019, 1, 100.0f);
            data.pv3_voltage = parseRegisterValue(32020, 1, 10.0f);
            data.pv3_current = parseRegisterValue(32021, 1, 100.0f);

            // --- TEST DATA ---
            // data.ip = WiFi.localIP().toString();
            // data.model = "SUN2000-10K-LC0";
            // data.serialNo = "TA2510131915";
            // data.inverterPowerRate = 10;
            // data.softwareVersion = "V100R023C10SPC111";
            // data.status = (randomFloat(1, 10) > 5) ? "Grid connected" : "Standby : no sunlight";
            // data.activePower = randomFloat(0.020f, 1.6f);
            // data.gridVolt = randomFloat(220.0f, 235.6f);
            // data.gridCurrent = randomFloat(1.0f, 11.6f);
            // data.gridFrequency = randomFloat(48.0f, 51.0f);
            // data.gridPowerFactor = randomFloat(0.1f, 1.0f);
            // data.efficiency = randomFloat(90.0f, 99.0f);
            // data.temperature = randomFloat(30.0f, 40.0f);
            // data.meterType = (randomFloat(1, 10) > 5) ? "Single-phase" : "Three-phase";
            // data.meterStatus = (randomFloat(1, 10) > 5) ? "Offline" : "Normal";
            // data.pv_power = randomFloat(0.200f, 2.6f);
            // data.grid_power = randomFloat(0.020f, 1.2f);
            // data.load_power = data.pv_power + data.grid_power;
            // data.grid_code = getGridCode(randomFloat(26, 27));
            // data.dailyEnergyYield = randomFloat(1.0f, 25.6f);
            // data.dailyRevenue = data.dailyEnergyYield * float(ELECTRICITY_PRICE);
            // data.accumulatedEnergy = 204400.0f;
            // data.positiveActivePower = 3400.0f;
            // data.reverseActivePower = 2342.0f;
            // data.pv1_voltage = randomFloat(220.0f, 400.0f);
            // data.pv1_current = randomFloat(2.0f, 15.0f);
            // data.pv2_voltage = randomFloat(220.0f, 400.0f);
            // data.pv2_current = randomFloat(2.0f, 15.0f);
            // data.pv3_voltage = 0;  // randomFloat(220.0f, 400.0f);
            // data.pv3_current = randomFloat(2.0f, 15.0f);

            unsigned long elapsedTime = micros() - startTime;
            Serial.print(">>> GetModBusData ElapsedTime: ");
            Serial.println(formatDuration(elapsedTime));
        } catch (std::exception& e) {
            Serial.println("-- ERROR: " + String(e.what()));
        }
        return data;
    }
};

#endif