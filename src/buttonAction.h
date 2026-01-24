/*
  # Author : Watchara Pongsri
  # [github/X-c0d3] https://github.com/X-c0d3/
  # Web Site: https://www.rockdevper.com
*/

#ifndef BUTTON_ACTION_H
#define BUTTON_ACTION_H

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "settings.h"

bool smartPress(TFT_eSPI tft, int x_target, int y_target, int scope) {
    const int samples = 5;
    int sx = 0, sy = 0, count = 0;
    uint16_t tx, ty;

    for (int i = 0; i < samples; i++) {
        if (tft.getTouch(&tx, &ty)) {
            if (ENABLE_DEBUG_MODE)
                Serial.printf("Touch: X=%d Y=%d\n", tx, ty);

            sx += tx;
            sy += ty;
            count++;
        }
        delay(4);
    }

    if (count == 0) return false;

    tx = sx / count;
    ty = sy / count;

    return (tx >= x_target - scope &&
            tx <= x_target + scope &&
            ty >= y_target - scope &&
            ty <= y_target + scope);
}

bool isPressedCircle(uint16_t tx, uint16_t ty, int x_target, int y_target, int radius) {
    int dx = tx - x_target;
    int dy = ty - y_target;
    return (dx * dx + dy * dy) <= (radius * radius);
}

#endif