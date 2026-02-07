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
    msg.level = LOG_ERROR; // Or map LVGL level to your LogLevel_t
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

    // Debug: print first 16 bytes of px_map for this flush
    {
        char debug_buf[128];
        int debug_len = 0;
        debug_len += snprintf(debug_buf + debug_len, sizeof(debug_buf) - debug_len, "px_map: ");
        int px_map_len = ((area->x2 - area->x1 + 1 + 7) / 8) * (area->y2 - area->y1 + 1);
        for (int i = 0; i < 16 && i < px_map_len; ++i)
        {
            debug_len += snprintf(debug_buf + debug_len, sizeof(debug_buf) - debug_len, "%02X ", px_map[i]);
        }
        debug_len += snprintf(debug_buf + debug_len, sizeof(debug_buf) - debug_len, "\r\n");
        msg.level = LOG_DEBUG;
        snprintf(msg.body, 128, "%s", debug_buf);
        xQueueSend(debug_queue, (void *)&msg, 0);
    }
    if (!epaper_drv)
    {
        lv_disp_flush_ready(display);
        return;
    }

    // LVGL provides px_map as a 1bpp packed buffer for the area only.
    // We must update only the area in our full-frame buffer (lvgl_buf).
    const int width = epaper_drv->GetWidth();
    const int height = epaper_drv->GetHeight();
    const int area_w = area->x2 - area->x1 + 1;
    const int area_h = area->y2 - area->y1 + 1;

    int stride_bytes = (width + 7) / 8;
    snprintf(msg.body, 128, "Flush area: x1=%d, y1=%d, x2=%d, y2=%d, area_w=%d, area_h=%d\r\n",
             area->x1, area->y1, area->x2, area->y2, area_w, area_h);
    msg.level = LOG_DEBUG;
    xQueueSend(debug_queue, (void *)&msg, 0);

    if (area->x1 == 0 && area->y1 == 0 && area->x2 == width - 1 && area->y2 == height - 1)
    {
        epaper_drv->Display(px_map + 8);
        lv_disp_flush_ready(display);
        return;
    }
    else
    {
        // Partial update, we need to copy row by row into the correct position in lvgl_buf
        for (int row = 0; row < area_h; ++row)
        {
            int dest_row = area->y1 + row;
            if (dest_row >= height)
                break; // Safety check

            int dest_byte_index = (area->x1 / 8) + dest_row * stride_bytes;
            int src_byte_index = row * ((area_w + 7) / 8);

            // Copy the relevant bytes for this row
            for (int col_byte = 0; col_byte < (area_w + 7) / 8; ++col_byte)
            {
                if (dest_byte_index + col_byte >= stride_bytes * height)
                    break; // Safety check
                lvgl_buf[dest_byte_index + col_byte] = px_map[src_byte_index + col_byte];
            }
        }
    }

    // Send only the pixel data (skip palette) to the e-paper display
    epaper_drv->Display(lvgl_buf + 8);
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

    // Create LVGL display
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

    // Set flush callback
    lv_display_set_flush_cb(display, lvgl_epaper_flush);

    lv_display_set_default(display);

    snprintf(msg.body, 128, "e-Paper display registered with LVGL\r\n");
    msg.level = LOG_DEBUG;
    xQueueSend(dbg_queue, (void *)&msg, 0);
}

uint32_t lvgl_tick_cb(void)
{
    return to_ms_since_boot(get_absolute_time());
}