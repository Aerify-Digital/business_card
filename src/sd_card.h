#ifndef SD_CARD_H

#define SD_CARD_H
#include <Arduino.h>
#include <SD.h>
#include "FS.h"
#include "pindefs.h"

/**
 * @brief Initialize the SD card
 */
bool beginSD(SPIClassRP2040 &spi)
{
    pinMode(SD_CARD_CS_PIN, OUTPUT);
    digitalWrite(SD_CARD_CS_PIN, HIGH);
    return SD.begin(SD_CARD_CS_PIN, spi);
}

bool endSD()
{
    SD.end(false);
    return true;
}

/**
 * @brief Open a file on the SD card
 */
File openSDFile(const char *filename, int mode = FILE_READ)
{
    return SD.open(filename, mode);
}

/**
 * @brief Check if the SD card is present by attempting to read a known file
 */
bool cardPresent(SPIClassRP2040 &spi)
{
    bool present = false;
    File lock = openSDFile("/.aerlock", FILE_WRITE);
    bool sd_removed = false;
    if (!lock)
    {
        sd_removed = true;
        present = false;
    }
    else
    {
        lock.seek(0);
        lock.print("lock");
        lock.close();
        File check = openSDFile("/.aerlock", FILE_READ);
        if (!check)
        {
            sd_removed = true;
        }
        else
        {
            char buf[5] = {0};
            int n = check.readBytes(buf, 4);
            check.close();
            if (n != 4 || strncmp(buf, "lock", 4) != 0)
            {
                sd_removed = true;
            }
            else
            {
                present = true;
            }
        }
    }
    if (sd_removed)
    {
        present = false;
        endSD();
        if (beginSD(spi))
        {
            present = true;
        }
    }
    return present;
}

#endif