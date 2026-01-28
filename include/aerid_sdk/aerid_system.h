#ifndef AERID_SYSTEM_H
#define AERID_SYSTEM_H
#ifdef __cplusplus
extern "C"
{
#endif
#include <stdint.h>
#include <stddef.h>

    typedef struct
    {
        int major;
        int minor;
        int build;
    } aerid_version_t;

    // Get the system firmware version as a struct
    void aerid_system_get_firmware_version(aerid_version_t *version);

    // Get the system firmware version
    const char *aerid_system_get_firmware_version_string();

    // Get the major version number of the firmware
    int aerid_system_get_firmware_version_major();

    // Get the minor version number of the firmware
    int aerid_system_get_firmware_version_minor();

    // Get the build version number of the firmware
    int aerid_system_get_firmware_version_build();

    // Get the system uptime in milliseconds
    uint32_t aerid_system_get_uptime_ms();

    // Get the free and total RAM in bytes
    void aerid_system_get_ram_info(int *total_bytes, int *free_bytes);

    // Exit the current application and return to the launcher
    void aerid_system_exit_to_launcher();

    // Get the unique device identifier
    // The uid_buffer should be exactly 32 bytes to hold the resulting UID
    // Returns 0 on success, -1 on error
    int aerid_system_get_system_uid(uint8_t *uid_buffer);

#ifdef __cplusplus
}
#endif
#endif