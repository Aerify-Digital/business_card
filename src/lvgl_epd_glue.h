#ifndef LVGL_EPD_GLUE_H
#define LVGL_EPD_GLUE_H
#include <lvgl.h>
#include "ePaperDisplay/epd_base.h"
#include "FreeRTOS.h"
#include "queue.h"

// Set the e-paper driver to use (Epd2in13 or Epd2in66)
void lvgl_epaper_set_driver(EpdBase *drv);

// LVGL flush callback
void lvgl_epaper_flush(lv_display_t *display, const lv_area_t *area, uint8_t *px_map);

// Register the e-paper display with LVGL
void lvgl_epaper_register_display(QueueHandle_t dbg_queue);

// LVGL tick callback
uint32_t lvgl_tick_cb(void);

#endif