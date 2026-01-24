#include "main.h"

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
        gpio_set_irq_enabled_with_callback(pin, GPIO_IRQ_EDGE_FALL, true, &btn_gpio_callback);
    }

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(20));
    }
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

    vTaskDelay(pdMS_TO_TICKS(2000));

    float voltage = 0.0f;
    const char *status = read_status();
    voltage = measure_battery_voltage();
    snprintf(msg.body, 128, ">battery_voltage:%.2fV\r\n", voltage);
    msg.level = LOG_DEBUG;
    xQueueSend(usbQueue, (void *)&msg, 0);
    while (1)
    {
        status = read_status();
        voltage = measure_battery_voltage();
        snprintf(msg.body, 128, ">battery_voltage:%.2fV\r\n", voltage);
        msg.level = LOG_DEBUG;
        xQueueSend(usbQueue, (void *)&msg, 0);
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

        // TODO: Add queue and file operations here
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void display_task(void *pvParameters)
{
    Message_t msg;
    snprintf(msg.body, 128, "Display Task Started\r\n");
    msg.level = LOG_DEBUG;
    xQueueSend(usbQueue, (void *)&msg, 0);

    // TODO: Initialize LVGL and e-Paper display here
    //  lv_init();
    //  lv_tick_set_cb(my_tick_cb);
    //  lv_display_t * display = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
    /*Add rendering buffers to the screen.
     *Here adding a smaller partial buffer assuming 1 bit color */
    // static uint8_t buf[(WIDTH * HEIGHT + 7) / 8]; /* 1 bit color */
    // lv_display_set_buffers(display, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);

    /*Add a callback that can flush the content from `buf` when it has been rendered*/
    // lv_display_set_flush_cb(display, my_flush_cb);

    // lv_indev_t * indev = lv_indev_create();
    // lv_indev_set_type(indev, LV_INDEV_TYPE_KEYPAD);
    // lv_indev_set_read_cb(indev, my_keypad_read_cb);

    /* bool my_keypad_read_cb(lv_indev_t * indev, lv_indev_data_t * data) {
    if (button_up_pressed) {
        data->key = LV_KEY_UP;
        data->state = LV_INDEV_STATE_PRESSED;
    } else if (button_down_pressed) {
        data->key = LV_KEY_DOWN;
        data->state = LV_INDEV_STATE_PRESSED;
    } else if (button_left_pressed) {
        data->key = LV_KEY_LEFT;
        data->state = LV_INDEV_STATE_PRESSED;
    } else if (button_right_pressed) {
        data->key = LV_KEY_RIGHT;
        data->state = LV_INDEV_STATE_PRESSED;
    } else if (button_center_pressed) {
        data->key = LV_KEY_ENTER;
        data->state = LV_INDEV_STATE_PRESSED;
    } else if (button_a_pressed) {
        data->key = LV_KEY_NEXT;
        data->state = LV_INDEV_STATE_PRESSED;
    } else if (button_b_pressed) {
        data->key = LV_KEY_PREV;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
    return false;
}*/

    /*The drivers are in place; now we can create the UI*/
    // lv_obj_t * label = lv_label_create(lv_screen_active());
    // lv_label_set_text(label, "Hello world");
    // lv_obj_center(label);

#ifdef EPD_2IN13

#elif defined(EPD_2IN66)

#else
#ifndef I2C_SCAN
#error "No e-Paper display selected"
#endif
#endif

    while (1)
    {
        // lv_timer_handler();
        if (xSemaphoreTake(spi0_mutex, portMAX_DELAY) == pdTRUE)
        {
#ifdef EPD_2IN13

#elif defined(EPD_2IN66)

#else
#ifndef I2C_SCAN
#error "No e-Paper display selected"
#endif
#endif
            xSemaphoreGive(spi0_mutex);
        }
        else
        {
            snprintf(msg.body, 128, "Failed to take SPI0 mutex for e-Paper display\r\n");
            msg.level = LOG_ERROR;
            xQueueSend(usbQueue, (void *)&msg, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(5));
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
    displayQueue = xQueueCreate(MSG_QUEUE_LEN, sizeof(Message_t));      // TODO: add a display command struct
    buzzerQueue = xQueueCreate(MSG_QUEUE_LEN, sizeof(BuzzerCommand_t)); // TODO: update buzzer command to take melodies etc.

    pinMode(SPI0_CS_PIN, OUTPUT);
    pinMode(EPD_RST_PIN, OUTPUT);
    pinMode(EPD_DC_PIN, OUTPUT);
    pinMode(EPD_BUSY_PIN, INPUT);

    SPI0.begin();
    SPI0.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));

    spi0_mutex = xSemaphoreCreateMutex();
    i2c_default_mutex = xSemaphoreCreateMutex();
    adc_mutex = xSemaphoreCreateMutex();

    xTaskCreate(usb_task, "USB Task", 1024, NULL, 1, &usbTaskHandle);
#ifdef I2C_SCAN
    xTaskCreate(i2c_scan_task, "I2C Scan Task", 1024, NULL, 1, &i2cScanTaskHandle);
    I2C0.begin();
    I2C1.begin();
#else
    I2C0.begin(0x21); // Initialize I2C0 as slave with address 0x21
    I2C1.begin();
    xTaskCreate(bms_task, "BMS Task", 1024, NULL, 1, &bmsTaskHandle);
    xTaskCreate(sd_task, "SD Task", 4096, NULL, 1, &sdTaskHandle);
    xTaskCreate(display_task, "Display Task", 4096, NULL, 1, &displayTaskHandle);
    xTaskCreate(buzzer_task, "Buzzer Task", 1024, NULL, 1, &buzzerTaskHandle);
    xTaskCreate(button_task, "Button Task", 512, NULL, 1, &buttonTaskHandle);
    // test atecc508a
    /*
    if (init_atecc508a(I2C1) == false)
    {
        DEBUG_PRINTLN("ATECC508A not detected!");
    }
    else
    {
        DEBUG_PRINTLN("ATECC508A detected.");
        uint8_t serial_number[SERIAL_NUMBER_SIZE];
        if (read_atecc508a_serial_number(serial_number))
        {
            DEBUG_PRINT("ATECC508A Serial Number: ");
            for (int i = 0; i < SERIAL_NUMBER_SIZE; ++i)
            {
                if (serial_number[i] < 0x10)
                    DEBUG_PRINT("0");
                DEBUG_PRINT(serial_number[i], HEX);
                if (i < SERIAL_NUMBER_SIZE - 1)
                    DEBUG_PRINT(":");
            }
            DEBUG_PRINTLN();
        }
        else
        {
            DEBUG_PRINTLN("Failed to read ATECC508A serial number!");
        }
    }
    */
#endif
}

void loop()
{
    // Not used when using FreeRTOS, but required by Arduino
    vTaskDelay(pdMS_TO_TICKS(1));
}