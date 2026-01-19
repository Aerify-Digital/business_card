#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
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

typedef struct
{
    char body[128];
    LogLevel_t level = LOG_NONE;
} Message_t;

static const int MSG_QUEUE_LEN = 64;

static QueueHandle_t usbQueue = NULL;

static TaskHandle_t usbTaskHandle = NULL;
