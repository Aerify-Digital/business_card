#ifndef EPD_H
#define EPD_H

#ifdef EPD_2IN13
#include "epd2in13/epd2in13_V4.h"
#include "epd2in13/imagedata.h"
#elif defined(EPD_2IN66)
#include "epd2in66/epd2in66.h"
#include "epd2in66/imagedata.h"
#else
#error "No e-Paper display selected"
#endif
#define COLORED 0
#define UNCOLORED 1
#endif