#ifndef ARECC508A_H
#define ARECC508A_H
#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_ATECCX08a_Arduino_Library.h>

#ifndef ATECC508A_ADDRESS
#define ATECC508A_ADDRESS 0x60
#endif

ATECCX08A atecc;

bool init_atecc508a(TwoWire &i2cPort)
{

    if (!atecc.begin(ATECC508A_ADDRESS, i2cPort))
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool read_atecc508a_serial_number(uint8_t *serial_number_buffer)
{
    atecc.readConfigZone(false);
    memcpy(serial_number_buffer, atecc.serialNumber, SERIAL_NUMBER_SIZE);
    return true;
}

#endif