/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

typedef struct {
    const uint8_t *address;
    int            size;
} esp_embed_tone_t;

extern const uint8_t alarm_mp3[] asm("_binary_alarm_mp3_start");

esp_embed_tone_t g_esp_embed_tone[] = {
    [0] = {
        .address = alarm_mp3,
        .size    = 36018,
    },
};

enum esp_embed_tone_index {
    ESP_EMBED_TONE_ALARM_MP3 = 0,
    ESP_EMBED_TONE_URL_MAX   = 1,
};

const char *esp_embed_tone_url[] = {
    "embed://tone/0_alarm.mp3",
};
