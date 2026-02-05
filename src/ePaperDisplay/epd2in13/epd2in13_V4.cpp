#include <stdlib.h>
#include "epd2in13_V4.h"

Epd2in13::~Epd2in13() {
};

Epd2in13::Epd2in13(SPIClassRP2040 *spi, SemaphoreHandle_t spi_mutex) : EpdIf(spi), spi_mutex(spi_mutex)
{
    reset_pin = EPD_RST_PIN;
    dc_pin = EPD_DC_PIN;
    cs_pin = SPI0_CS_PIN;
    busy_pin = EPD_BUSY_PIN;
    bufwidth = EPD_WIDTH / 8;
    bufheight = EPD_HEIGHT;
    this->spi_mutex = spi_mutex;
};

int Epd2in13::Init(char Mode)
{

    Reset();

    int count;
    if (Mode == FULL)
    {
        WaitUntilIdle();
        SendCommand(0x12); // soft reset
        WaitUntilIdle();

        SendCommand(0x01); // Driver output control
        SendData(0xF9);
        SendData(0x00);
        SendData(0x00);

        SendCommand(0x11); // data entry mode
        SendData(0x03);

        SetWindows(0, 0, EPD_WIDTH - 1, EPD_HEIGHT - 1);
        SetCursor(0, 0);

        SendCommand(0x3C); // BorderWavefrom
        SendData(0x05);

        SendCommand(0x21); //  Display update control
        SendData(0x00);
        SendData(0x80);

        SendCommand(0x18); // Read built-in temperature sensor
        SendData(0x80);
        WaitUntilIdle();
    }
    else if (Mode == FAST)
    {
        WaitUntilIdle();
        SendCommand(0x12); // soft reset
        WaitUntilIdle();

        SendCommand(0x18); // Read built-in temperature sensor
        SendData(0x80);

        SendCommand(0x11); // data entry mode
        SendData(0x03);

        SetWindows(0, 0, EPD_WIDTH - 1, EPD_HEIGHT - 1);
        SetCursor(0, 0);

        SendCommand(0x22); // Load temperature value
        SendData(0xB1);
        SendCommand(0x20);
        WaitUntilIdle();

        SendCommand(0x1A); //  Write to temperature register
        SendData(0x64);
        SendData(0x00);

        SendCommand(0x22); //  Load temperature value
        SendData(0x91);
        SendCommand(0x20);
        WaitUntilIdle();
    }
    else if (Mode == PART)
    {
        DigitalWrite(reset_pin, LOW); // module reset
        DelayMs(1);
        DigitalWrite(reset_pin, HIGH);

        SendCommand(0x3C); // BorderWavefrom
        SendData(0x80);

        SendCommand(0x01); // Driver output control
        SendData(0xF9);
        SendData(0x00);
        SendData(0x00);

        SendCommand(0x11); // data entry mode
        SendData(0x03);

        SetWindows(0, 0, EPD_WIDTH - 1, EPD_HEIGHT - 1);
        SetCursor(0, 0);
    }
    else
    {
        return -1;
    }

    return 0;
}

void Epd2in13::SendCommand(unsigned char command)
{
    if (spi_mutex)
    {
        if (xSemaphoreTake(spi_mutex, portMAX_DELAY) == pdTRUE)
        {
            DigitalWrite(dc_pin, LOW);
            SpiTransfer(command);
            xSemaphoreGive(spi_mutex);
        }
    }
    else
    {
        DigitalWrite(dc_pin, LOW);
        SpiTransfer(command);
    }
}

void Epd2in13::SendData(unsigned char data)
{
    if (spi_mutex)
    {
        if (xSemaphoreTake(spi_mutex, portMAX_DELAY) == pdTRUE)
        {
            DigitalWrite(dc_pin, HIGH);
            SpiTransfer(data);
            xSemaphoreGive(spi_mutex);
        }
    }
    else
    {
        DigitalWrite(dc_pin, HIGH);
        SpiTransfer(data);
    }
}

void Epd2in13::WaitUntilIdle(void)
{
    while (1)
    { // LOW: idle, HIGH: busy
        if (DigitalRead(busy_pin) == 0)
            break;
        DelayMs(10);
    }
}

void Epd2in13::SetWindows(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend)
{
    SendCommand(0x44); // SET_RAM_X_ADDRESS_START_END_POSITION
    SendData((Xstart >> 3) & 0xFF);
    SendData((Xend >> 3) & 0xFF);

    SendCommand(0x45); // SET_RAM_Y_ADDRESS_START_END_POSITION
    SendData(Ystart & 0xFF);
    SendData((Ystart >> 8) & 0xFF);
    SendData(Yend & 0xFF);
    SendData((Yend >> 8) & 0xFF);
}

void Epd2in13::SetCursor(uint16_t Xstart, uint16_t Ystart)
{
    SendCommand(0x4E); // SET_RAM_X_ADDRESS_COUNTER
    SendData(Xstart & 0xFF);

    SendCommand(0x4F); // SET_RAM_Y_ADDRESS_COUNTER
    SendData(Ystart & 0xFF);
    SendData((Ystart >> 8) & 0xFF);
}

void Epd2in13::Lut(unsigned char *lut)
{
    SendCommand(0x32);
    for (int i = 0; i < 153; i++)
    {
        SendData(lut[i]);
    }
}

