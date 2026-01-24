#ifndef SD_CARD_H

#define SD_CARD_H
#include <Arduino.h>
#include "SdFat.h"
#include "pindefs.h"
#include "message.h"

SdFat SD;

/**
 * @brief Initialize the SD card
 */
bool beginSD(SPIClassRP2040 &spi)
{
    pinMode(SD_CARD_CS_PIN, OUTPUT);
    digitalWrite(SD_CARD_CS_PIN, HIGH);
    return SD.begin(SdSpiConfig(SD_CARD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(50), &spi));
}

bool endSD()
{
    SD.end();
    return true;
}

/**
 * @brief Open a file on the SD card
 */
FsFile openSDFile(const char *filename, int mode = FILE_READ)
{
    return SD.open(filename, mode);
}

/**
 * @brief Check if the SD card is present by attempting to read a known file
 */
bool cardPresent(SPIClassRP2040 &spi)
{
    bool present = false;
    FsFile lock = openSDFile("/.aerlock", FILE_WRITE);
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
        FsFile check = openSDFile("/.aerlock", FILE_READ);
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

bool formatSD(SPIClassRP2040 &spi)
{
    bool result = SD.format();
    return result;
}

#endif