#include "aerid_launcher.h"
#include "lvgl_epd_glue.h"
#include <vector>
#include <lvgl.h>
#include <queue.h>

static QueueHandle_t debug_queue_handle = nullptr;

static aerid_input_callback_t launcher_input_callback = nullptr;

static std::vector<aerid_app_metadata_t> registered_apps;

static TickType_t last_display_tick = 0;

static volatile bool battery_update_needed = false;
static volatile int latest_battery_level = 0;
static volatile int latest_battery_charging = 0;

static lv_img_dsc_t img_dsc;
static lv_obj_t *battery_icon = nullptr;
static uint8_t battery_icon_data[8 + ((50 + 7) / 8) * 24]; // move to file scope

void update_battery_icon(int level, int charging)
{
    const uint16_t img_stride = (50 + 7) / 8;
    const size_t img_size = img_stride * 24;

    memcpy(battery_icon_data, LVGL_PALETTE, 8);

    static const unsigned char *img_data = nullptr;

    if (charging)
    {
        if (level >= 88)
            img_data = BATTERY_CHARGE_100_DATA;
        else if (level >= 63)
            img_data = BATTERY_CHARGE_75_DATA;
        else if (level >= 38)
            img_data = BATTERY_CHARGE_50_DATA;
        else if (level >= 13)
            img_data = BATTERY_CHARGE_25_DATA;
        else
            img_data = BATTERY_CHARGE_0_DATA;
    }
    else
    {
        if (level >= 88)
            img_data = BATTERY_100_DATA;
        else if (level >= 63)
            img_data = BATTERY_75_DATA;
        else if (level >= 38)
            img_data = BATTERY_50_DATA;
        else if (level >= 13)
            img_data = BATTERY_25_DATA;
        else
            img_data = BATTERY_0_DATA;
    }

    for (uint16_t row = 0; row < 24; ++row)
    {
        for (uint16_t col_byte = 0; col_byte < img_stride; ++col_byte)
        {
            size_t src_idx = row * img_stride + col_byte;
            size_t dst_idx = 8 + row * img_stride + col_byte;
            battery_icon_data[dst_idx] = pgm_read_byte(&img_data[src_idx]);
        }
    }

    img_dsc.header =
        {LV_IMAGE_HEADER_MAGIC,       // header.magic
         LV_COLOR_FORMAT_I1,          // header.cf
         0,                           // header.flags
         50,                          // header.width
         24,                          // header.height
         img_stride,                  // header.stride
         0};                          // header.reserved
    img_dsc.data_size = img_size + 8; // data length
    img_dsc.data = battery_icon_data; // data

    if (!battery_icon)
    {
        battery_icon = lv_img_create(lv_scr_act());
        lv_obj_align(battery_icon, LV_ALIGN_TOP_RIGHT, -6, 2);
    }
    lv_img_set_src(battery_icon, &img_dsc);
}

void battery_status_callback(int level, int charging)
{
    if (debug_queue_handle)
    {
        Message_t msg;
        snprintf(msg.body, 128, ">battery_level:%d%%,battery_charging:%d\r\n", level, charging);
        msg.level = LOG_DEBUG;
        xQueueSendFromISR(debug_queue_handle, (void *)&msg, 0); // needs to be thread safe since it can be called from ISR, use FromISR version of xQueueSend
    }
    else
    {
        Serial.printf(">battery_level:%d%%,battery_charging:%d\r\n", level, charging);
    }

    if ((latest_battery_level >= 88 && level < 88) ||
        (latest_battery_level >= 63 && level < 63) ||
        (latest_battery_level >= 38 && level < 38) ||
        (latest_battery_level >= 13 && level < 13) ||
        (latest_battery_level < 13 && level >= 13) ||
        charging != latest_battery_charging)
    {
        battery_update_needed = true;
    }
    latest_battery_level = level;
    latest_battery_charging = charging;
}

auto input_callback = [](aerid_input_t input, aerid_input_event_t event)
{
    if (debug_queue_handle)
    {
        Message_t msg;
        snprintf(msg.body, 128, "Received input event: input=%d event=%d\r\n", input, event);
        msg.level = LOG_DEBUG;
        xQueueSendFromISR(debug_queue_handle, (void *)&msg, 0); // needs to be thread safe since it can be called from ISR, use FromISR version of xQueueSend
    }
    else
    {
        Serial.printf("Received input event: input=%d event=%d\r\n", input, event);
    }
};

void aerid_launcher_init(QueueHandle_t debug_queue)
{
    last_display_tick = xTaskGetTickCount();
    launcher_input_callback = input_callback;
    debug_queue_handle = debug_queue;
    for (int i = 0; i < AERID_INPUT_COUNT; ++i)
    {
        aerid_input_register_callback((aerid_input_t)i, launcher_input_callback);
    }

    registered_apps = std::vector<aerid_app_metadata_t>();

    aerid_battery_register_status_callback(battery_status_callback);
    battery_update_needed = true; // force update on init

    lv_obj_clean(lv_scr_act());
    lv_timer_handler();

    // TODO: Implement launcher initialization

    aerid_app_metadata_t launcher_app;
    strncpy(launcher_app.app_id, "digital.aerify.aerid_launcher", sizeof(launcher_app.app_id));
    strncpy(launcher_app.name, "AerID Launcher", sizeof(launcher_app.name));
    strncpy(launcher_app.version, aerid_system_get_firmware_version_string(), sizeof(launcher_app.version));
    strncpy(launcher_app.author, "SeqSEE", sizeof(launcher_app.author));
    strncpy(launcher_app.description, "The main launcher interface for AerID apps", sizeof(launcher_app.description));

    registered_apps.push_back(launcher_app);
}

void aerid_launcher_launch()
{
    // TODO: Implement launcher launch functionality
}

int aerid_launcher_launch_app(const char *app_id)
{
    // TODO: Implement app launch functionality
    return -1;
}

void aerid_launcher_return()
{
    // TODO: Implement return to launcher functionality
}

int aerid_launcher_is_active()
{
    // TODO: Implement active status check
    return 0;
}

int aerid_launcher_list_apps(char app_ids[][64], size_t max_apps)
{
    // TODO: Implement app listing functionality
    return -1;
}

int aerid_launcher_get_active_app(char *app_id, size_t max_len)
{
    // TODO: Implement get active app functionality
    return -1;
}

int aerid_launcher_get_metadata(const char *app_id, aerid_app_metadata_t *metadata)
{
    // TODO: Implement metadata retrieval functionality
    return -1;
}

int aerid_launcher_get_metadata_by_id(const char *app_id, aerid_app_metadata_t *metadata)
{
    // TODO: Implement metadata retrieval by ID functionality
    return -1;
}

void aerid_launcher_register_app_launch_callback(aerid_launcher_app_launch_callback_t callback)
{
    // TODO: Implement callback registration
}

void aerid_launcher_tick()
{

    TickType_t now = xTaskGetTickCount();
    if ((now - last_display_tick) >= LV_DEF_REFR_PERIOD * 3)
    {
        if (battery_update_needed)
        {
            update_battery_icon(latest_battery_level, latest_battery_charging);
            battery_update_needed = false;
        }
        lv_timer_handler();
        last_display_tick = now;
    }
}