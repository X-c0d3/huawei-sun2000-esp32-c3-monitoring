/*
  # Author : Watchara Pongsri
  # [github/X-c0d3] https://github.com/X-c0d3/
  # Web Site: https://www.rockdevper.com
*/

#ifndef UTILITY_H
#define UTILITY_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <TFT_eSPI.h>

int timezone = 7;
char ntp_server1[20] = "ntp.ku.ac.th";
char ntp_server2[20] = "fw.eng.ku.ac.th";
char ntp_server3[20] = "time.uni.net.th";
int dst = 0;

void printMessage(int X, int Y, String message, bool isPrintLn) {
    if (isPrintLn)
        Serial.println(message);
    else
        Serial.print(message);
}

String NowString() {
    time_t now = time(nullptr);
    struct tm* newtime = localtime(&now);
    String tmpNow = "";
    tmpNow += String(newtime->tm_hour);
    tmpNow += ":";
    tmpNow += String(newtime->tm_min);
    tmpNow += ":";
    tmpNow += String(newtime->tm_sec);
    return tmpNow;
}

String DateNowString() {
    time_t now = time(nullptr);
    struct tm* newtime = localtime(&now);
    // Serial.println(ctime(&now));
    String tmpNow = "";
    tmpNow += String(newtime->tm_mday);
    tmpNow += "/";
    tmpNow += String(newtime->tm_mon + 1);
    tmpNow += "/";
    tmpNow += String(newtime->tm_year + 1900);
    tmpNow += " ";
    tmpNow += String(newtime->tm_hour);
    tmpNow += ":";
    tmpNow += String(newtime->tm_min);
    tmpNow += ":";
    tmpNow += String(newtime->tm_sec);
    return tmpNow;
}

void setupTimeZone() {
    configTime(timezone * 3600, dst, ntp_server1, ntp_server2, ntp_server3);
    Serial.println("Waiting for time");
    while (!time(nullptr)) {
        Serial.print(".");
        delay(500);
    }
    Serial.println("Curent time: " + NowString());
}

String getSplitValue(String data, char separator, int index) {
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i + 1 : i;
        }
    }

    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

String getChipId() {
    String ChipIdHex = String((uint32_t)(ESP.getEfuseMac() >> 32), HEX);
    ChipIdHex += String((uint32_t)ESP.getEfuseMac(), HEX);
    return ChipIdHex;
}

String smartEnergyW(float wh) {
    if (wh < 1000.0) {
        return String(wh, 1) + " Wh";
    } else if (wh < 1000000.0) {
        float kwh = wh / 1000.0;
        return String(kwh, 2) + " kWh";
    } else {
        float mwh = wh / 1000000.0;
        return String(mwh, 3) + " MWh";
    }
}

String smartDuration(long sec) {
    if (sec < 60) {
        return String(sec) + " sec";
    } else if (sec < 3600) {
        float minutes = sec / 60.0;
        return String(minutes, minutes < 10 ? 1 : 0) + " min";
    } else if (sec < 86400) {
        float hours = sec / 3600.0;
        return String(hours, hours < 10 ? 1 : 0) + " hr";
    } else {
        float days = sec / 86400.0;
        return String(days, days < 10 ? 1 : 0) + " days";
    }
}

void drawBar(TFT_eSPI tft, int x, int y, int w, int h, float value, float maxValue, uint16_t color) {
    int filled = (int)((value / maxValue) * w);
    tft.fillRect(x, y, w, h, TFT_DARKGREY);
    tft.fillRect(x, y, filled, h, color);
}

