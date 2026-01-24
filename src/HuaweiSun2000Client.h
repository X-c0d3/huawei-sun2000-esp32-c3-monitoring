/*
  # Author : Watchara Pongsri
  # [github/X-c0d3] https://github.com/X-c0d3/
  # Web Site: https://www.rockdevper.com
*/

#ifndef HUAWEI_SUN2000_CLIENT_H
#define HUAWEI_SUN2000_CLIENT_H
#include "models/InverterData.h"
#include "settings.h"
#include "utility.h"

class HuaweiSun2000Client {
   private:
    ModbusMaster node;
    HardwareSerial& serialPort;  // อ้างอิง Serial ที่ส่งมา (เช่น Serial2)
    uint8_t slaveId;
    uint32_t baudRate;

    float parseRegisterValue(uint16_t address, uint16_t quantity, float gain, bool isSigned = false) {
        uint8_t result = node.readHoldingRegisters(address, quantity);
        if (result != node.ku8MBSuccess) {
            Serial.printf("Read error at %d: 0x%02X\n", address, result);
            return 0;
        }

        uint32_t raw = 0;
        for (uint16_t i = 0; i < quantity; i++) {
            raw = (raw << 16) | node.getResponseBuffer(i);
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
        uint8_t result = node.readHoldingRegisters(address, quantity);
        if (result != node.ku8MBSuccess) {
            Serial.printf("String read error at %d: 0x%02X\n", address, result);
            return "-";
        }

        String str = "";
        for (uint16_t i = 0; i < quantity; i++) {
            uint16_t word = node.getResponseBuffer(i);
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
    // Constructor
    HuaweiSun2000Client(HardwareSerial& serial, uint8_t slave, uint32_t baud) : serialPort(serial), slaveId(slave), baudRate(baud) {
        node.begin(slaveId, serialPort);
    }

    bool connect() {
        serialPort.begin(baudRate, SERIAL_8N1);
        Serial.println("HuaweiSun2000Client connected");
        return true;
    }
    void disconnect() {
        serialPort.end();
    }

    float randomFloat(float minVal, float maxVal) {
        return minVal + (maxVal - minVal) * random(10000) / 10000.0;
    }
    InverterData getDeviceInfo() {
        InverterData data;
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
        data.grid_power = abs(parseRegisterValue(37113, 2, 1000.0f, true));
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

        return data;
    }
};

#endif