#include "main.h"

static EpdBase *epd = nullptr;

extern "C" void vApplicationTickHook(void)
{
    lv_tick_inc(1);
}

void splash_screen()
{
    Message_t msg;
    snprintf(msg.body, 128, "Displaying splash screen\r\n");
    msg.level = LOG_DEBUG;
    xQueueSend(usbQueue, (void *)&msg, 0);

    int screen_w = lv_obj_get_width(lv_scr_act());
    int screen_h = lv_obj_get_height(lv_scr_act());

    snprintf(msg.body, 128, "Screen dimensions: %dx%d\r\n", screen_w, screen_h);
    msg.level = LOG_DEBUG;
    xQueueSend(usbQueue, (void *)&msg, 0);
    static const unsigned char *splash_data = nullptr;

#ifdef EPD_2IN13
    splash_data = IMAGE_DATA_2IN13;
#elif defined(EPD_2IN66)
    splash_data = IMAGE_DATA_2IN66;
#else
#ifndef I2C_SCAN
#error "No e-Paper display selected"
#endif
#endif

    const uint16_t splash_stride = (screen_w + 7) / 8;
    const size_t splash_img_size = splash_stride * screen_h;

    uint8_t splash_data_prefixed[8 + splash_img_size];

    memcpy(splash_data_prefixed, LVGL_PALETTE, 8);

    for (uint16_t row = 0; row < screen_h; ++row)
    {
        for (uint16_t col_byte = 0; col_byte < splash_stride; ++col_byte)
        {
            size_t src_idx = row * splash_stride + col_byte;
            size_t dst_idx = 8 + row * splash_stride + col_byte;
            splash_data_prefixed[dst_idx] = pgm_read_byte(&splash_data[src_idx]);
        }
    }

    lv_obj_t *container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(container, screen_w, screen_h);
    lv_obj_set_layout(container, LV_LAYOUT_NONE);

    static lv_img_dsc_t splash_img_dsc = {
        {LV_IMAGE_HEADER_MAGIC, // header.magic
         LV_COLOR_FORMAT_I1,    // header.cf
         0,                     // header.flags
         (uint16_t)screen_w,    // header.width
         (uint16_t)screen_h,    // header.height
         splash_stride,         // header.stride
         0},                    // header.reserved
        splash_img_size + 8,    // data length                                                                               // data_size
        splash_data_prefixed,   // data
    };

    lv_obj_t *splash_img = lv_img_create(container);
    lv_img_set_src(splash_img, &splash_img_dsc);

    lv_obj_set_style_transform_pivot_x(splash_img, screen_w / 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_pivot_y(splash_img, screen_h / 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(splash_img, LV_ALIGN_CENTER, 0, 0);
    lv_timer_handler();
}

void launcher_task(void *pvParameters)
{
    Message_t msg;
    snprintf(msg.body, 128, "Launcher Task Started\r\n");
    msg.level = LOG_DEBUG;
    xQueueSend(usbQueue, (void *)&msg, 0);

    constexpr int NUM_COMPONENTS = 8; // display, buttons, BMS, SD card, ATECC508A, ATSHA204A, buzzer, NFC
    constexpr int ALL_READY_MASK = (1 << NUM_COMPONENTS) - 1;

    int ready_flags = 0;
    int ready_id = -1;
    while (ready_flags != ALL_READY_MASK)
    {
        xQueueReceive(launcherQueue, &ready_id, portMAX_DELAY);
        ready_flags |= (1 << ready_id);
        snprintf(msg.body, 128, "Received ready signal from component %d, ready_flags=0x%02X\r\n", ready_id, ready_flags);
        msg.level = LOG_DEBUG;
        xQueueSend(usbQueue, (void *)&msg, 0);
        if (ready_id == 1) // display ready start loading animation
        {
            snprintf(msg.body, 128, "Display is ready, starting loading animation\r\n");
            msg.level = LOG_DEBUG;
            xQueueSend(usbQueue, (void *)&msg, 0);
            // send message to display task to clear screen and show loading animation
            lv_init();
            lv_tick_set_cb(lvgl_tick_cb);

            lvgl_epaper_set_driver(epd);
            lvgl_epaper_register_display(usbQueue);
            snprintf(msg.body, 128, "LVGL Initialized\r\n");
            msg.level = LOG_DEBUG;
            xQueueSend(usbQueue, (void *)&msg, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    snprintf(msg.body, 128, "All components ready, initializing launcher\r\n");
    msg.level = LOG_DEBUG;
    xQueueSend(usbQueue, (void *)&msg, 0);

    splash_screen();
    vTaskDelay(pdMS_TO_TICKS(3300));

    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *loading = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(loading, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(loading, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(loading, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(loading, "Loading");
    lv_obj_align(loading, LV_ALIGN_CENTER, 0, 0);
    lv_timer_handler();

    aerid_launcher_init(usbQueue);
    // TODO: Initialize the launcher

    while (1)
    {
        aerid_launcher_tick();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void nfc_task(void *pvParameters)
{
    Message_t msg;
    snprintf(msg.body, 128, "NFC Task Started\r\n");
    msg.level = LOG_DEBUG;
    xQueueSend(usbQueue, (void *)&msg, 0);

    int ready_id = 6;
    xQueueSend(launcherQueue, (void *)&ready_id, portMAX_DELAY); // signal launcher task that NFC is ready
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void atecc_task(void *pvParameters)
{
    Message_t msg;
    snprintf(msg.body, 128, "ATECC508A Task Started\r\n");
    msg.level = LOG_DEBUG;
    xQueueSend(usbQueue, (void *)&msg, 0);
    if (init_atecc508a(I2C1) == false)
    {
        snprintf(msg.body, 128, "ATECC508A not detected!\r\n");
        msg.level = LOG_ERROR;
        xQueueSend(usbQueue, (void *)&msg, 0);
    }
    else
    {
        uint8_t serial_number[SERIAL_NUMBER_SIZE];
        if (read_atecc508a_serial_number(serial_number))
        {
            // convert serial number to hex string
            char serial_str[SERIAL_NUMBER_SIZE * 2 + 1];
            for (size_t i = 0; i < SERIAL_NUMBER_SIZE; ++i)
            {
                snprintf(&serial_str[i * 2], 3, "%02X", serial_number[i]);
            }
            snprintf(msg.body, 128, "ATECC508A Serial Number: %s\r\n", serial_str);
            msg.level = LOG_DEBUG;
            xQueueSend(usbQueue, (void *)&msg, 0);
        }
        else
        {
            snprintf(msg.body, 128, "Failed to read ATECC508A serial number!\r\n");
            msg.level = LOG_ERROR;
            xQueueSend(usbQueue, (void *)&msg, 0);
        }
    }

    int ready_id = 4;
    xQueueSend(launcherQueue, (void *)&ready_id, portMAX_DELAY); // signal launcher task that ATECC508A is ready
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void atsha_task(void *pvParameters)
{
    Message_t msg;
    snprintf(msg.body, 128, "ATSHA204A Task Started\r\n");
    msg.level = LOG_DEBUG;
    xQueueSend(usbQueue, (void *)&msg, 0);

    // TODO: implement ATSHA204A initialization and functionality

    int ready_id = 5;
    xQueueSend(launcherQueue, (void *)&ready_id, portMAX_DELAY); // signal launcher task that ATSHA204A is ready
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void button_task(void *pvParameters)
{
    Message_t msg;
    snprintf(msg.body, 128, "Button Task Started\r\n");
    msg.level = LOG_DEBUG;
    xQueueSend(usbQueue, (void *)&msg, 0);

    for (size_t i = 0; i < BUTTON_COUNT; ++i)
    {
        uint pin = button_pins[i];
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
        gpio_set_irq_enabled_with_callback(pin, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &btn_gpio_callback);
    }

    int ready_id = 2;
    xQueueSend(launcherQueue, (void *)&ready_id, portMAX_DELAY); // signal launcher task that buttons are ready
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

float voltageToPercentage(float voltage)
{
    static const float voltage_points[] = {4.20f, 4.10f, 3.95f, 3.85f, 3.70f, 3.50f, 3.30f, 3.00f};
    static const float percent_points[] = {100.0f, 90.0f, 80.0f, 70.0f, 50.0f, 20.0f, 5.0f, 0.0f};
    const int n = sizeof(voltage_points) / sizeof(voltage_points[0]);

    if (voltage >= voltage_points[0])
        return 100.0f;
    if (voltage <= voltage_points[n - 1])
        return 0.0f;

    for (int i = 0; i < n - 1; ++i)
    {
        if (voltage <= voltage_points[i] && voltage > voltage_points[i + 1])
        {
            float v1 = voltage_points[i], v2 = voltage_points[i + 1];
            float p1 = percent_points[i], p2 = percent_points[i + 1];
            return p1 + (voltage - v1) * (p2 - p1) / (v2 - v1);
        }
    }
    return 0.0f;
}

void bms_task(void *pvParameters)
{
    Message_t msg;
    snprintf(msg.body, 128, "BMS Task Started\r\n");
    msg.level = LOG_DEBUG;
    xQueueSend(usbQueue, (void *)&msg, 0);

    gpio_init(BAT_CHARGE_EN_PIN);
    gpio_set_dir(BAT_CHARGE_EN_PIN, GPIO_OUT);
    gpio_put(BAT_CHARGE_EN_PIN, 1);

    gpio_init(BAT_STAT1_PIN);
    gpio_set_dir(BAT_STAT1_PIN, GPIO_IN);
    gpio_disable_pulls(BAT_STAT1_PIN);
    gpio_init(BAT_STAT2_PIN);
    gpio_set_dir(BAT_STAT2_PIN, GPIO_IN);
    gpio_disable_pulls(BAT_STAT2_PIN);

    gpio_init(BAT_VOLTAGE_PIN);
    adc_init();
    adc_gpio_init(BAT_VOLTAGE_PIN);

    gpio_set_irq_enabled_with_callback(BAT_STAT1_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &stat_gpio_callback);
    gpio_set_irq_enabled_with_callback(BAT_STAT2_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &stat_gpio_callback);

    auto measure_battery_voltage = []()
    {
        Message_t msg;
        // take a sample average to reduce noise
        const int samples = 5;
        float total = 0.0f;
        if (xSemaphoreTake(adc_mutex, portMAX_DELAY) == pdTRUE)
        {
            for (int i = 0; i < samples; ++i)
            {
                adc_select_input(0);
                uint16_t adc_raw = adc_read();
                float vbat = 2.0f * ((float)adc_raw / 4095.0f) * 3.3f * 0.969f;
                total += vbat;
                sleep_us(10);
            }
            xSemaphoreGive(adc_mutex);
        }
        else
        {
            snprintf(msg.body, 128, "Failed to obtain ADC mutex for battery voltage measurement!\r\n");
            msg.level = LOG_ERROR;
            xQueueSend(usbQueue, (void *)&msg, 0);
        }
        float voltage = (float)total / (float)samples;
        return voltage;
    };

    auto read_status = []()
    {
        bool STAT1 = gpio_get(BAT_STAT1_PIN);
        bool STAT2 = gpio_get(BAT_STAT2_PIN);
        bool STAT1_flashing = stat1_transitions > 1;
        bool STAT2_flashing = stat2_transitions > 1;
        stat1_transitions = 0;
        stat2_transitions = 0;
        const char *result;
        if (STAT1_flashing && STAT2_flashing)
        {
            result = "EMPTY";
        }
        else if (STAT1 && !STAT2)
        {
            result = "CHARGING_COMPLETE";
        }
        else if (!STAT1 && STAT2)
        {
            result = "CHARGING";
        }
        else if (STAT1 && STAT2)
        {
            result = "FAULT_CONDITION";
        }
        else
        {
            result = "DISCHARGING";
        }
        if (result == nullptr)
            result = "UNKNOWN";

        return result;
    };

    int ready_id = 0;
    xQueueSend(launcherQueue, (void *)&ready_id, portMAX_DELAY); // signal launcher task that BMS is ready

    vTaskDelay(pdMS_TO_TICKS(2000));

    float voltage = 0.0f;
    const char *status = read_status();
    voltage = measure_battery_voltage();
    snprintf(msg.body, 128, ">battery_voltage:%.2fV\r\n", voltage);
    msg.level = LOG_DEBUG;
    xQueueSend(usbQueue, (void *)&msg, 0);

    int level = 0;
    int charging = 0;
    int voltage_mv = (int)(voltage * 1000);

    if (strcmp(status, "EMPTY") == 0)
    {
        level = 0;
        charging = 0;
    }
    else if (strcmp(status, "CHARGING_COMPLETE") == 0)
    {
        level = 100;
        charging = 0;
    }
    else if (strcmp(status, "CHARGING") == 0)
    {
        level = (int)voltageToPercentage(voltage);
        charging = 1;
    }
    else if (strcmp(status, "FAULT_CONDITION") == 0)
    {
        level = 0;
        charging = 0;
    }
    else if (strcmp(status, "DISCHARGING") == 0)
    {
        level = (int)voltageToPercentage(voltage);
        charging = 0;
    }
    aerid_battery_update_status(level, charging, voltage_mv);

    while (1)
    {
        status = read_status();
        voltage = measure_battery_voltage();
        snprintf(msg.body, 128, ">battery_voltage:%.2fV\r\n", voltage);
        msg.level = LOG_DEBUG;
        xQueueSend(usbQueue, (void *)&msg, 0);
        voltage_mv = (int)(voltage * 1000);
        if (voltage <= 3.0f)
        {
            level = 0;
        }
        else if (voltage >= 4.2f)
        {
            level = 100;
        }
        else
        {
            level = (int)(((voltage - 3.0f) / (4.2f - 3.0f)) * 100.0f);
        }
        if (strcmp(status, "EMPTY") == 0)
        {
            level = 0;
            charging = 0;
        }
        else if (strcmp(status, "CHARGING_COMPLETE") == 0)
        {
            level = 100;
            charging = 0;
        }
        else if (strcmp(status, "CHARGING") == 0)
        {
            charging = 1;
        }
        else if (strcmp(status, "FAULT_CONDITION") == 0)
        {
            level = 0;
            charging = 0;
        }
        else if (strcmp(status, "DISCHARGING") == 0)
        {
            charging = 0;
        }
        aerid_battery_update_status(level, charging, voltage_mv);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void usb_task(void *pvParameters)
{
    DEBUG_PRINTLN("USB Task Started");
    Message_t rcv_msg;
    while (1)
    {
        xQueueReceive(usbQueue, (void *)&rcv_msg, portMAX_DELAY);

        // Write log to SD card using aerid_fs_append_file if level is not LOG_NONE, and also print to Serial based on log levelS
        if (rcv_msg.level != LOG_NONE)
        {

            const char *level_str = "UNKNOWN";
            switch (rcv_msg.level)
            {
            case LOG_DEBUG:
                level_str = "DEBUG";
                break;
            case LOG_INFO:
                level_str = "INFO";
                break;
            case LOG_WARN:
                level_str = "WARN";
                break;
            case LOG_ERROR:
                level_str = "ERROR";
                break;
            default:
                break;
            }
            /*

            TODO: Re-enable SD card logging once we have a more robust solution for handling SD card unmounting and errors or whatever about it is crashing the usb_task
            if (aerid_fs_is_mounted())
            {
                int result = aerid_fs_append_file("log.txt", (const uint8_t *)rcv_msg.body, strnlen(rcv_msg.body, 128));
                if (result < 0)
                {
                    DEBUG_PRINTF("Failed to write log entry to SD card (error code %d)\r\n", result);
                }
            }*/
        }

        switch (rcv_msg.level)
        {
        case LOG_NONE:
            break;
        case LOG_DEBUG:
            DEBUG_PRINTF("%s", rcv_msg.body);
            break;
        case LOG_INFO:
            Serial.printf("%s", rcv_msg.body);
            break;
        case LOG_WARN:
            Serial.printf("%s", rcv_msg.body);
            break;
        case LOG_ERROR:
            Serial.printf("%s", rcv_msg.body);
            break;
        default:
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void sd_task(void *pvParameters)
{
    Message_t msg;
    snprintf(msg.body, 128, "SD Task Started\r\n");
    msg.level = LOG_DEBUG;
    xQueueSend(usbQueue, (void *)&msg, 0);
    sd_request_t req;
    bool initialized = false;
    if (xSemaphoreTake(spi0_mutex, portMAX_DELAY) == pdTRUE)
    {

        initialized = beginSD(SPI0);
        xSemaphoreGive(spi0_mutex);
    }
    else
    {
        snprintf(msg.body, 128, "Failed to obtain SPI mutex for SD Card initialization!\r\n");
        msg.level = LOG_ERROR;
        xQueueSend(usbQueue, (void *)&msg, 0);
    }
    if (!initialized)
    {
        snprintf(msg.body, 128, "SD Card initialization failed!\r\n");
        msg.level = LOG_ERROR;
        xQueueSend(usbQueue, (void *)&msg, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    else
    {
        snprintf(msg.body, 128, "SD Card initialized.\r\n");
        msg.level = LOG_DEBUG;
        xQueueSend(usbQueue, (void *)&msg, 0);
    }

    int ready_id = 3;
    xQueueSend(launcherQueue, (void *)&ready_id, portMAX_DELAY); // signal launcher task that SD card is ready

    while (1)
    {
        if (!initialized)
        {
            snprintf(msg.body, 128, "Attempting SD Card re-initialization...\r\n");
            msg.level = LOG_INFO;
            xQueueSend(usbQueue, (void *)&msg, 0);
            if (xSemaphoreTake(spi0_mutex, portMAX_DELAY) == pdTRUE)
            {
                initialized = beginSD(SPI0);
                xSemaphoreGive(spi0_mutex);
            }
            else
            {
                snprintf(msg.body, 128, "Failed to obtain SPI mutex for SD Card re-initialization!\r\n");
                msg.level = LOG_ERROR;
                xQueueSend(usbQueue, (void *)&msg, 0);
                continue;
            }
            if (initialized)
            {
                snprintf(msg.body, 128, "SD Card re-initialized successfully.\r\n");
                msg.level = LOG_DEBUG;
                xQueueSend(usbQueue, (void *)&msg, 0);
            }
            else
            {
                snprintf(msg.body, 128, "SD Card re-initialization failed!\r\n");
                msg.level = LOG_ERROR;
                xQueueSend(usbQueue, (void *)&msg, 0);
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }
        }

        if (xSemaphoreTake(spi0_mutex, portMAX_DELAY) == pdTRUE)
        {

            bool present = cardPresent(SPI0);
            if (!present)
            {
                initialized = false;
                snprintf(msg.body, 128, "SD Card not present!\r\n");
                msg.level = LOG_WARN;
                xQueueSend(usbQueue, (void *)&msg, 0);
            }
            xSemaphoreGive(spi0_mutex);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        else
        {
            snprintf(msg.body, 128, "Failed to obtain SPI mutex for SD Card re-initialization!\r\n");
            msg.level = LOG_ERROR;
            xQueueSend(usbQueue, (void *)&msg, 0);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        if (xQueueReceive(sdQueue, &req, portMAX_DELAY) == pdTRUE)
        {
            if (req.done)
            {
                xSemaphoreTake(req.done, portMAX_DELAY);
            }
            xSemaphoreTake(spi0_mutex, portMAX_DELAY);
            switch (req.op)
            {
            case SD_OP_READ:
                req.length = readSDFile(req.filename, req.buffer, req.length);
                req.result = req.length >= 0;
                break;
            case SD_OP_WRITE:
                req.length = writeSDFile(req.filename, req.buffer, req.length);
                req.result = req.length >= 0;
                break;
            case SD_OP_APPEND:
                req.length = appendSDFile(req.filename, req.buffer, req.length);
                req.result = req.length >= 0;
                break;
            case SD_OP_DELETE:
                req.result = deleteSDFile(req.filename);
                break;
            case SD_OP_RENAME:
                req.result = renameSDFile(req.filename, (const char *)req.buffer);
                break;
            default:
                req.result = false;
                break;
            }
            xSemaphoreGive(spi0_mutex);
            if (req.done)
            {
                xSemaphoreGive(req.done);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void display_task(void *pvParameters)
{
    Message_t msg;
    snprintf(msg.body, 128, "Display Task Started\r\n");
    msg.level = LOG_DEBUG;
    xQueueSend(usbQueue, (void *)&msg, 0);

#ifdef EPD_2IN13
    epd = new Epd2in13(&SPI0, spi0_mutex);
    epd->Init(FULL);
#elif defined(EPD_2IN66)
    epd = new Epd2in66(&SPI0, spi0_mutex);
    epd->Init(FULL);
#else
#ifndef I2C_SCAN
#error "No e-Paper display selected"
#endif
#endif

    int ready_id = 1;
    xQueueSend(launcherQueue, &ready_id, portMAX_DELAY); // signal launcher task that display is ready

    while (1)
    {

        // TODO: create a queue to receive display update requests

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void buzzer_task(void *pvParameters)
{
    Message_t msg;
    snprintf(msg.body, 128, "Buzzer Task Started\r\n");
    msg.level = LOG_DEBUG;
    xQueueSend(usbQueue, (void *)&msg, 0);
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    gpio_put(BUZZER_PIN, 0);

    const int doom_melody[] = {
        NOTE_E2, NOTE_E2, NOTE_E4, NOTE_E2, NOTE_E2, NOTE_D4, NOTE_E2, NOTE_E2, NOTE_C4, NOTE_E2, NOTE_E2, NOTE_AS4, NOTE_E2, NOTE_E2, NOTE_B4, NOTE_C4,
        NOTE_E2, NOTE_E2, NOTE_E4, NOTE_E2, NOTE_E2, NOTE_D4, NOTE_E2, NOTE_E2, NOTE_C4, NOTE_E2, NOTE_E2, NOTE_AS4, NOTE_AS4, NOTE_AS4, NOTE_AS4, NOTE_REST,
        NOTE_E2, NOTE_E2, NOTE_E4, NOTE_E2, NOTE_E2, NOTE_D4, NOTE_E2, NOTE_E2, NOTE_C4, NOTE_E2, NOTE_E2, NOTE_AS4, NOTE_E2, NOTE_E2, NOTE_B4, NOTE_C4,
        NOTE_E2, NOTE_E2, NOTE_E4, NOTE_E2, NOTE_E2, NOTE_D4, NOTE_E2, NOTE_E2, NOTE_C4, NOTE_E2, NOTE_E2, NOTE_AS4, NOTE_AS4, NOTE_AS4, NOTE_AS4, NOTE_REST,
        NOTE_A3, NOTE_A3, NOTE_A5, NOTE_A3, NOTE_A3, NOTE_G4, NOTE_A3, NOTE_A3, NOTE_F4, NOTE_A3, NOTE_A3, NOTE_DS4, NOTE_A3, NOTE_A3, NOTE_E4, NOTE_F4,
        NOTE_A3, NOTE_A3, NOTE_A5, NOTE_A3, NOTE_A3, NOTE_G4, NOTE_A3, NOTE_A3, NOTE_F4, NOTE_A3, NOTE_A3, NOTE_DS4, NOTE_DS4, NOTE_DS4, NOTE_DS4, NOTE_REST};

    const int note_count = sizeof(doom_melody) / sizeof(doom_melody[0]);
    const int note_duration_ms = 150; // 200 BPM 1/8 notes

    int ready_id = 7;
    xQueueSend(launcherQueue, (void *)&ready_id, portMAX_DELAY); // signal launcher task that buzzer is ready

    while (1)
    {

        /*
        for (int i = 0; i < note_count; ++i)
        {
            play_tone(BUZZER_PIN, midi_to_freq(doom_melody[i]), note_duration_ms);
        }
        */

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void i2c_scan_task(void *pvParameters)
{

    Message_t msg;
    bool led = false;
    bool scan = false;

    int count = 0;
    msg.level = LOG_DEBUG;
    snprintf(msg.body, 128, "I2C Scan Started\n");
    xQueueSend(usbQueue, (void *)&msg, 10);
    while (1)
    {
        if (count == 10 && !scan)
        {
            msg.level = LOG_INFO;
            snprintf(msg.body, 128, "\nI2C Bus 0 Scan\n");
            xQueueSend(usbQueue, (void *)&msg, 10);
            snprintf(msg.body, 128, "   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
            xQueueSend(usbQueue, (void *)&msg, 10);
            for (int addr = 0; addr < (1 << 7); ++addr)
            {
                if (addr % 16 == 0)
                {
                    snprintf(msg.body, 128, "%02x ", addr);
                    xQueueSend(usbQueue, (void *)&msg, 10);
                }

                // Perform a 1-byte dummy read from the probe address. If a slave
                // acknowledges this address, the function returns the number of bytes
                // transferred. If the address byte is ignored, the function returns
                // -1.

                // Skip over any reserved addresses.
                int ret;
                uint8_t rxdata;
                if (reserved_addr(addr))
                    ret = PICO_ERROR_GENERIC;
                else
                    ret = i2c_read_blocking(i2c_default, addr, &rxdata, 1, false);

                snprintf(msg.body, 128, ret < 0 ? "." : "@");
                xQueueSend(usbQueue, (void *)&msg, 10);
                snprintf(msg.body, 128, addr % 16 == 15 ? "\n" : "  ");
                xQueueSend(usbQueue, (void *)&msg, 10);
            }
            snprintf(msg.body, 128, "Done.\n");
            xQueueSend(usbQueue, (void *)&msg, 10);
            snprintf(msg.body, 128, "\nI2C Bus 1 Scan\n");
            xQueueSend(usbQueue, (void *)&msg, 10);
            snprintf(msg.body, 128, "   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
            xQueueSend(usbQueue, (void *)&msg, 10);
            for (int addr = 0; addr < (1 << 7); ++addr)
            {
                if (addr % 16 == 0)
                {
                    snprintf(msg.body, 128, "%02x ", addr);
                    xQueueSend(usbQueue, (void *)&msg, 10);
                }

                // Perform a 1-byte dummy read from the probe address. If a slave
                // acknowledges this address, the function returns the number of bytes
                // transferred. If the address byte is ignored, the function returns
                // -1.

                // Skip over any reserved addresses.
                int ret;
                uint8_t rxdata;
                if (reserved_addr(addr))
                    ret = PICO_ERROR_GENERIC;
                else
                    ret = i2c_read_blocking(i2c1, addr, &rxdata, 1, false);

                snprintf(msg.body, 128, ret < 0 ? "." : "@");
                xQueueSend(usbQueue, (void *)&msg, 10);
                snprintf(msg.body, 128, addr % 16 == 15 ? "\n" : "  ");
                xQueueSend(usbQueue, (void *)&msg, 10);
            }
            snprintf(msg.body, 128, "Done.\n");
            xQueueSend(usbQueue, (void *)&msg, 10);
            scan = true;
            vTaskDelete(i2cScanTaskHandle);
        }

        if (count < 10)
            count++;

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void setup()
{
    Serial.begin(115200);
    sleep_ms(3000);
    Serial.println("Aerify Digital Business Card Starting...");

    usbQueue = xQueueCreate(MSG_QUEUE_LEN, sizeof(Message_t));
    launcherQueue = xQueueCreate(MSG_QUEUE_LEN, sizeof(int));           // TODO: create a proper ready message struct if more data is needed
    displayQueue = xQueueCreate(MSG_QUEUE_LEN, sizeof(Message_t));      // TODO: add a display command struct
    buzzerQueue = xQueueCreate(MSG_QUEUE_LEN, sizeof(BuzzerCommand_t)); // TODO: update buzzer command to take melodies etc.
    sdQueue = xQueueCreate(MSG_QUEUE_LEN, sizeof(sd_request_t));

    pinMode(SPI0_CS_PIN, OUTPUT);
    pinMode(EPD_RST_PIN, OUTPUT);
    pinMode(EPD_DC_PIN, OUTPUT);
    pinMode(EPD_BUSY_PIN, INPUT);

    SPI0.begin();
    SPI0.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));

    spi0_mutex = xSemaphoreCreateMutex();
    i2c_default_mutex = xSemaphoreCreateMutex();
    adc_mutex = xSemaphoreCreateMutex();

    xTaskCreate(usb_task, "USB Task", 4096, NULL, 1, &usbTaskHandle);
#ifdef I2C_SCAN
    xTaskCreate(i2c_scan_task, "I2C Scan Task", 1024, NULL, 1, &i2cScanTaskHandle);
    I2C0.begin();
    I2C1.begin();
#else
    I2C0.begin(0x21); // Initialize I2C0 as slave with address 0x21
    I2C1.begin();
    xTaskCreate(launcher_task, "Launcher Task", 4096, NULL, 1, &launcherTaskHandle);
    xTaskCreate(bms_task, "BMS Task", 1024, NULL, 1, &bmsTaskHandle);
    xTaskCreate(sd_task, "SD Task", 4096, NULL, 1, &sdTaskHandle);
    xTaskCreate(display_task, "Display Task", 8192, NULL, 1, &displayTaskHandle);
    xTaskCreate(buzzer_task, "Buzzer Task", 1024, NULL, 1, &buzzerTaskHandle);
    xTaskCreate(button_task, "Button Task", 512, NULL, 1, &buttonTaskHandle);
    xTaskCreate(atecc_task, "ATECC Task", 1024, NULL, 1, &ateccTaskHandle);
    xTaskCreate(atsha_task, "ATSHA Task", 1024, NULL, 1, &atshaTaskHandle);
    xTaskCreate(nfc_task, "NFC Task", 1024, NULL, 1, &nfcTaskHandle);
#endif
}

void loop()
{
    // Not used when using FreeRTOS, but required by Arduino
    vTaskDelay(pdMS_TO_TICKS(1));
}