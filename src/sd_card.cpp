#include "sd_card.h"
#include <Arduino.h>
#include "SdFat.h"
#include "pindefs.h"
#include "message.h"

SdFat SD;

volatile bool sd_mounted = false;

bool beginSD(SPIClassRP2040 &spi)
{
    pinMode(SD_CARD_CS_PIN, OUTPUT);
    digitalWrite(SD_CARD_CS_PIN, HIGH);
    bool result = SD.begin(SdSpiConfig(SD_CARD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(50), &spi));
    sd_mounted = result;
    return result;
}

bool endSD()
{
    SD.end();
    sd_mounted = false;
    return true;
}

FsFile openSDFile(const char *filename, int mode)
{
    return SD.open(filename, mode);
}

int readSDFile(const char *filename, uint8_t *buffer, size_t max_len)
{
    if (max_len > SD_RW_MAX_CHUNK)
        max_len = SD_RW_MAX_CHUNK;
    FsFile file = openSDFile(filename, FILE_READ);
    if (!file)
    {
        return -1;
    }
    if (file.size() > SD_FILE_MAX_SIZE)
    {
        file.close();
        return SD_ERR_TOO_LARGE; // file too large
    }
    size_t read = file.read(buffer, max_len);
    file.close();
    return read;
}

int writeSDFile(const char *filename, const uint8_t *data, size_t len)
{
    if (len > SD_RW_MAX_CHUNK)
        return -1;
    FsFile file = openSDFile(filename, FILE_WRITE);
    if (!file)
    {
        return -1;
    }
    // Overwrite, so file size after write is len
    if (len > SD_FILE_MAX_SIZE)
    {
        file.close();
        return SD_ERR_TOO_LARGE; // would exceed max file size
    }
    size_t written = file.write(data, len);
    file.close();
    return written;
}

int appendSDFile(const char *filename, const uint8_t *data, size_t len)
{
    if (len > SD_RW_MAX_CHUNK)
        return -1;
    FsFile file = openSDFile(filename, FILE_WRITE);
    if (!file)
    {
        return -1;
    }
    size_t cur_size = file.size();
    if (cur_size + len > SD_FILE_MAX_SIZE)
    {
        file.close();
        return SD_ERR_TOO_LARGE; // would exceed max file size
    }
    file.seek(cur_size);
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
    sd_mounted = present;
    return present;
}

bool formatSD(SPIClassRP2040 &spi)
{
    bool result = SD.format();
    return result;
}
