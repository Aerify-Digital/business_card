#ifndef MESSAGE_H
#define MESSAGE_H
#include <Arduino.h>
#include "debug.h"

typedef struct
{
    char body[128];
    LogLevel_t level = LOG_NONE;
} Message_t;

static const int MSG_QUEUE_LEN = 16;

#endif