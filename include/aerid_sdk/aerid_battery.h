#ifndef AERID_BATTERY_H
#define AERID_BATTERY_H
#ifdef __cplusplus
extern "C"
{
#endif

    // Get the current battery level as a percentage (0-100)
    int aerid_battery_get_level();

    // Check if the device is currently charging
    int aerid_battery_is_charging();

    // Get the battery voltage in millivolts
    int aerid_battery_get_voltage();

    typedef void (*aerid_battery_status_callback_t)(int level, int charging);

    // Register a callback function for battery status changes
    void aerid_battery_register_status_callback(aerid_battery_status_callback_t callback);

    // Clear the registered battery status callback
    void aerid_battery_clear_callback();

#ifdef __cplusplus
}
#endif
#endif