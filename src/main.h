#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "semphr.h"
#include <vector>
#include "debug.h"
#include "pindefs.h"
#include "buzzer.h"
#include "buttons.h"
#include "sd_card.h"
#include "message.h"
#include "ePaperDisplay/epd.h"
#include "ePaperDisplay/epdpaint.h"
#include "atecc508a.h"
#include "atsha204a.h"
#include "lvgl.h"
#include "lvgl_epd_glue.h"
#include "aerid_sdk/aerid_sdk.h"
#include "aerid_sdk/aerid_sdk_internal.h"

static QueueHandle_t usbQueue = NULL;
static QueueHandle_t launcherQueue = NULL;
static QueueHandle_t displayQueue = NULL;
static QueueHandle_t buzzerQueue = NULL;

static TaskHandle_t usbTaskHandle = NULL;
static TaskHandle_t launcherTaskHandle = NULL;
static TaskHandle_t bmsTaskHandle = NULL;
static TaskHandle_t sdTaskHandle = NULL;
static TaskHandle_t displayTaskHandle = NULL;
static TaskHandle_t buzzerTaskHandle = NULL;
static TaskHandle_t buttonTaskHandle = NULL;
static TaskHandle_t i2cScanTaskHandle = NULL;
static TaskHandle_t ateccTaskHandle = NULL;
static TaskHandle_t atshaTaskHandle = NULL;
static TaskHandle_t nfcTaskHandle = NULL;

SemaphoreHandle_t spi0_mutex = NULL;
SemaphoreHandle_t i2c_default_mutex = NULL;
SemaphoreHandle_t adc_mutex = NULL;

SPIClassRP2040 SPI0(spi0, SPI0_MISO_PIN, SPI0_CS_PIN, SPI0_SCK_PIN, SPI0_MOSI_PIN);

TwoWire I2C0(i2c_default, I2C0_SDA_PIN, I2C0_SCL_PIN);
TwoWire I2C1(i2c1, I2C1_SDA_PIN, I2C1_SCL_PIN);

volatile uint32_t stat1_transitions = 0;
volatile uint32_t stat2_transitions = 0;

void stat_gpio_callback(uint gpio, uint32_t events)
{
    Message_t msg;
    if (gpio == BAT_STAT1_PIN)
    {
        snprintf(msg.body, 128, "STAT1 Interrupt\r\n");
        msg.level = LOG_DEBUG;
        xQueueSendFromISR(usbQueue, (void *)&msg, 0);
        stat1_transitions++;
    }
    if (gpio == BAT_STAT2_PIN)
    {
        snprintf(msg.body, 128, "STAT2 Interrupt\r\n");
        msg.level = LOG_DEBUG;
        xQueueSendFromISR(usbQueue, (void *)&msg, 0);
        stat2_transitions++;
    }
}

void btn_gpio_callback(uint gpio, uint32_t events)
{

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

    // TODO: check for long press / repeat here
    Message_t msg;
    switch (gpio)
    {
    case BTN_DPAD_UP_PIN:
        handle_input_event(AERID_BUTTON_UP, events == GPIO_IRQ_EDGE_FALL ? AERID_INPUT_EVENT_PRESS : AERID_INPUT_EVENT_RELEASE);
        break;
    case BTN_DPAD_DOWN_PIN:
        handle_input_event(AERID_BUTTON_DOWN, events == GPIO_IRQ_EDGE_FALL ? AERID_INPUT_EVENT_PRESS : AERID_INPUT_EVENT_RELEASE);
        break;
    case BTN_DPAD_LEFT_PIN:
        handle_input_event(AERID_BUTTON_LEFT, events == GPIO_IRQ_EDGE_FALL ? AERID_INPUT_EVENT_PRESS : AERID_INPUT_EVENT_RELEASE);
        break;
    case BTN_DPAD_RIGHT_PIN:
        handle_input_event(AERID_BUTTON_RIGHT, events == GPIO_IRQ_EDGE_FALL ? AERID_INPUT_EVENT_PRESS : AERID_INPUT_EVENT_RELEASE);
        break;
    case BTN_DPAD_CENTER_PIN:
        handle_input_event(AERID_BUTTON_CENTER, events == GPIO_IRQ_EDGE_FALL ? AERID_INPUT_EVENT_PRESS : AERID_INPUT_EVENT_RELEASE);
        break;
    case BTN_A_PIN:
        handle_input_event(AERID_BUTTON_A, events == GPIO_IRQ_EDGE_FALL ? AERID_INPUT_EVENT_PRESS : AERID_INPUT_EVENT_RELEASE);
        break;
    case BTN_B_PIN:
        handle_input_event(AERID_BUTTON_B, events == GPIO_IRQ_EDGE_FALL ? AERID_INPUT_EVENT_PRESS : AERID_INPUT_EVENT_RELEASE);
        break;
    default:
        // Invalid GPIO
        break;
    }
}

// These are any addresses of the form 000 0xxx or 111 1xxx
bool reserved_addr(uint8_t addr)
{
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}