void Epd2in13::Reset(void)
{
    DigitalWrite(reset_pin, HIGH);
    DelayMs(20);
    DigitalWrite(reset_pin, LOW); // module reset
    DelayMs(2);
    DigitalWrite(reset_pin, HIGH);
    DelayMs(20);
    this->count = 0;
}

void Epd2in13::Clear(void)
{
    int w, h;
    w = (EPD_WIDTH % 8 == 0) ? (EPD_WIDTH / 8) : (EPD_WIDTH / 8 + 1);
    h = EPD_HEIGHT;
    SendCommand(0x24);
    for (int j = 0; j < h; j++)
    {
        for (int i = 0; i < w; i++)
        {
            SendData(0xff);
        }
    }

    // DISPLAY REFRESH
    SendCommand(0x22);
    SendData(0xf7);
    SendCommand(0x20);
    WaitUntilIdle();
}

void Epd2in13::Display(const unsigned char *frame_buffer)
{
    int w = (EPD_WIDTH % 8 == 0) ? (EPD_WIDTH / 8) : (EPD_WIDTH / 8 + 1);
    int h = EPD_HEIGHT;

    if (frame_buffer != NULL)
    {
        SendCommand(0x24);
        for (int j = 0; j < h; j++)
        {
            for (int i = 0; i < w; i++)
            {
                SendData(pgm_read_byte(&frame_buffer[i + j * w]));
            }
        }
    }

    // DISPLAY REFRESH
    SendCommand(0x22);
    SendData(0xf7);
    SendCommand(0x20);
    WaitUntilIdle();
}

void Epd2in13::Display1(const unsigned char *frame_buffer)
{
    if (this->count == 0)
    {
        SendCommand(0x24);
        this->count++;
    }
    else if (this->count > 0 && this->count < 4)
    {
        this->count++;
    }
    for (int i = 0; i < this->bufwidth * this->bufheight; i++)
    {
        SendData(frame_buffer[i]);
    }
    if (this->count == 4)
    {
        SendCommand(0x22);
        SendData(0xf7);
        SendCommand(0x20);
        WaitUntilIdle();
        this->count = 0;
    }
}

void Epd2in13::Display_Fast(const unsigned char *frame_buffer)
{
    int w = (EPD_WIDTH % 8 == 0) ? (EPD_WIDTH / 8) : (EPD_WIDTH / 8 + 1);
    int h = EPD_HEIGHT;

    if (frame_buffer != NULL)
    {
        SendCommand(0x24);
        for (int j = 0; j < h; j++)
        {
            for (int i = 0; i < w; i++)
            {
                SendData(pgm_read_byte(&frame_buffer[i + j * w]));
            }
        }
    }

    // DISPLAY REFRESH
    SendCommand(0x22);
    SendData(0xC7);
    SendCommand(0x20);
    WaitUntilIdle();
}

void Epd2in13::DisplayPartBaseImage(const unsigned char *frame_buffer)
{
    // Use stride_bytes for correct row indexing (matches LVGL and most image generators)
    int stride_bytes = (EPD_WIDTH + 7) / 8;
    int h = EPD_HEIGHT;

    if (frame_buffer != NULL)
    {
        SendCommand(0x24);
        for (int j = 0; j < h; j++)
        {
            for (int i = 0; i < stride_bytes; i++)
            {
                SendData(pgm_read_byte(&frame_buffer[i + j * stride_bytes]));
            }
        }

        SendCommand(0x26);
        for (int j = 0; j < h; j++)
        {
            for (int i = 0; i < stride_bytes; i++)
            {
                SendData(pgm_read_byte(&frame_buffer[i + j * stride_bytes]));
            }
        }
    }

    // DISPLAY REFRESH
    SendCommand(0x22);
    SendData(0xf7);
    SendCommand(0x20);
    WaitUntilIdle();
}

void Epd2in13::DisplayPart(const unsigned char *frame_buffer)
{
    // Use stride_bytes for correct row indexing (matches LVGL and most image generators)
    int stride_bytes = (EPD_WIDTH + 7) / 8;
    int h = EPD_HEIGHT;

    if (frame_buffer != NULL)
    {
        SendCommand(0x24);
        for (int j = 0; j < h; j++)
        {
            for (int i = 0; i < stride_bytes; i++)
            {
                SendData(pgm_read_byte(&frame_buffer[i + j * stride_bytes]));
            }
        }
    }

    // DISPLAY REFRESH
    SendCommand(0x22);
    SendData(0xff);
    SendCommand(0x20);
    WaitUntilIdle();
}

void Epd2in13::ClearPart(void)
{
    int w, h;
    w = (EPD_WIDTH % 8 == 0) ? (EPD_WIDTH / 8) : (EPD_WIDTH / 8 + 1);
    h = EPD_HEIGHT;
    SendCommand(0x24);
    for (int j = 0; j < h; j++)
    {
        for (int i = 0; i < w; i++)
        {
            SendData(0xff);
        }
    }

    // DISPLAY REFRESH
    SendCommand(0x22);
    SendData(0x0f);
    SendCommand(0x20);
    WaitUntilIdle();
}

void Epd2in13::Sleep()
{
    SendCommand(0x10); // enter deep sleep
    SendData(0x01);
    DelayMs(200);

    DigitalWrite(reset_pin, LOW);
}
