#include "main.h"

void gpio_callback(uint gpio, uint32_t events)
{
    Message_t msg;
    uint32_t now = to_ms_since_boot(get_absolute_time());
    int idx = -1;
    for (int i = 0; i < BUTTON_COUNT; ++i)
    {
        if (gpio == button_pins[i])
        {
            idx = i;
            break;
        }
    }
    if (idx == -1)
        return;

    if (now - last_press_time[idx] < BTN_DEBOUNCE_MS)
    {
        return;
    }
    last_press_time[idx] = now;

    switch (gpio)
    {
    case BTN_DPAD_UP_PIN:
        snprintf(msg.body, 128, "Button UP Pressed\r\n");
        msg.level = LOG_DEBUG;
        xQueueSend(usbQueue, (void *)&msg, 0);
        break;
    case BTN_DPAD_DOWN_PIN:
        snprintf(msg.body, 128, "Button DOWN Pressed\r\n");
        msg.level = LOG_DEBUG;
        xQueueSend(usbQueue, (void *)&msg, 0);
        break;
    case BTN_DPAD_LEFT_PIN:
        snprintf(msg.body, 128, "Button LEFT Pressed\r\n");
        msg.level = LOG_DEBUG;
        xQueueSend(usbQueue, (void *)&msg, 0);
        break;
    case BTN_DPAD_RIGHT_PIN:
        snprintf(msg.body, 128, "Button RIGHT Pressed\r\n");
        msg.level = LOG_DEBUG;
        xQueueSend(usbQueue, (void *)&msg, 0);
        break;
    case BTN_DPAD_CENTER_PIN:
        snprintf(msg.body, 128, "Button CENTER Pressed\r\n");
        msg.level = LOG_DEBUG;
        xQueueSend(usbQueue, (void *)&msg, 0);
        break;
    case BTN_A_PIN:
        snprintf(msg.body, 128, "Button A Pressed\r\n");
        msg.level = LOG_DEBUG;
        xQueueSend(usbQueue, (void *)&msg, 0);
        break;
    case BTN_B_PIN:
        snprintf(msg.body, 128, "Button B Pressed\r\n");
        msg.level = LOG_DEBUG;
        xQueueSend(usbQueue, (void *)&msg, 0);
        break;
    default:
        sniprintf(msg.body, 128, "Unknown Button %d Pressed\r\n", gpio);
        msg.level = LOG_WARN;
        xQueueSend(usbQueue, (void *)&msg, 0);
        break;
    }
}

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
        gpio_set_irq_enabled_with_callback(pin, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    }

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(20));
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
    Paint paint(image, 48, 80); // width should be the multiple of 8
    UDOUBLE time_start_ms;
    UDOUBLE time_now_s;
    static Epd2in66 epd(&SPI0);

