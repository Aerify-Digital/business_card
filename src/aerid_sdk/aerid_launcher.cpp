#include "aerid_launcher.h"
#include <vector>
#include <lvgl.h>
#include <queue.h>

static QueueHandle_t debug_queue_handle = nullptr;

static aerid_input_callback_t launcher_input_callback = nullptr;

static std::vector<aerid_app_metadata_t> registered_apps;

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
    launcher_input_callback = input_callback;
    debug_queue_handle = debug_queue;
    for (int i = 0; i < AERID_INPUT_COUNT; ++i)
    {
        aerid_input_register_callback((aerid_input_t)i, launcher_input_callback);
    }

    registered_apps = std::vector<aerid_app_metadata_t>();
    lv_obj_clean(lv_scr_act());
    lv_obj_t *dummy_battery_area = lv_obj_create(lv_scr_act());
    lv_obj_set_size(dummy_battery_area, 88, 22);
    lv_obj_align(dummy_battery_area, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_bg_color(dummy_battery_area, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(dummy_battery_area, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    vTaskDelay(pdMS_TO_TICKS(1320)); // 330 * 4 ms delay to give the display time
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