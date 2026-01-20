#include "main.h"

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

    if (epd.Init(FAST) != 0)
    {
        snprintf(msg.body, 128, "e-Paper init failed...\r\n");
        msg.level = LOG_ERROR;
        xQueueSend(usbQueue, (void *)&msg, 0);
        return;
    }
    snprintf(msg.body, 128, "2.13inch e-Paper demo...\r\n ");
    msg.level = LOG_INFO;
    xQueueSend(usbQueue, (void *)&msg, 0);
    epd.Display_Fast(IMAGE_DATA_2IN13);

#if 1
    snprintf(msg.body, 128, "epd FULL\r\n");
    msg.level = LOG_INFO;
    xQueueSend(usbQueue, (void *)&msg, 0);
    epd.Init(FULL);
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
        epd.Init(PART);
        epd.DisplayPart(IMAGE_DATA_2IN13);
        snprintf(msg.body, 128, "e-Paper PART Clear\r\n");
        msg.level = LOG_INFO;
        xQueueSend(usbQueue, (void *)&msg, 0);
        epd.Init(PART);
        epd.ClearPart();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

#endif

    epd.Init(FULL);
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
    snprintf(msg.body, 128, "2.66inch e-Paper demo...\r\n ");
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
    epd.Init_Partial();
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

    epd.Init();
    snprintf(msg.body, 128, "clear and sleep......\r\n ");
    msg.level = LOG_INFO;
    xQueueSend(usbQueue, (void *)&msg, 0);
    epd.Clear();
    epd.Sleep();

#else
#error "No e-Paper display selected"
#endif

    while (1)
    {
        // Display handling code would go here

        vTaskDelay(pdMS_TO_TICKS(300));
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

    const float melody[] = {
        midi_to_freq(NOTE_C1), midi_to_freq(NOTE_C3), midi_to_freq(NOTE_CS1), midi_to_freq(NOTE_C2),
        midi_to_freq(NOTE_C1), midi_to_freq(NOTE_CS3), midi_to_freq(NOTE_FS1), midi_to_freq(NOTE_F2),
        midi_to_freq(NOTE_G1), midi_to_freq(NOTE_G3), midi_to_freq(NOTE_A2), midi_to_freq(NOTE_G2),
        midi_to_freq(NOTE_G1), midi_to_freq(NOTE_GS3), midi_to_freq(NOTE_G1), midi_to_freq(NOTE_F2)};

    const int note_count = sizeof(melody) / sizeof(melody[0]);
    const int note_duration_ms = 208; // 144 BPM 1/8 notes
    while (1)
    {
        for (int i = 0; i < note_count; ++i)
        {
            play_tone(BUZZER_PIN, melody[i], note_duration_ms);
        }
        vTaskDelay(pdMS_TO_TICKS(1)); // brief pause between notes
    }
}

void setup()
{
    Serial.begin(115200);
    sleep_ms(3000);
    Serial.println("Aerify Digital Business Card Starting...");
    usbQueue = xQueueCreate(MSG_QUEUE_LEN, sizeof(Message_t));
    displayQueue = xQueueCreate(MSG_QUEUE_LEN, sizeof(Message_t)); // TODO: add a display command struct
    buzzerQueue = xQueueCreate(MSG_QUEUE_LEN, sizeof(BuzzerCommand_t));
    pinMode(SPI0_CS_PIN, OUTPUT);
    pinMode(EPD_RST_PIN, OUTPUT);
    pinMode(EPD_DC_PIN, OUTPUT);
    pinMode(EPD_BUSY_PIN, INPUT);
    SPI0.begin();
    SPI0.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
    // displayQueue = xQueueCreate(16, sizeof(Message_t));
    xTaskCreate(usb_task, "USB Task", 1024, NULL, 1, &usbTaskHandle);
    xTaskCreate(display_task, "Display Task", 4096, NULL, 1, &displayTaskHandle);
    xTaskCreate(buzzer_task, "Buzzer Task", 1024, NULL, 1, &buzzerTaskHandle);
}

void loop()
{
    // Not used when using FreeRTOS, but required by Arduino
    vTaskDelay(pdMS_TO_TICKS(1));
}