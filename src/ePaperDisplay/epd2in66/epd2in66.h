#ifndef EPD2IN66_H
#define EPD2IN66_H

#include "../epdif.h"
#include "../epd_base.h"

#define EPD_WIDTH 152
#define EPD_HEIGHT 296

class Epd2in66 : public EpdIf, public EpdBase
{
public:
    int bufwidth;
    int bufheight;
    int count;

    Epd2in66(SPIClassRP2040 *spi, SemaphoreHandle_t spi_mutex);
    ~Epd2in66();
    int Init(char Mode) override;
    void SendCommand(unsigned char command) override;
    void SendData(unsigned char data) override;
    void WaitUntilIdle(void) override;
    void SetWindows(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend) override;
    void SetCursor(uint16_t Xstart, uint16_t Ystart) override;
    void Lut(unsigned char *lut) override;
    void Reset(void) override;
    void Clear(void) override;
    void Display(const unsigned char *frame_buffer) override;
    void Display1(const unsigned char *frame_buffer) override;
    void Display_Fast(const unsigned char *frame_buffer) override;
    void DisplayPartBaseImage(const unsigned char *frame_buffer) override;
    void DisplayPart(const unsigned char *frame_buffer) override;
    void ClearPart(void) override;
    void Sleep(void) override;
    int GetWidth() const override { return EPD_WIDTH; }
    int GetHeight() const override { return EPD_HEIGHT; }

private:
    unsigned int reset_pin;
    unsigned int dc_pin;
    unsigned int cs_pin;
    unsigned int busy_pin;
    SemaphoreHandle_t spi_mutex = nullptr;
};

#endif
