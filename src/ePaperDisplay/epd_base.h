#pragma once
#include <stdint.h>

#define FULL 0
#define FAST 1
#define PART 2

#define EPD_BUFFER_MAX_SIZE 5632 // (5624 + 8) Largest buffer size for 2.66" e-paper will work for smaller displays also

class EpdBase
{
public:
    virtual ~EpdBase() {}
    virtual int Init(char Mode) = 0;
    virtual void SendCommand(unsigned char command) = 0;
    virtual void SendData(unsigned char data) = 0;
    virtual void WaitUntilIdle(void) = 0;
    virtual void SetWindows(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend) = 0;
    virtual void SetCursor(uint16_t Xstart, uint16_t Ystart) = 0;
    virtual void Lut(unsigned char *lut) = 0;
    virtual void Reset(void) = 0;
    virtual void Clear() = 0;
    virtual void Display(const unsigned char *frame_buffer) = 0;
    virtual void Display1(const unsigned char *frame_buffer) = 0;
    virtual void Display_Fast(const unsigned char *frame_buffer) = 0;
    virtual void DisplayPartBaseImage(const unsigned char *frame_buffer) = 0;
    virtual void DisplayPart(const unsigned char *frame_buffer) = 0;
    virtual void ClearPart(void) = 0;
    virtual void Sleep(void) = 0;
    virtual int GetWidth() const = 0;
    virtual int GetHeight() const = 0;
};