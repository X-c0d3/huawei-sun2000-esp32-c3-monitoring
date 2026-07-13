
/*
  # Author : Watchara Pongsri
  # [github/X-c0d3] https://github.com/X-c0d3/
  # Web Site: https://www.rockdevper.com
*/

#ifndef InverterData_h
#define InverterData_h

#include <ArduinoJson.h>

#include "Arduino.h"

class InverterData {
   public:
    // true only after a full, successful Modbus read. Consumers MUST check this
    // before using/publishing the data — a failed read leaves fields at their
    // safe defaults below instead of stack garbage.
    bool valid = false;

    String ip;
    String model;
    String serialNo;
    float inverterPowerRate = 0;
    String softwareVersion;
    String status;
    float activePower = 0;
    float gridVolt = 0;
    float gridCurrent = 0;
    float gridFrequency = 0;
    float gridPowerFactor = 0;
    float efficiency = 0;
    float temperature = 0;
    String meterType;
    String meterStatus;
    float pv_power = 0;
    String grid_code;
    float grid_power = 0;
    float load_power = 0;
    float dailyEnergyYield = 0;
    float accumulatedEnergy = 0;
    float dailyRevenue = 0;
    float gridExportEnergy = 0;    // reg 37119: cumulative kWh fed TO grid
    float gridImportEnergy = 0;    // reg 37121: cumulative kWh from grid
    float dailyGridImport = 0;     // calculated: kWh imported from grid today

    // PV details
    float pv1_voltage = 0;
    float pv1_current = 0;
    float pv2_voltage = 0;
    float pv2_current = 0;
    float pv3_voltage = 0;
    float pv3_current = 0;

    void toJson(JsonObject obj) const {
        obj["ip"] = ip;
        obj["model"] = model;
        obj["serialNo"] = serialNo;
        obj["inverterPowerRate"] = inverterPowerRate;
        obj["softwareVersion"] = softwareVersion;
        obj["status"] = status;
        obj["activePower"] = activePower;
        obj["gridVolt"] = gridVolt;
        obj["gridCurrent"] = gridCurrent;
        obj["gridFrequency"] = gridFrequency;
        obj["gridPowerFactor"] = gridPowerFactor;
        obj["efficiency"] = efficiency;
        obj["temperature"] = temperature;
        obj["meterType"] = meterType;
        obj["meterStatus"] = meterStatus;
        obj["pv_power"] = pv_power;
        obj["grid_power"] = grid_power;
        obj["load_power"] = load_power;
        obj["grid_code"] = grid_code;
        obj["dailyEnergyYield"] = dailyEnergyYield;
        obj["dailyRevenue"] = dailyRevenue;
        obj["accumulatedEnergy"] = accumulatedEnergy;
        obj["gridExportEnergy"] = gridExportEnergy;
        obj["gridImportEnergy"] = gridImportEnergy;
        obj["dailyGridImport"] = dailyGridImport;
        obj["pv1_voltage"] = pv1_voltage;
        obj["pv1_current"] = pv1_current;
        obj["pv2_voltage"] = pv2_voltage;
        obj["pv2_current"] = pv2_current;
        obj["pv3_voltage"] = pv3_voltage;
        obj["pv3_current"] = pv3_current;
    }
};

#endif
