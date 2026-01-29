#ifndef SD_CARD_H

#define SD_CARD_H
#include <Arduino.h>
#include "SdFat.h"
#include "pindefs.h"
#include "message.h"

static QueueHandle_t sdQueue = NULL;

typedef enum
{
    SD_OP_READ,
    SD_OP_WRITE,
    SD_OP_DELETE,
    SD_OP_RENAME
} sd_op_t;

typedef struct
{
    sd_op_t op;
    char filename[64];
    uint8_t *buffer;
    size_t length;
    bool result;
    SemaphoreHandle_t done;
} sd_request_t;

/**
 * @brief Initialize the SD card
 */
bool beginSD(SPIClassRP2040 &spi);

/**
 * @brief End use of the SD card
 */
bool endSD();

/**
 * @brief Open a file on the SD card
 */
FsFile openSDFile(const char *filename, int mode = FILE_READ);

/**
 * @brief Read a file from the SD card into a buffer
 */
int readSDFile(const char *filename, uint8_t *buffer, size_t max_len);

/**
 * @brief Write data to a file on the SD card
 */
int writeSDFile(const char *filename, const uint8_t *data, size_t len);

/**
 * @brief Delete a file from the SD card
 */
bool deleteSDFile(const char *filename);

/**
 * @brief Rename a file on the SD card
 */
bool renameSDFile(const char *old_filename, const char *new_filename);

/**
 * @brief Check if the SD card is present by attempting to read a known file
 */
bool cardPresent(SPIClassRP2040 &spi);

/**
 * @brief Format the SD card
 */
bool formatSD(SPIClassRP2040 &spi);

#endif