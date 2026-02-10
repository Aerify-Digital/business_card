/**
 *  @filename   :   epd2in66.cpp
 *  @brief      :   Implements for e-paper library
 *  @author     :   Waveshare
 *
 *  Copyright (C) Waveshare     July 29 2020
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documnetation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to  whom the Software is
 * furished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdlib.h>
#include "epd2in66.h"
#include "imagedata.h"

static const unsigned char WF_PARTIAL[159] PROGMEM =
    {
        0x00,
        0x40,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x80,
        0x80,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x40,
        0x40,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x80,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x0A,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x02,
        0x01,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x22,
        0x22,
        0x22,
        0x22,
        0x22,
        0x22,
        0x00,
        0x00,
        0x00,
        0x22,
        0x17,
        0x41,
        0xB0,
        0x32,
        0x36,
};

Epd2in66::~Epd2in66() {
};

Epd2in66::Epd2in66(SPIClassRP2040 *spi, SemaphoreHandle_t spi_mutex) : EpdIf(spi), spi_mutex(spi_mutex)
{
    reset_pin = EPD_RST_PIN;
    dc_pin = EPD_DC_PIN;
    cs_pin = SPI0_CS_PIN;
    busy_pin = EPD_BUSY_PIN;
    bufwidth = EPD_WIDTH / 8;
    bufheight = EPD_HEIGHT;
    this->spi_mutex = spi_mutex;
};

void Epd2in66::Reset(void)
{
    DigitalWrite(reset_pin, LOW); // module reset
    DelayMs(100);
    DigitalWrite(reset_pin, HIGH);
    DelayMs(200);
}

int Epd2in66::Init(char Mode)
{

    DelayMs(10);
    Reset();
    WaitUntilIdle();
    SendCommand(0x12); // soft  reset
    WaitUntilIdle();

    int count;
    if (Mode == FULL)
    {
        SendCommand(0x01); // Set display size and driver output control
        SendData((EPD_HEIGHT - 1) & 0xff);
        SendData(((EPD_HEIGHT - 1) >> 8) & 0xff);
        SendData(0x00);

        /*	Y increment, X increment	*/
        SendCommand(0x11);
        SendData(0x03);
        /*	Set RamX-address Start/End position	*/
        SendCommand(0x44);
        SendData(0x01);

        SendData((GetWidth() % 8 == 0) ? (GetWidth() / 8) : (GetWidth() / 8 + 1));
        /*	Set RamY-address Start/End position	*/
        SendCommand(0x45);
        SendData(0);
        SendData(0);
        SendData((GetHeight() & 0xff));
        SendData((GetHeight() & 0x100) >> 8);

        WaitUntilIdle();
    }
    else if (Mode == FAST)
    {
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
        Lut((unsigned char *)WF_PARTIAL);
        SendCommand(0x37);
        SendData(0x00);
        SendData(0x00);
        SendData(0x00);
        SendData(0x00);
        SendData(0x00);
        SendData(0x40);
        SendData(0x00);
        SendData(0x00);
        SendData(0x00);
        SendData(0x00);

        /* Y increment, X increment */
        SendCommand(0x11);
        SendData(0x03);
        /*	Set RamX-address Start/End position	*/
        SendCommand(0x44);
        SendData(0x01);
        SendData((GetWidth() % 8 == 0) ? (GetWidth() / 8) : (GetWidth() / 8 + 1));
        /*	Set RamY-address Start/End position	*/
        SendCommand(0x45);
        SendData(0);
        SendData(0);
        SendData((GetHeight() & 0xff));
        SendData((GetHeight() & 0x100) >> 8);

        SendCommand(0x3C);
        SendData(0x80);

        SendCommand(0x22);
        SendData(0xcf);
        SendCommand(0x20);
        WaitUntilIdle();
    }
    else
    {
        return -1;
    }

    return 0;
}

void Epd2in66::SendCommand(unsigned char command)
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

void Epd2in66::SendData(unsigned char data)
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

void Epd2in66::WaitUntilIdle(void)
{
    while (DigitalRead(busy_pin) != 0)
    {
        DelayMs(10);
    }
    DelayMs(200);
}

void Epd2in66::SetWindows(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend)
{
}

void Epd2in66::SetCursor(uint16_t Xstart, uint16_t Ystart)
{
}

void Epd2in66::Lut(unsigned char *lut)
{
    SendCommand(0x32);
    for (unsigned int i = 0; i < 153; i++)
    {
        SendData(lut[i]);
    }
    WaitUntilIdle();
}

void Epd2in66::Clear(void)
{
    int w, h;
    w = (EPD_WIDTH % 8 == 0) ? (EPD_WIDTH / 8) : (EPD_WIDTH / 8 + 1);
    h = EPD_HEIGHT;
    SendCommand(0x24);
    for (int j = 0; j < h; j++)
    {
        for (int i = 0; i < w; i++)
        {
            SendData(0xFF);
        }
    }

    // DISPLAY REFRESH
    SendCommand(0x22);
    SendData(0xf7);
    SendCommand(0x20);
    WaitUntilIdle();
}

void Epd2in66::Display(const unsigned char *frame_buffer)
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

void Epd2in66::Display1(const unsigned char *frame_buffer)
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

void Epd2in66::Display_Fast(const unsigned char *frame_buffer)
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

void Epd2in66::DisplayPartBaseImage(const unsigned char *frame_buffer)
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

        SendCommand(0x26);
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

void Epd2in66::DisplayPart(const unsigned char *frame_buffer)
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
    SendData(0xff);
    SendCommand(0x20);
    WaitUntilIdle();
}

void Epd2in66::ClearPart(void)
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

void Epd2in66::Sleep(void)
{
    SendCommand(0x10);
    SendData(0x01);
}
