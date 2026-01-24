
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
    String model;
    String serialNo;
    float inverterPowerRate;
    String softwareVersion;
    String status;
    float activePower;
    float gridVolt;
    float gridCurrent;
    float gridFrequency;
    float gridPowerFactor;
    float efficiency;
    float temperature;
    String meterType;
    String meterStatus;
    float pv_power;
    String grid_code;
    float grid_power;
    float dailyEnergyYield;
    float accumulatedEnergy;
    float dailyRevenue;
    float positiveActivePower;
    float reverseActivePower;

    // PV details
    float pv1_voltage;
    float pv1_current;
    float pv2_voltage;
    float pv2_current;
    float pv3_voltage;
    float pv3_current;

    void toJson(JsonObject obj) const {
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
        obj["grid_code"] = grid_code;
        obj["dailyEnergyYield"] = dailyEnergyYield;
        obj["dailyRevenue"] = dailyRevenue;
        obj["accumulatedEnergy"] = accumulatedEnergy;
        obj["positiveActivePower"] = positiveActivePower;
        obj["reverseActivePower"] = reverseActivePower;
        obj["pv1_voltage"] = pv1_voltage;
        obj["pv1_current"] = pv1_current;
        obj["pv2_voltage"] = pv2_voltage;
        obj["pv2_current"] = pv2_current;
        obj["pv3_voltage"] = pv3_voltage;
        obj["pv3_current"] = pv3_current;
    }
};

#endif
