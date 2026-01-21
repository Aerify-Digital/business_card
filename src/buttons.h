#ifndef BUTTONS_H
#define BUTTONS_H
#include <Arduino.h>
#include "pindefs.h"

#define BUTTON_COUNT 7

const uint button_pins[BUTTON_COUNT] = {
    BTN_DPAD_UP_PIN, BTN_DPAD_DOWN_PIN, BTN_DPAD_LEFT_PIN,
    BTN_DPAD_RIGHT_PIN, BTN_DPAD_CENTER_PIN, BTN_A_PIN, BTN_B_PIN};

#define BTN_DEBOUNCE_MS 200

#endif