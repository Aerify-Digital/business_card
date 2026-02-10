#ifndef AERID_LAUNCHER_H
#define AERID_LAUNCHER_H
#include "aerid_sdk/aerid_sdk.h"
#include <stdint.h>
#include <stddef.h>
#include <FreeRTOS.h>
#include <queue.h>
#include "message.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        char app_id[32];
        char name[64];
        char version[16];
        char author[64];
        char description[128];
    } aerid_app_metadata_t;

    // Initialize the AerID launcher system
    void aerid_launcher_init(QueueHandle_t debug_queue = nullptr);

    // Launch the AerID launcher interface
    void aerid_launcher_launch();

    // Launch a specific app by its identifier
    // Returns 0 on success, -1 on failure
    int aerid_launcher_launch_app(const char *app_id);

    // Return to the AerID launcher from an app
    void aerid_launcher_return();

    // Check if the AerID launcher is currently active
    // Returns 1 if active, 0 if not
    int aerid_launcher_is_active();

    // List available apps
    // Fills the provided array with app identifiers (each max 64 bytes)
    // Returns the number of apps found, or -1 on error
    int aerid_launcher_list_apps(char app_ids[][64], size_t max_apps);

    // Get the currently active app identifier
    // Fills the provided buffer with the app identifier (max 64 bytes)
    // Returns 0 on success, -1 on error
    int aerid_launcher_get_active_app(char *app_id, size_t max_len);

    // Get metadata for a specific app
    // Fills the provided metadata structure
    // Returns 0 on success, -1 on error
    int aerid_launcher_get_metadata(const char *app_id, aerid_app_metadata_t *metadata);

    // Get metadata for a specific app by its identifier
    // Fills the provided metadata structure
    // Returns 0 on success, -1 on error
    int aerid_launcher_get_metadata_by_id(const char *app_id, aerid_app_metadata_t *metadata);

    // Callback function type for app launch events
    typedef void (*aerid_launcher_app_launch_callback_t)(const char *app_id);

    // Register a callback function for app launch events
    void aerid_launcher_register_app_launch_callback(aerid_launcher_app_launch_callback_t callback);

    void aerid_launcher_tick();

#ifdef __cplusplus
}
#endif
#endif