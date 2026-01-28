#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

typedef enum
{
    BUZZER_ON,
    BUZZER_OFF,
    BUZZER_CHIRP,
    BUZZER_ALARM
} BuzzType_t;

typedef struct
{
    BuzzType_t type;
    uint16_t duration_ms; // Only used for CHIRP
    // TODO: add fields for frequency, etc.
} BuzzerCommand_t;

typedef enum
{
    NOTE_REST = 0,
    NOTE_A0 = 21,
    NOTE_AS0,
    NOTE_B0,
    NOTE_C0,
    NOTE_CS0,
    NOTE_D0,
    NOTE_DS0,
    NOTE_E0,
    NOTE_F0,
    NOTE_FS0,
    NOTE_G0,
    NOTE_GS0,
    NOTE_A1,
    NOTE_AS1,
    NOTE_B1,
    NOTE_C1,
    NOTE_CS1,
    NOTE_D1,
    NOTE_DS1,
    NOTE_E1,
    NOTE_F1,
    NOTE_FS1,
    NOTE_G1,
    NOTE_GS1,
    NOTE_A2,
    NOTE_AS2,
    NOTE_B2,
    NOTE_C2,
    NOTE_CS2,
    NOTE_D2,
    NOTE_DS2,
    NOTE_E2,
    NOTE_F2,
    NOTE_FS2,
    NOTE_G2,
    NOTE_GS2,
    NOTE_A3,
    NOTE_AS3,
    NOTE_B3,
    NOTE_C3,
    NOTE_CS3,
    NOTE_D3,
    NOTE_DS3,
    NOTE_E3,
    NOTE_F3,
    NOTE_FS3,
    NOTE_G3,
    NOTE_GS3,
    NOTE_A4,
    NOTE_AS4,
    NOTE_B4,
    NOTE_C4,
    NOTE_CS4,
    NOTE_D4,
    NOTE_DS4,
    NOTE_E4,
    NOTE_F4,
    NOTE_FS4,
    NOTE_G4,
    NOTE_GS4,
    NOTE_A5,
    NOTE_AS5,
    NOTE_B5,
    NOTE_C5,
    NOTE_CS5,
    NOTE_D5,
    NOTE_DS5,
    NOTE_E5,
    NOTE_F5,
    NOTE_FS5,
    NOTE_G5,
    NOTE_GS5,
    NOTE_A6,
    NOTE_AS6,
    NOTE_B6,
    NOTE_C6,
    NOTE_CS6,
    NOTE_D6,
    NOTE_DS6,
    NOTE_E6,
    NOTE_F6,
    NOTE_FS6,
    NOTE_G6,
    NOTE_GS6,
    NOTE_A7,
    NOTE_AS7,
    NOTE_B7,
    NOTE_C7,
    NOTE_CS7,
    NOTE_D7,
    NOTE_DS7,
    NOTE_E7,
    NOTE_F7,
    NOTE_FS7,
    NOTE_G7,
    NOTE_GS7,
    NOTE_A8,
    NOTE_AS8,
    NOTE_B8,
    NOTE_C8
} Note_t;

// Returns frequency in Hz for a given MIDI note number (A4 = 69 = 440 Hz)
float midi_to_freq(int midi_note)
{
    if (midi_note <= 0)
        return 0.0f; // treat 0 or negative as rest
    return 440.0f * powf(2.0f, (midi_note - NOTE_A4) / 12.0f);
}

// Software tone generator: toggles pin at desired frequency
void play_tone(uint pin, float freq_hz, int duration_ms)
{
    if (freq_hz <= 0.0f)
    {
        sleep_ms(duration_ms);
        return;
    }
    int cycles = (int)(freq_hz * duration_ms / 1000.0f);
    float half_period_us = 500000.0f / freq_hz;
    for (int i = 0; i < cycles; ++i)
    {
        gpio_put(pin, 1);
        sleep_us((uint32_t)half_period_us);
        gpio_put(pin, 0);
        sleep_us((uint32_t)half_period_us);
    }
    // Silence for any remaining time
    int played_ms = (int)(cycles * 2 * half_period_us / 1000.0f);
    if (played_ms < duration_ms)
        sleep_ms(duration_ms - played_ms);
    gpio_put(pin, 0);
};

#endif