#include "aerid_sdk/aerid_input.h"
#include <stdint.h>

static volatile aerid_input_callback_t input_callbacks[AERID_INPUT_COUNT] = {0};

static volatile uint8_t input_states[AERID_INPUT_COUNT] = {0};

int aerid_input_read(aerid_input_t input)
{
    if (input >= 0 && input < AERID_INPUT_COUNT)
        return input_states[input];
    return 0;
}

void aerid_input_register_callback(aerid_input_t input, aerid_input_callback_t callback)
{
    if (input >= 0 && input < AERID_INPUT_COUNT)
    {
        input_callbacks[input] = callback;
    }
}

void handle_input_event(aerid_input_t input, aerid_input_event_t event)
{
    if (input >= 0 && input < AERID_INPUT_COUNT)
    {
        if (event == AERID_INPUT_EVENT_PRESS || event == AERID_INPUT_EVENT_REPEAT)
            input_states[input] = 1;
        else if (event == AERID_INPUT_EVENT_RELEASE)
            input_states[input] = 0;

        if (input_callbacks[input])
            input_callbacks[input](input, event);
    }
}

void aerid_input_clear_callbacks()
{
    for (int i = 0; i < AERID_INPUT_COUNT; ++i)
    {
        input_callbacks[i] = nullptr;
    }
}

void aerid_input_clear_callback(aerid_input_t input)
{
    if (input >= 0 && input < AERID_INPUT_COUNT)
    {
        input_callbacks[input] = nullptr;
    }
}