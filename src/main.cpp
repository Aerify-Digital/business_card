#include "main.h"

void button_task(void *pvParameters)
{
    // Configure button GPIOs with pull-ups and interrupts
    const uint button_pins[] = {
        BTN_DPAD_UP_PIN,
        BTN_DPAD_DOWN_PIN,
        BTN_DPAD_LEFT_PIN,
        BTN_DPAD_RIGHT_PIN,
        BTN_DPAD_CENTER_PIN,
        BTN_A_PIN,
        BTN_B_PIN};
    const size_t button_count = sizeof(button_pins) / sizeof(button_pins[0]);

    for (size_t i = 0; i < button_count; ++i)
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
        // take a sample average to reduce noise
        const int samples = 5;
        float total = 0.0f;
        for (int i = 0; i < samples; ++i)
        {
            adc_select_input(0);
            uint16_t adc_raw = adc_read();
            float vbat = 2.0f * ((float)adc_raw / 4095.0f) * 3.3f * 0.969f;
            total += vbat;
            sleep_us(10);
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
    if (xSemaphoreTake(spi0_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
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
    }
    else
    {
        snprintf(msg.body, 128, "SD Card initialized.\r\n");
        msg.level = LOG_INFO;
        xQueueSend(usbQueue, (void *)&msg, 0);
    }

    while (1)
    {
        if (!initialized)
        {
            snprintf(msg.body, 128, "Attempting SD Card re-initialization...\r\n");
            msg.level = LOG_INFO;
            xQueueSend(usbQueue, (void *)&msg, 0);
            if (xSemaphoreTake(spi0_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
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
                msg.level = LOG_INFO;
                xQueueSend(usbQueue, (void *)&msg, 0);
            }
            else
            {
                snprintf(msg.body, 128, "SD Card re-initialization failed!\r\n");
                msg.level = LOG_ERROR;
                xQueueSend(usbQueue, (void *)&msg, 0);
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        if (xSemaphoreTake(spi0_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
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
        }
        else
        {
            snprintf(msg.body, 128, "Failed to obtain SPI mutex for SD Card re-initialization!\r\n");
            msg.level = LOG_ERROR;
            xQueueSend(usbQueue, (void *)&msg, 0);
            continue;
        }

        // TODO: Add queue and file operations here
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void display_task(void *pvParameters)
{
    Message_t msg;
    snprintf(msg.body, 128, "Display Task Started\r\n");
    msg.level = LOG_DEBUG;
    xQueueSend(usbQueue, (void *)&msg, 0);

#ifdef EPD_2IN13
    unsigned char image[1050];
    static Epd2in13 epd(&SPI0);
#elif defined(EPD_2IN66)
    UBYTE image[500];
    static Epd2in66 epd(&SPI0);
#else
#error "No e-Paper display selected"
#endif

    Paint paint(image, EPD_WIDTH * 8, EPD_HEIGHT);
    while (1)
    {
        if (xSemaphoreTake(spi0_mutex, portMAX_DELAY) == pdTRUE)
        {
#ifdef EPD_2IN13

#elif defined(EPD_2IN66)

#else
#error "No e-Paper display selected"
#endif
            xSemaphoreGive(spi0_mutex);
        }
        else
        {
            snprintf(msg.body, 128, "Failed to take SPI0 mutex for e-Paper display\r\n");
            msg.level = LOG_ERROR;
            xQueueSend(usbQueue, (void *)&msg, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(1));
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
    xTaskCreate(usb_task, "USB Task", 1024, NULL, 1, &usbTaskHandle);
    xTaskCreate(bms_task, "BMS Task", 1024, NULL, 1, &bmsTaskHandle);
    xTaskCreate(sd_task, "SD Task", 4096, NULL, 1, &sdTaskHandle);
    xTaskCreate(display_task, "Display Task", 4096, NULL, 1, &displayTaskHandle);
    xTaskCreate(buzzer_task, "Buzzer Task", 1024, NULL, 1, &buzzerTaskHandle);
    xTaskCreate(button_task, "Button Task", 512, NULL, 1, &buttonTaskHandle);
}

void loop()
{
    // Not used when using FreeRTOS, but required by Arduino
    vTaskDelay(pdMS_TO_TICKS(1));
}