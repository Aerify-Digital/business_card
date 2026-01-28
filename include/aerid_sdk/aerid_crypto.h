#ifndef AERID_CRYPTO_H
#define AERID_CRYPTO_H
#ifdef __cplusplus
extern "C"
{
#endif
#include <stdint.h>
#include <stddef.h>

    typedef enum
    {
        AERID_CRYPTO_ATECC508A,
        AERID_CRYPTO_ATSHA204A,
        AERID_CRYPTO_COUNT
    } aerid_crypto_device_t;

    // Generate random bytes using the specified crypto device
    // Returns 0 on success, -1 on error
    int aerid_crypto_random_bytes(aerid_crypto_device_t device, uint8_t *buffer, size_t len);

#ifdef __cplusplus
}
#endif
#endif