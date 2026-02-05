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

    // For troubleshooting, sample first 8 bytes of px_map
    char px_sample[32] = {0};
    int sample_len = ((area_w + 7) / 8) * area_h;
    if (sample_len > 8)
        sample_len = 8;
    for (int i = 0; i < sample_len; ++i)
    {
        snprintf(px_sample + i * 3, 4, "%02X ", px_map[i]);
    }

    snprintf(msg.body, 128, "px_map sample (first %d bytes): %s\r\n", sample_len, px_sample);
    msg.level = LOG_DEBUG;
    xQueueSend(debug_queue, (void *)&msg, 0);

    // Copy px_map to lvgl_buf at the area specified, handling byte alignment and bit inversion
    bool byte_aligned = (area->x1 % 8 == 0);
    int src_stride = (area_w + 7) / 8;
    size_t buf_size = stride_bytes * height;
    for (int y = 0; y < area_h; ++y)
    {
        int src_row = y * src_stride;
        int dst_row = (area->y1 + y) * stride_bytes;
        if (byte_aligned)
        {
            // Fast path: area->x1 is byte-aligned, can copy whole bytes
            int dst_byte = dst_row + (area->x1 / 8);
            for (int x_byte = 0; x_byte < src_stride; ++x_byte)
            {
                int src_idx = src_row + x_byte;
                int dst_idx = dst_byte + x_byte;
                if (dst_idx < 0 || (size_t)dst_idx >= buf_size)
                    continue;
                lvgl_buf[dst_idx] = ~px_map[src_idx];
            }
        }
        else
        {
            // Slow path: not byte-aligned, must copy bit-by-bit
            for (int x = 0; x < area_w; ++x)
            {
                int abs_x = area->x1 + x;
                int abs_y = area->y1 + y;
                int px_index = y * area_w + x;
                int px_byte = px_index / 8;
                int px_bit = 7 - (px_index % 8);
                uint8_t px_val = (px_map[px_byte] >> px_bit) & 0x1;
                uint8_t epd_val = px_val ? 0 : 1;
                int buf_index = abs_y * width + abs_x;
                int buf_byte = buf_index / 8;
                int buf_bit = 7 - (buf_index % 8);
                if (buf_byte < 0 || (size_t)buf_byte >= buf_size)
                    continue;
                if (epd_val)
                    lvgl_buf[buf_byte] |= (1 << buf_bit);
                else
                    lvgl_buf[buf_byte] &= ~(1 << buf_bit);
            }
        }
    }

    // Send the full buffer to the e-paper display
    epaper_drv->Display(lvgl_buf);
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

    lv_display_set_rotation(display, LV_DISPLAY_ROTATION_0);

    snprintf(msg.body, 128, "e-Paper display registered with LVGL\r\n");
    msg.level = LOG_DEBUG;
    xQueueSend(dbg_queue, (void *)&msg, 0);
}

uint32_t lvgl_tick_cb(void)
{
    return to_ms_since_boot(get_absolute_time());
}