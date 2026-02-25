/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief  CLI-controlled audio player using esp_audio_simple_player
 *
 * This example demonstrates playback of audio from multiple media sources
 * (HTTP, SD card, flash) via a console-based interface. A lightweight
 * temporary playlist component manages the track list; it will be
 * replaced by the official playlist component once available.
 *
 * Console commands:
 *   play   – Start or restart the current track
 *   stop   – Stop playback
 *   pause  – Pause playback
 *   resume – Resume playback
 *   next   – Skip to the next track
 *   prev   – Go back to the previous track
 *   vol+   – Increase volume by 10
 *   vol-   – Decrease volume by 10
 *   mute   – Mute / unmute toggle
 *   status – Print player state and current track info
 */

#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_console.h"

#include "esp_audio_simple_player.h"
#include "esp_audio_simple_player_advance.h"
#include "esp_gmf_pipeline.h"
#include "esp_gmf_obj.h"
#include "esp_codec_dev.h"
#include "esp_gmf_app_setup_peripheral.h"
#include "esp_gmf_app_cli.h"
#include "esp_gmf_audio_helper.h"
#include "esp_embed_tone.h"
#include "esp_gmf_io_embed_flash.h"

#include "esp_gmf_playlist.h"

static const char *TAG = "CLI_SIMPLE_PLAYER";

/* ---------------------------------------------------------------------------
 *  Volume / mute state
 * --------------------------------------------------------------------------*/
#define DEFAULT_VOLUME   60
#define VOLUME_STEP      10
#define VOLUME_MAX       100
#define VOLUME_MIN       0

static int  s_volume     = DEFAULT_VOLUME;
static bool s_muted      = false;
static int  s_vol_backup = DEFAULT_VOLUME;

/* ---------------------------------------------------------------------------
 *  Global handles
 * --------------------------------------------------------------------------*/
static esp_asp_handle_t          s_player   = NULL;
static esp_gmf_playlist_handle_t s_playlist = NULL;
static void                     *s_sdcard   = NULL;

/* ---------------------------------------------------------------------------
 *  Output data callback – writes decoded PCM to the codec device
 * --------------------------------------------------------------------------*/
static int _out_write_cb(uint8_t *data, int data_size, void *ctx)
{
    return esp_codec_dev_write((esp_codec_dev_handle_t)ctx, data, data_size);
}

/* ---------------------------------------------------------------------------
 *  Embed-flash pre-run callback – configures flash context before pipeline runs
 * --------------------------------------------------------------------------*/
