#include "lvgl_epd_glue.h"
#include "pico/time.h"
#include <cstring>
#include "message.h"

static EpdBase *epaper_drv = nullptr;
static lv_display_t *display = nullptr;
static QueueHandle_t debug_queue = nullptr;

alignas(8) static uint8_t lvgl_buf[EPD_BUFFER_MAX_SIZE];

void my_lvgl_log_cb(lv_log_level_t level, const char *buf)
{
    if (!debug_queue)
    {
        Serial.printf("LVGL LOG [%d]: %s\r\n", level, buf);
        return;
    }
    Message_t msg;
    msg.level = LOG_DEBUG;
    snprintf(msg.body, 128, "LVGL LOG: %s\r\n", buf);
    xQueueSend(debug_queue, (void *)&msg, 0);
}

void lvgl_epaper_set_driver(EpdBase *drv)
{
    epaper_drv = drv;
}

void lvgl_epaper_flush(lv_display_t *display, const lv_area_t *area, uint8_t *px_map)
{
    Message_t msg;
    snprintf(msg.body, 128, "lvgl_epaper_flush called\r\n");
    msg.level = LOG_DEBUG;
    xQueueSend(debug_queue, (void *)&msg, 0);

    if (!epaper_drv)
    {
        lv_disp_flush_ready(display);
        return;
    }

    int width = epaper_drv->GetWidth();
    int height = epaper_drv->GetHeight();
    int stride_bytes = (width + 7) / 8;
    const int area_w = area->x2 - area->x1 + 1;
    const int area_h = area->y2 - area->y1 + 1;

    snprintf(msg.body, 128, "Flush area: x1=%d, y1=%d, x2=%d, y2=%d, area_w=%d, area_h=%d\r\n",
             area->x1, area->y1, area->x2, area->y2, area_w, area_h);
    msg.level = LOG_DEBUG;
    xQueueSend(debug_queue, (void *)&msg, 0);

    // LVGL display is rotated 270 degrees compared to the e-Paper display, so width/height are swapped and x/y are swapped in the mapping
    if (area->x1 == 0 && area->y1 == 0 && area->x2 == height - 1 && area->y2 == width - 1)
    {
        snprintf(msg.body, 128, "Performing full update\r\n");
        msg.level = LOG_DEBUG;
        xQueueSend(debug_queue, (void *)&msg, 0);

        uint8_t *src_buf = px_map + 8;
        memcpy(lvgl_buf, px_map, EPD_BUFFER_MAX_SIZE);

        int src_w = lv_obj_get_width(lv_scr_act());
        int src_h = lv_obj_get_height(lv_scr_act());
        int dst_w = width;
        int dst_h = height;
        int src_stride = (src_w + 7) / 8;
        int dst_stride = (dst_w + 7) / 8;
        uint8_t rotated_buf[(dst_stride * dst_h)] = {0};

        for (int y = 0; y < dst_h; ++y)
        {
            for (int x = 0; x < dst_w; ++x)
            {
                int src_x = src_w - 1 - y;
                int src_y = x;

                int src_byte = src_y * src_stride + (src_x / 8);
                int src_bit = 7 - (src_x % 8);
                bool pixel_on = (src_buf[src_byte] >> src_bit) & 0x1;

                int dst_byte = y * dst_stride + (x / 8);
                int dst_bit = 7 - (x % 8);

                if (pixel_on)
                    rotated_buf[dst_byte] |= (1 << dst_bit); // 1 = black
                else
                    rotated_buf[dst_byte] &= ~(1 << dst_bit); // 0 = white
            }
        }

        epaper_drv->Display(rotated_buf);
        lv_disp_flush_ready(display);
        return;
    }
    // TODO: Partial update - copy the area from px_map to the correct location in lvgl_buf, then call a partial update function on epaper_drv if available
    epaper_drv->Display(px_map + 8); // TODO: change to partial update function if available
    lv_disp_flush_ready(display);
}

void lvgl_epaper_register_display(QueueHandle_t dbg_queue)
{
    Message_t msg;
    snprintf(msg.body, 128, "Registering e-Paper display with LVGL\r\n");
    msg.level = LOG_DEBUG;
    xQueueSend(dbg_queue, (void *)&msg, 0);
    debug_queue = dbg_queue;
#if LV_USE_LOG
    lv_log_register_print_cb(my_lvgl_log_cb);
#endif
    if (!epaper_drv)
    {
        snprintf(msg.body, 128, "No e-Paper driver set, cannot register display\r\n");
        msg.level = LOG_ERROR;
        xQueueSend(dbg_queue, (void *)&msg, 0);
        return;
    }

    int width = epaper_drv->GetWidth();
    int height = epaper_drv->GetHeight();
    int stride_bytes = (width + 7) / 8;
    int buf_size = stride_bytes * height + 8;
    snprintf(msg.body, 128, "Register display: width=%d height=%d stride_bytes=%d buf_size=%d", width, height, stride_bytes, buf_size);
    msg.level = LOG_DEBUG;
    xQueueSend(dbg_queue, (void *)&msg, 0);
    display = lv_display_create(width, height);
    memset(lvgl_buf, 0xFF, buf_size);
    lv_display_set_buffers(display, lvgl_buf, NULL, buf_size, LV_DISPLAY_RENDER_MODE_FULL);

    lv_display_set_flush_cb(display, lvgl_epaper_flush);

    lv_display_set_default(display);

    lv_display_set_rotation(display, LV_DISP_ROTATION_270);

    snprintf(msg.body, 128, "e-Paper display registered with LVGL\r\n");
    msg.level = LOG_DEBUG;
    xQueueSend(dbg_queue, (void *)&msg, 0);
}

uint32_t lvgl_tick_cb(void)
{
    return to_ms_since_boot(get_absolute_time());
}