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

void setup()
{
    Serial.begin(115200);
    sleep_ms(3000);
    Serial.println("Aerify Digital Business Card Starting...");
    usbQueue = xQueueCreate(MSG_QUEUE_LEN, sizeof(Message_t));

    xTaskCreate(usb_task, "USB Task", 1024, NULL, 1, &usbTaskHandle);
}