static int _embed_flash_prev_cb(esp_asp_handle_t *handle, void *ctx)
{
    esp_gmf_pipeline_handle_t pipe = NULL;
    esp_audio_simple_player_get_pipeline(handle, &pipe);
    if (pipe) {
        esp_gmf_io_handle_t flash_io = NULL;
        esp_gmf_pipeline_get_in(pipe, &flash_io);
        if (flash_io && (strcasecmp(OBJ_GET_TAG(flash_io), "io_embed_flash") == 0)) {
            esp_gmf_io_embed_flash_set_context(flash_io,
                (embed_item_info_t *)&g_esp_embed_tone[0], ESP_EMBED_TONE_URL_MAX);
        }
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 *  Player event callback
 * --------------------------------------------------------------------------*/
static int _player_event_cb(esp_asp_event_pkt_t *event, void *ctx)
{
    if (event->type == ESP_ASP_EVENT_TYPE_MUSIC_INFO) {
        esp_asp_music_info_t info = {0};
        memcpy(&info, event->payload, event->payload_size);
        ESP_LOGI(TAG, "Music info: rate=%d, ch=%d, bits=%d, bitrate=%d",
                 info.sample_rate, info.channels, info.bits, info.bitrate);
    } else if (event->type == ESP_ASP_EVENT_TYPE_STATE) {
        esp_asp_state_t st = ESP_ASP_STATE_NONE;
        memcpy(&st, event->payload, event->payload_size);
        ESP_LOGI(TAG, "Player state: %s", esp_audio_simple_player_state_to_str(st));

        if (st == ESP_ASP_STATE_FINISHED) {
            ESP_LOGI(TAG, "Track finished, advancing to next");
            const char *uri = NULL;
            if (esp_gmf_playlist_next(s_playlist, &uri) == ESP_OK) {
                esp_audio_simple_player_run(s_player, uri, NULL);
            }
        }
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 *  Helper: play the current playlist entry
 * --------------------------------------------------------------------------*/
static esp_err_t _play_current(void)
{
    const char *uri = NULL;
    if (esp_gmf_playlist_get_current(s_playlist, &uri) != ESP_OK) {
        ESP_LOGW(TAG, "Playlist is empty");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Playing [%d/%d]: %s",
             esp_gmf_playlist_get_current_index(s_playlist) + 1,
             esp_gmf_playlist_get_count(s_playlist), uri);

    esp_asp_state_t state;
    esp_audio_simple_player_get_state(s_player, &state);
    if (state == ESP_ASP_STATE_RUNNING || state == ESP_ASP_STATE_PAUSED) {
        esp_audio_simple_player_stop(s_player);
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    return esp_audio_simple_player_run(s_player, uri, NULL);
}

/* ---------------------------------------------------------------------------
 *  Helper: apply current volume to the codec device
 * --------------------------------------------------------------------------*/
static void _apply_volume(void)
{
    esp_codec_dev_handle_t dev = (esp_codec_dev_handle_t)esp_gmf_app_get_playback_handle();
    if (dev) {
        esp_codec_dev_set_out_vol(dev, s_muted ? 0 : s_volume);
    }
}

/* ---------------------------------------------------------------------------
 *  CLI command handlers
 * --------------------------------------------------------------------------*/
static int _cmd_play(int argc, char **argv)
{
    esp_err_t ret = _play_current();
    if (ret != ESP_OK) {
        printf("Failed to play (0x%x)\n", ret);
    }
    return 0;
}

static int _cmd_stop(int argc, char **argv)
{
    esp_audio_simple_player_stop(s_player);
    printf("Stopped\n");
    return 0;
}

static int _cmd_pause(int argc, char **argv)
{
    esp_err_t ret = esp_audio_simple_player_pause(s_player);
    printf("%s\n", (ret == ESP_OK) ? "Paused" : "Cannot pause");
    return 0;
}

static int _cmd_resume(int argc, char **argv)
{
    esp_err_t ret = esp_audio_simple_player_resume(s_player);
    printf("%s\n", (ret == ESP_OK) ? "Resumed" : "Cannot resume");
    return 0;
}

static int _cmd_next(int argc, char **argv)
{
    const char *uri = NULL;
    if (esp_gmf_playlist_next(s_playlist, &uri) != ESP_OK) {
        printf("No next track\n");
        return 0;
    }
    esp_audio_simple_player_stop(s_player);
    vTaskDelay(pdMS_TO_TICKS(50));
    printf("Next [%d/%d]: %s\n",
           esp_gmf_playlist_get_current_index(s_playlist) + 1,
           esp_gmf_playlist_get_count(s_playlist), uri);
    esp_audio_simple_player_run(s_player, uri, NULL);
    return 0;
}

static int _cmd_prev(int argc, char **argv)
{
    const char *uri = NULL;
    if (esp_gmf_playlist_prev(s_playlist, &uri) != ESP_OK) {
        printf("No previous track\n");
        return 0;
    }
    esp_audio_simple_player_stop(s_player);
    vTaskDelay(pdMS_TO_TICKS(50));
    printf("Prev [%d/%d]: %s\n",
           esp_gmf_playlist_get_current_index(s_playlist) + 1,
           esp_gmf_playlist_get_count(s_playlist), uri);
    esp_audio_simple_player_run(s_player, uri, NULL);
    return 0;
}

static int _cmd_vol_up(int argc, char **argv)
{
    s_volume += VOLUME_STEP;
    if (s_volume > VOLUME_MAX) {
        s_volume = VOLUME_MAX;
    }
    _apply_volume();
    printf("Volume: %d%s\n", s_volume, s_muted ? " (muted)" : "");
    return 0;
}

static int _cmd_vol_down(int argc, char **argv)
{
    s_volume -= VOLUME_STEP;
    if (s_volume < VOLUME_MIN) {
        s_volume = VOLUME_MIN;
    }
    _apply_volume();
    printf("Volume: %d%s\n", s_volume, s_muted ? " (muted)" : "");
    return 0;
}

static int _cmd_mute(int argc, char **argv)
{
    if (!s_muted) {
        s_vol_backup = s_volume;
        s_muted = true;
    } else {
        s_muted = false;
        s_volume = s_vol_backup;
    }
    _apply_volume();
    printf("Mute: %s (volume=%d)\n", s_muted ? "ON" : "OFF", s_volume);
    return 0;
}

static int _cmd_status(int argc, char **argv)
{
    esp_asp_state_t state;
    esp_audio_simple_player_get_state(s_player, &state);
    const char *uri = NULL;
    esp_gmf_playlist_get_current(s_playlist, &uri);
    printf("State  : %s\n", esp_audio_simple_player_state_to_str(state));
    printf("Track  : [%d/%d] %s\n",
           esp_gmf_playlist_get_current_index(s_playlist) + 1,
           esp_gmf_playlist_get_count(s_playlist),
           uri ? uri : "(none)");
    printf("Volume : %d%s\n", s_volume, s_muted ? " (muted)" : "");
    return 0;
}

/* ---------------------------------------------------------------------------
 *  CLI command registration
 * --------------------------------------------------------------------------*/
static void _register_player_commands(void)
{
    const esp_console_cmd_t cmds[] = {
        { .command = "play",   .help = "Play current track",      .func = &_cmd_play },
        { .command = "stop",   .help = "Stop playback",           .func = &_cmd_stop },
        { .command = "pause",  .help = "Pause playback",          .func = &_cmd_pause },
        { .command = "resume", .help = "Resume playback",         .func = &_cmd_resume },
        { .command = "next",   .help = "Next track",              .func = &_cmd_next },
        { .command = "prev",   .help = "Previous track",          .func = &_cmd_prev },
        { .command = "vol+",   .help = "Volume up (+10)",         .func = &_cmd_vol_up },
        { .command = "vol-",   .help = "Volume down (-10)",       .func = &_cmd_vol_down },
        { .command = "mute",   .help = "Toggle mute",             .func = &_cmd_mute },
        { .command = "status", .help = "Show player status",      .func = &_cmd_status },
    };
    for (int i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        ESP_ERROR_CHECK(esp_console_cmd_register(&cmds[i]));
    }
}

/* ---------------------------------------------------------------------------
 *  Playlist population: add demo tracks from various sources
 * --------------------------------------------------------------------------*/
static void _populate_playlist(void)
{
    /* SD card tracks */
    esp_gmf_playlist_add(s_playlist, "file://sdcard/test.mp3");

    /* Embedded flash tone */
    esp_gmf_playlist_add(s_playlist, esp_embed_tone_url[0]);

    /* HTTP stream */
    esp_gmf_playlist_add(s_playlist, "https://dl.espressif.com/dl/audio/gs-16b-2c-44100hz.mp3");
}

/* ---------------------------------------------------------------------------
 *  app_main
 * --------------------------------------------------------------------------*/
void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);
    ESP_LOGI(TAG, "=== CLI Simple Player Example ===");

    /* 1. Setup peripherals: codec device */
    esp_gmf_app_codec_info_t codec_info = ESP_GMF_APP_CODEC_INFO_DEFAULT();
    codec_info.play_info.sample_rate = CONFIG_GMF_AUDIO_EFFECT_RATE_CVT_DEST_RATE;
    codec_info.play_info.channel     = CONFIG_GMF_AUDIO_EFFECT_CH_CVT_DEST_CH;
    codec_info.play_info.bits_per_sample = CONFIG_GMF_AUDIO_EFFECT_BIT_CVT_DEST_BITS;
    codec_info.record_info = codec_info.play_info;
    esp_gmf_app_setup_codec_dev(&codec_info);

    /* Set initial volume */
    esp_codec_dev_set_out_vol(
        (esp_codec_dev_handle_t)esp_gmf_app_get_playback_handle(), s_volume);

    /* 2. Mount SD card */
    esp_gmf_app_setup_sdcard(&s_sdcard);

    /* 3. Connect Wi-Fi (needed for HTTP playback) */
    esp_gmf_app_wifi_connect();

    /* 4. Create playlist and populate with demo tracks */
    esp_gmf_playlist_create(&s_playlist);
    _populate_playlist();

    /* 5. Create the simple player */
    esp_asp_cfg_t player_cfg = {
        .out.cb       = _out_write_cb,
        .out.user_ctx = esp_gmf_app_get_playback_handle(),
        .task_prio    = 5,
        .task_stack   = 0,
        .prev         = _embed_flash_prev_cb,
        .prev_ctx     = NULL,
    };
    ESP_ERROR_CHECK(esp_audio_simple_player_new(&player_cfg, &s_player));
    ESP_ERROR_CHECK(esp_audio_simple_player_set_event(s_player, _player_event_cb, NULL));

    ESP_LOGI(TAG, "Player ready. Tracks in playlist: %d", esp_gmf_playlist_get_count(s_playlist));
    ESP_LOGI(TAG, "Type 'help' for available commands");

    /* 6. Start CLI */
    esp_gmf_app_cli_init("player> ", _register_player_commands);
}
