#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "semphr.h"
#include <vector>
#include "debug.h"
#include "pindefs.h"
#include "buzzer.h"
#include "buttons.h"
#include "ePaperDisplay/epd.h"
#include "ePaperDisplay/epdpaint.h"

typedef struct
{
    char body[128];
    LogLevel_t level = LOG_NONE;
} Message_t;

static const int MSG_QUEUE_LEN = 16;

static QueueHandle_t usbQueue = NULL;
static QueueHandle_t displayQueue = NULL;
static QueueHandle_t buzzerQueue = NULL;

static TaskHandle_t usbTaskHandle = NULL;
static TaskHandle_t displayTaskHandle = NULL;
static TaskHandle_t buzzerTaskHandle = NULL;

SPIClassRP2040 SPI0(spi0, SPI0_MISO_PIN, SPI0_CS_PIN, SPI0_SCK_PIN, SPI0_MOSI_PIN);

uint32_t last_press_time[BUTTON_COUNT] = {0};
