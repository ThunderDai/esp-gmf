/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>

typedef struct {
    const uint8_t *address;
    int           size;
} esp_embed_tone_t;

/*
 * NOTE:
 * To keep this example PR binary-free, we use tiny placeholder tone payloads.
 * For real prompt playback, replace these arrays with valid embedded audio data.
 */
static const uint8_t s_tone_placeholder_0[] = {0x00};
static const uint8_t s_tone_placeholder_1[] = {0x00};

enum esp_embed_tone_index {
    ESP_EMBED_TONE_ALARM_MP3 = 0,
    ESP_EMBED_TONE_FF_16B_1C_44100HZ_MP3 = 1,
    ESP_EMBED_TONE_URL_MAX = 2
};

static esp_embed_tone_t g_esp_embed_tone[] = {
    [ESP_EMBED_TONE_ALARM_MP3] = {
        .address = s_tone_placeholder_0,
        .size    = sizeof(s_tone_placeholder_0),
    },
    [ESP_EMBED_TONE_FF_16B_1C_44100HZ_MP3] = {
        .address = s_tone_placeholder_1,
        .size    = sizeof(s_tone_placeholder_1),
    },
};

static const char *esp_embed_tone_url[] = {
    "embed://tone/0_alarm.mp3",
    "embed://tone/1_ff_16b_1c_44100hz.mp3",
};