#else
#error "No e-Paper display selected"
#endif
    while (1)
    {

#ifdef EPD_2IN13
        static Epd2in13 epd(&SPI0);

        if (epd.Init(FAST) != 0)
        {
            snprintf(msg.body, 128, "e-Paper init failed...\r\n");
            msg.level = LOG_ERROR;
            xQueueSend(usbQueue, (void *)&msg, 0);
            return;
        }
        snprintf(msg.body, 128, "Starting 2.13inch e-Paper demo...\r\n ");
        msg.level = LOG_INFO;
        xQueueSend(usbQueue, (void *)&msg, 0);
        epd.Display_Fast(IMAGE_DATA_2IN13);

#if 1
        snprintf(msg.body, 128, "epd FULL\r\n");
        msg.level = LOG_INFO;
        xQueueSend(usbQueue, (void *)&msg, 0);
        if (epd.Init(FULL) != 0)
        {
            snprintf(msg.body, 128, "e-Paper init failed...\r\n");
            msg.level = LOG_ERROR;
            xQueueSend(usbQueue, (void *)&msg, 0);
            return;
        }
        Paint paint(image, epd.bufwidth * 8, epd.bufheight); // width should be the multiple of 8

        paint.Clear(UNCOLORED);
        paint.DrawStringAt(8, 2, "e-Paper Demo", &Font12, COLORED);
        paint.DrawStringAt(8, 20, "Hello world", &Font12, COLORED);
        epd.Display1(image); // 1

        paint.Clear(UNCOLORED);
        paint.DrawRectangle(2, 2, 50, 50, COLORED);
        paint.DrawLine(2, 2, 50, 50, COLORED);
        paint.DrawLine(2, 50, 50, 2, COLORED);
        paint.DrawFilledRectangle(52, 2, 100, 50, COLORED);
        paint.DrawLine(52, 2, 100, 50, UNCOLORED);
        paint.DrawLine(100, 2, 52, 50, UNCOLORED);
        epd.Display1(image); // 2

        paint.Clear(UNCOLORED);
        paint.DrawCircle(25, 25, 20, COLORED);
        paint.DrawFilledCircle(75, 25, 20, COLORED);
        epd.Display1(image); // 3

        paint.Clear(UNCOLORED);
        epd.Display1(image); // 4

        vTaskDelay(pdMS_TO_TICKS(2000));

#else

        snprintf(msg.body, 128, "epd PART\r\n");
        msg.level = LOG_INFO;
        xQueueSend(usbQueue, (void *)&msg, 0);
        epd.DisplayPartBaseImage(IMAGE_DATA_2IN13);
        char i = 0;
        for (i = 0; i < 10; i++)
        {
            snprintf(msg.body, 128, "e-Paper PART IMAGE_DATA\r\n");
            msg.level = LOG_INFO;
            xQueueSend(usbQueue, (void *)&msg, 0);
            if (epd.Init(PART) != 0)
            {
                snprintf(msg.body, 128, "e-Paper init failed...\r\n");
                msg.level = LOG_ERROR;
                xQueueSend(usbQueue, (void *)&msg, 0);
                return;
            }
            epd.DisplayPart(IMAGE_DATA_2IN13);
            snprintf(msg.body, 128, "e-Paper PART Clear\r\n");
            msg.level = LOG_INFO;
            xQueueSend(usbQueue, (void *)&msg, 0);
            if (epd.Init(PART) != 0)
            {
                snprintf(msg.body, 128, "e-Paper init failed...\r\n");
                msg.level = LOG_ERROR;
                xQueueSend(usbQueue, (void *)&msg, 0);
                return;
            }
            epd.ClearPart();
            vTaskDelay(pdMS_TO_TICKS(2000));
        }

#endif

        if (epd.Init(FULL) != 0)
        {
            snprintf(msg.body, 128, "e-Paper init failed...\r\n");
            msg.level = LOG_ERROR;
            xQueueSend(usbQueue, (void *)&msg, 0);
            return;
        }
        snprintf(msg.body, 128, "e-Paper clear and sleep\r\n");
        msg.level = LOG_INFO;
        xQueueSend(usbQueue, (void *)&msg, 0);
        epd.Clear();
        epd.Sleep();
#elif defined(EPD_2IN66)
        UBYTE image[500];
        Paint paint(image, 48, 80); // width should be the multiple of 8
        UDOUBLE time_start_ms;
        UDOUBLE time_now_s;
        static Epd2in66 epd(&SPI0);
        if (epd.Init() != 0)
        {
            snprintf(msg.body, 128, "e-Paper init failed...\r\n");
            msg.level = LOG_ERROR;
            xQueueSend(usbQueue, (void *)&msg, 0);
            return;
        }
        snprintf(msg.body, 128, "Starting 2.66inch e-Paper demo...\r\n ");
        msg.level = LOG_INFO;
        xQueueSend(usbQueue, (void *)&msg, 0);
        snprintf(msg.body, 128, "e-Paper Clear...\r\n ");
        msg.level = LOG_INFO;
        xQueueSend(usbQueue, (void *)&msg, 0);
        epd.Clear();

        paint.SetRotate(ROTATE_270);

#if 1
        snprintf(msg.body, 128, "draw image...\r\n ");
        msg.level = LOG_INFO;
        xQueueSend(usbQueue, (void *)&msg, 0);
        epd.DisplayFrame(IMAGE_DATA);
        vTaskDelay(pdMS_TO_TICKS(4000));
        epd.Clear();
#endif

#if 1
        if (epd.Init_Partial() != 0)
        {
            snprintf(msg.body, 128, "e-Paper init failed...\r\n");
            msg.level = LOG_ERROR;
            xQueueSend(usbQueue, (void *)&msg, 0);
            return;
        }
        epd.Clear();
        snprintf(msg.body, 128, "partial display___ \r\n ");
        msg.level = LOG_INFO;
        xQueueSend(usbQueue, (void *)&msg, 0);
        UBYTE i;
        time_start_ms = millis();
        for (i = 0; i < 10; i++)
        {
            time_now_s = (millis() - time_start_ms) / 1000;
            char time_string[] = {'0', '0', ':', '0', '0', '\0'};
            time_string[0] = time_now_s / 60 / 10 + '0';
            time_string[1] = time_now_s / 60 % 10 + '0';
            time_string[3] = time_now_s % 60 / 10 + '0';
            time_string[4] = time_now_s % 60 % 10 + '0';

            paint.Clear(UNCOLORED);
            paint.DrawStringAt(10, 10, time_string, &Font16, COLORED);
            snprintf(msg.body, 128, "refresh------\r\n ");
            msg.level = LOG_INFO;
            xQueueSend(usbQueue, (void *)&msg, 0);
            epd.DisplayFrame_part(paint.GetImage(), 20, 100, 48, 80);
        }
#endif

        if (epd.Init() != 0)
        {
            snprintf(msg.body, 128, "e-Paper init failed...\r\n");
            msg.level = LOG_ERROR;
            xQueueSend(usbQueue, (void *)&msg, 0);
            return;
        }
        snprintf(msg.body, 128, "clear and sleep......\r\n ");
        msg.level = LOG_INFO;
        xQueueSend(usbQueue, (void *)&msg, 0);
        epd.Clear();
        epd.Sleep();

#else
#error "No e-Paper display selected"
#endif

        vTaskDelay(pdMS_TO_TICKS(3000));
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
        NOTE_E2,
        NOTE_E2,
        NOTE_E4,
        NOTE_E2,
        NOTE_E2,
        NOTE_D4,
        NOTE_E2,
        NOTE_E2,
        NOTE_C4,
        NOTE_E2,
        NOTE_E2,
        NOTE_AS4,
        NOTE_E2,
        NOTE_E2,
        NOTE_B4,
        NOTE_C4,
        NOTE_E2,
        NOTE_E2,
        NOTE_E4,
        NOTE_E2,
        NOTE_E2,
        NOTE_D4,
        NOTE_E2,
        NOTE_E2,
        NOTE_C4,
        NOTE_E2,
        NOTE_E2,
        NOTE_AS4,
        NOTE_AS4,
        NOTE_AS4,
        NOTE_AS4,
        NOTE_REST,

        NOTE_E2,
        NOTE_E2,
        NOTE_E4,
        NOTE_E2,
        NOTE_E2,
        NOTE_D4,
        NOTE_E2,
        NOTE_E2,
        NOTE_C4,
        NOTE_E2,
        NOTE_E2,
        NOTE_AS4,
        NOTE_E2,
        NOTE_E2,
        NOTE_B4,
        NOTE_C4,
        NOTE_E2,
        NOTE_E2,
        NOTE_E4,
        NOTE_E2,
        NOTE_E2,
        NOTE_D4,
        NOTE_E2,
        NOTE_E2,
        NOTE_C4,
        NOTE_E2,
        NOTE_E2,
        NOTE_AS4,
        NOTE_AS4,
        NOTE_AS4,
        NOTE_AS4,
        NOTE_REST,

        NOTE_A3,
        NOTE_A3,
        NOTE_A5,
        NOTE_A3,
        NOTE_A3,
        NOTE_G4,
        NOTE_A3,
        NOTE_A3,
        NOTE_F4,
        NOTE_A3,
        NOTE_A3,
        NOTE_DS4,
        NOTE_A3,
        NOTE_A3,
        NOTE_E4,
        NOTE_F4,
        NOTE_A3,
        NOTE_A3,
        NOTE_A5,
        NOTE_A3,
        NOTE_A3,
        NOTE_G4,
        NOTE_A3,
        NOTE_A3,
        NOTE_F4,
        NOTE_A3,
        NOTE_A3,
        NOTE_DS4,
        NOTE_DS4,
        NOTE_DS4,
        NOTE_DS4,
        NOTE_REST,

    };
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
    xTaskCreate(usb_task, "USB Task", 1024, NULL, 3, &usbTaskHandle);
    xTaskCreate(display_task, "Display Task", 2048, NULL, 2, &displayTaskHandle);
    xTaskCreate(buzzer_task, "Buzzer Task", 1024, NULL, 1, &buzzerTaskHandle);
    xTaskCreate(button_task, "Button Task", 512, NULL, 1, &buttonTaskHandle);
}

void loop()
{
    // Not used when using FreeRTOS, but required by Arduino
    vTaskDelay(pdMS_TO_TICKS(1));
}