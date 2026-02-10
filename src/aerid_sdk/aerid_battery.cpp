#include "aerid_sdk/aerid_battery.h"

static volatile int battery_level = 0;
static volatile int battery_charging = 0;
static volatile int battery_voltage = 0;
static aerid_battery_status_callback_t status_callback = nullptr;

void aerid_battery_update_status(int level, int charging, int voltage)
{
    battery_level = level;
    battery_charging = charging;
    battery_voltage = voltage;

    if (status_callback)
        status_callback(level, charging);
}

int aerid_battery_get_level()
{
    return battery_level;
}

int aerid_battery_is_charging()
{
    return battery_charging;
}

int aerid_battery_get_voltage()
{
    return battery_voltage;
}

void aerid_battery_register_status_callback(aerid_battery_status_callback_t callback)
{
    status_callback = callback;
}

void aerid_battery_clear_callback()
{
    status_callback = nullptr;
}