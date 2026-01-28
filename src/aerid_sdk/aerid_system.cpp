#include "aerid_sdk/aerid_system.h"
#include "aerid_launcher.h"
#include "aerid_version.h"
#include <Arduino.h>

void aerid_system_get_firmware_version(aerid_version_t *version)
{
    if (version)
    {
        version->major = AERID_VERSION_MAJOR;
        version->minor = AERID_VERSION_MINOR;
        version->build = AERID_VERSION_BUILD;
    }
}

const char *aerid_system_get_firmware_version_string()
{
    return aerid_get_version_string();
}

int aerid_system_get_firmware_version_major()
{
    return AERID_VERSION_MAJOR;
}

int aerid_system_get_firmware_version_minor()
{
    return AERID_VERSION_MINOR;
}

int aerid_system_get_firmware_version_build()
{
    return AERID_VERSION_BUILD;
}

uint32_t aerid_system_get_uptime_ms()
{
    return to_ms_since_boot(get_absolute_time());
}

void aerid_system_get_ram_info(int *total_bytes, int *free_bytes)
{

    if (total_bytes)
        *total_bytes = rp2040.getTotalHeap();
    if (free_bytes)
        *free_bytes = rp2040.getFreeHeap();
}

void aerid_system_exit_to_launcher()
{
    return aerid_launcher_return();
}

int aerid_system_get_system_uid(uint8_t *uid_buffer)
{
    pico_unique_board_id_t board_id;
    pico_get_unique_board_id(&board_id);
    // TODO: Implement system UID retrieval for ecc508a and atsha204a devices
    // via their freertos queues. sha256 hash (board uid, ecc508a uid, atsha204a uid)
    // to produce a 32 byte uid. For now, just return the board unique id.
    if (uid_buffer)
    {
        memcpy(uid_buffer, board_id.id, PICO_UNIQUE_BOARD_ID_SIZE_BYTES);
        // Zero out the rest of the buffer if it's larger than the board ID size
        memset(uid_buffer + PICO_UNIQUE_BOARD_ID_SIZE_BYTES, 0, 32 - PICO_UNIQUE_BOARD_ID_SIZE_BYTES);
        return 0;
    }
    return -1;
}