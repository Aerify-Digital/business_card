#ifndef AERID_SDK_INTERNAL_H
#define AERID_SDK_INTERNAL_H
#include "aerid_sdk/aerid_sdk.h"
#include "aerid_launcher.h"
void handle_input_event(aerid_input_t input, aerid_input_event_t event);

void aerid_battery_update_status(int level, int charging, int voltage);

#endif