#ifndef AERID_VERSION_H
#define AERID_VERSION_H
#include <stdio.h>

#define AERID_VERSION_MAJOR 0
#define AERID_VERSION_MINOR 0
#define AERID_VERSION_BUILD 0

#define AERID_VERSION_YEAR 2026

static inline const char *aerid_get_version_string()
{
    static char version_string[16];
    snprintf(version_string, sizeof(version_string), "%d.%d.%d", AERID_VERSION_MAJOR, AERID_VERSION_MINOR, AERID_VERSION_BUILD);
    return version_string;
}

#endif
