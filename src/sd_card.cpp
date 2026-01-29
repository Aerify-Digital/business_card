#ifndef SD_CARD_H

#define SD_CARD_H
#include <Arduino.h>
#include "SdFat.h"
#include "pindefs.h"
#include "message.h"

SdFat SD;

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

FsFile openSDFile(const char *filename, int mode = FILE_READ)
{
    return SD.open(filename, mode);
}

int readSDFile(const char *filename, uint8_t *buffer, size_t max_len)
{
    FsFile file = openSDFile(filename, FILE_READ);
    if (!file)
    {
        return -1;
    }
    size_t read = file.read(buffer, max_len);
    file.close();
    return read;
}

int writeSDFile(const char *filename, const uint8_t *data, size_t len)
{
    FsFile file = openSDFile(filename, FILE_WRITE);
    if (!file)
    {
        return -1;
    }
    size_t written = file.write(data, len);
    file.close();
    return written;
}

bool deleteSDFile(const char *filename)
{
    return SD.remove(filename);
}

bool renameSDFile(const char *old_filename, const char *new_filename)
{
    return SD.rename(old_filename, new_filename);
}

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