void drawFixedText(TFT_eSPI& tft, int x, int y,
                   int width, int height,
                   String text,
                   uint16_t textColor,
                   uint16_t bgColor,
                   uint8_t fontSize,
                   uint8_t align = MC_DATUM) {
    tft.fillRect(x, y, width, height, bgColor);
    //  ตั้งค่าฟอนต์
    tft.setTextFont(fontSize);
    tft.setTextColor(textColor, bgColor);

    tft.setTextDatum(align);
    int drawX;
    int drawY = y + (height / 2);

    switch (align) {
        case ML_DATUM:
            drawX = x + 4;
            break;
        case MR_DATUM:
            drawX = x + width - 4;
            break;
        case MC_DATUM:
        default:
            drawX = x + (width / 2);
            break;
    }

    tft.drawString(text, drawX, drawY);
}

void drawStatus(TFT_eSPI tft, int x, int y, bool status) {
    tft.fillCircle(x, y, 6, status ? TFT_GREEN : TFT_RED);
}

String httpGet(String url) {
    HTTPClient http;
    http.begin(url);
    int code = http.GET();
    if (code == HTTP_CODE_OK) {
        String payload = http.getString();
        http.end();
        return payload;
    }
    http.end();
    Serial.println("HTTP GET failed: " + String(code));
    return "";
}
String getDeviceStatusDescription(uint16_t code) {
    switch (code) {
        case 0xa000:
            return "Standby : no sunlight";
        case 0x0000:
            return "Standby: initializing";
        case 0x0001:
            return "Standby: detecting insulation resistance";
        case 0x0002:
            return "Standby: detecting irradiation";
        case 0x0003:
            return "Standby: grid detecting";
        case 0x0100:
            return "Starting";
        case 0x0200:
            return "Grid connected";
        case 0x0201:
            return "Grid connection: power limited";
        case 0x0202:
            return "Grid connection: self-derating";
        case 0x0203:
            return "Off-grid Running";
        case 0x0300:
            return "Shutdown: fault";
        case 0x0301:
            return "Shutdown: command";
        case 0x0302:
            return "Shutdown: OVGR";
        case 0x0303:
            return "Shutdown: communication disconnected";
        case 0x0304:
            return "Shutdown: power limited";
        case 0x0305:
            return "Shutdown: manual startup required";
        case 0x0306:
            return "Shutdown: DC switches disconnected";
        case 0x0307:
            return "Shutdown: rapid cutoff";
        case 0x0308:
            return "Shutdown: input underpower";
        case 0x0401:
            return "Grid scheduling: cosφ-P curve";
        case 0x0402:
            return "Grid scheduling: Q-U curve";
        case 0x0403:
            return "Grid scheduling: PF-U curve";
        case 0x0404:
            return "Grid scheduling: dry contact";
        case 0x0405:
            return "Grid scheduling: Q-P curve";
        case 0x0500:
            return "Spot-check ready";
        case 0x0501:
            return "Spot-checking";
        case 0x0600:
            return "Inspecting";
        case 0x0700:
            return "AFCI self check";
        case 0x0800:
            return "I-V scanning";
        case 0x0900:
            return "DC input detection";

        default:
            char buf[20];
            snprintf(buf, sizeof(buf), "Unknown status: 0x%04X", code);
            return String(buf);
    }
}

String getGridCode(uint16_t code) {
    switch (code) {
        case 26:
            return "PEA";
        case 27:
            return "MEA";

        // add more your standard/countrycode in here
        default:
            char buf[20];
            snprintf(buf, sizeof(buf), "Unknown code: 0x%04X", code);
            return String(buf);
    }
}

String toCustomFixed(float value, int decimals = 0) {
    char buf[32];
    char format[16];

    snprintf(format, sizeof(format), "%%.%df", decimals);

    snprintf(buf, sizeof(buf), format, value);

    return String(buf);
}

String formatPower(float power, int decimals = 3) {
    if (isnan(power)) {
        return "N/A";
    }

    float absPower = fabs(power);
    String sign = (power < 0) ? "-" : "";

    if (absPower < 1000) {
        return sign + String((int)round(absPower)) + " W";
    } else {
        float kw = absPower / 1000.0f;
        char buf[16];
        snprintf(buf, sizeof(buf), "%.*f kW", decimals, kw);
        return sign + String(buf);
    }
}

#endif