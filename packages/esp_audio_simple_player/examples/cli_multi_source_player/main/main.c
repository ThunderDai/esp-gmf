/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "esp_audio_simple_player.h"
#include "esp_audio_simple_player_advance.h"
#include "esp_console.h"
#include "esp_gmf_app_cli.h"
#include "esp_gmf_app_setup_peripheral.h"
#include "esp_gmf_io.h"
#include "esp_gmf_io_embed_flash.h"
#include "esp_gmf_pipeline.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_codec_dev.h"
#include "esp_embed_tone.h"

static const char *TAG = "CLI_MUSIC";

typedef enum {
    SRC_HTTP = 0,
    SRC_SDCARD,
} audio_src_t;

typedef struct {
    audio_src_t source;
    int         index;
} track_cursor_t;

typedef struct {
    esp_asp_handle_t handle;
    track_cursor_t current;
    track_cursor_t resume_track;
    bool flash_resume_pending;
    bool flash_playing;
} app_ctx_t;

static app_ctx_t s_app = {
    .current = {.source = SRC_HTTP, .index = 0},
};

static const char *s_http_playlist[] = {
    "https://dl.espressif.com/dl/audio/ff-16b-2c-44100hz.mp3",
    "https://dl.espressif.com/dl/audio/gs-16b-2c-44100hz.mp3",
};

static const char *s_sd_playlist[] = {
    "file://sdcard/test.mp3",
    "file://sdcard/test.aac",
};

static inline int playlist_size(audio_src_t src)
{
    return (src == SRC_HTTP) ? (sizeof(s_http_playlist) / sizeof(s_http_playlist[0])) : (sizeof(s_sd_playlist) / sizeof(s_sd_playlist[0]));
}

static const char *track_uri(const track_cursor_t *cursor)
{
    if (cursor->source == SRC_HTTP) {
        return s_http_playlist[cursor->index];
    }
    return s_sd_playlist[cursor->index];
}

static void normalize_cursor(track_cursor_t *cursor)
{
    int size = playlist_size(cursor->source);
    if (size <= 0) {
        cursor->index = 0;
        return;
    }
    while (cursor->index < 0) {
        cursor->index += size;
    }
    cursor->index %= size;
}

static int out_data_cb(uint8_t *data, int data_len, void *ctx)
{
    return esp_codec_dev_write(ctx, data, data_len);
}

static int embed_flash_io_set(esp_asp_handle_t *handle, void *ctx)
{
    (void)ctx;
    esp_gmf_pipeline_handle_t pipe = NULL;
    esp_gmf_io_handle_t flash = NULL;
    int ret = esp_audio_simple_player_get_pipeline(handle, &pipe);
    if (ret != ESP_GMF_ERR_OK || pipe == NULL) {
        return ret;
    }
    ret = esp_gmf_pipeline_get_in(pipe, &flash);
    if (ret == ESP_GMF_ERR_OK && flash && strcasecmp(OBJ_GET_TAG(flash), "io_embed_flash") == 0) {
        ret = esp_gmf_io_embed_flash_set_context(flash, g_esp_embed_tone, ESP_EMBED_TONE_URL_MAX);
    }
    return ret;
}

static int event_cb(esp_asp_event_pkt_t *pkt, void *ctx)
{
    app_ctx_t *app = (app_ctx_t *)ctx;
    if (pkt->type == ESP_ASP_EVENT_TYPE_STATE && pkt->payload_size == sizeof(esp_asp_state_t)) {
        esp_asp_state_t state = *(esp_asp_state_t *)pkt->payload;
        ESP_LOGI(TAG, "state=%s", esp_audio_simple_player_state_to_str(state));
        if (app->flash_playing && state == ESP_ASP_STATE_FINISHED) {
            app->flash_playing = false;
            if (app->flash_resume_pending) {
                app->flash_resume_pending = false;
                app->current = app->resume_track;
                normalize_cursor(&app->current);
                const char *resume_uri = track_uri(&app->current);
                ESP_LOGI(TAG, "flash 播放结束，恢复之前音源: %s", resume_uri);
                esp_audio_simple_player_run(app->handle, resume_uri, NULL);
            }
        }
    }
    return ESP_GMF_ERR_OK;
}

static esp_err_t play_track(const track_cursor_t *cursor)
{
    const char *uri = track_uri(cursor);
    ESP_LOGI(TAG, "play: %s", uri);
    esp_gmf_err_t ret = esp_audio_simple_player_run(s_app.handle, uri, NULL);
    return (ret == ESP_GMF_ERR_OK) ? ESP_OK : ESP_FAIL;
}

static esp_err_t cmd_play(int argc, char **argv)
{
    if (argc >= 2) {
        if (strcasecmp(argv[1], "http") == 0) {
            s_app.current.source = SRC_HTTP;
        } else if (strcasecmp(argv[1], "sd") == 0 || strcasecmp(argv[1], "sdcard") == 0) {
            s_app.current.source = SRC_SDCARD;
        } else {
            ESP_LOGW(TAG, "未知音源: %s (支持 http/sdcard)", argv[1]);
            return ESP_ERR_INVALID_ARG;
        }
    }
    if (argc >= 3) {
        s_app.current.index = atoi(argv[2]);
    }
    normalize_cursor(&s_app.current);
    s_app.flash_playing = false;
    s_app.flash_resume_pending = false;
    return play_track(&s_app.current);
}

static esp_err_t cmd_pause(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    return esp_audio_simple_player_pause(s_app.handle);
}

static esp_err_t cmd_resume(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    return esp_audio_simple_player_resume(s_app.handle);
}

static esp_err_t cmd_stop(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    s_app.flash_playing = false;
    s_app.flash_resume_pending = false;
    return esp_audio_simple_player_stop(s_app.handle);
}

static esp_err_t cmd_next(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    s_app.current.index++;
    normalize_cursor(&s_app.current);
    s_app.flash_playing = false;
    s_app.flash_resume_pending = false;
    return play_track(&s_app.current);
}

static esp_err_t cmd_prev(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    s_app.current.index--;
    normalize_cursor(&s_app.current);
    s_app.flash_playing = false;
    s_app.flash_resume_pending = false;
    return play_track(&s_app.current);
}

static esp_err_t cmd_flash(int argc, char **argv)
{
    if (argc < 2) {
        ESP_LOGI(TAG, "flash 用法: flash <interrupt|resume> [tone_idx]");
        return ESP_ERR_INVALID_ARG;
    }

    int tone_idx = 0;
    if (argc >= 3) {
        tone_idx = atoi(argv[2]);
    }
    if (tone_idx < 0 || tone_idx >= ESP_EMBED_TONE_URL_MAX) {
        ESP_LOGW(TAG, "tone_idx 范围: 0 ~ %d", ESP_EMBED_TONE_URL_MAX - 1);
        return ESP_ERR_INVALID_ARG;
    }

    esp_asp_state_t state = ESP_ASP_STATE_NONE;
    esp_audio_simple_player_get_state(s_app.handle, &state);

    bool recover_mode = (strcasecmp(argv[1], "resume") == 0);
    if (!recover_mode && strcasecmp(argv[1], "interrupt") != 0) {
        ESP_LOGW(TAG, "flash 模式仅支持 interrupt/resume");
        return ESP_ERR_INVALID_ARG;
    }

    if (recover_mode && (state == ESP_ASP_STATE_RUNNING || state == ESP_ASP_STATE_PAUSED)) {
        s_app.resume_track = s_app.current;
        s_app.flash_resume_pending = true;
    } else {
        s_app.flash_resume_pending = false;
    }

    esp_audio_simple_player_stop(s_app.handle);
    s_app.flash_playing = true;
    ESP_LOGI(TAG, "play flash tone: %s, mode=%s", esp_embed_tone_url[tone_idx], recover_mode ? "resume" : "interrupt");
    return esp_audio_simple_player_run(s_app.handle, esp_embed_tone_url[tone_idx], NULL);
}

static void register_cmds(void)
{
    const esp_console_cmd_t cmds[] = {
        {.command = "play", .help = "play [http|sdcard] [index]", .func = cmd_play},
        {.command = "pause", .help = "pause current track", .func = cmd_pause},
        {.command = "resume", .help = "resume current track", .func = cmd_resume},
        {.command = "stop", .help = "stop current track", .func = cmd_stop},
        {.command = "next", .help = "play next track in current source", .func = cmd_next},
        {.command = "prev", .help = "play previous track in current source", .func = cmd_prev},
        {.command = "flash", .help = "flash <interrupt|resume> [tone_idx]", .func = cmd_flash},
    };

    for (size_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        ESP_ERROR_CHECK(esp_console_cmd_register(&cmds[i]));
    }
}

void app_main(void)
{
    esp_gmf_app_setup_codec_dev(NULL);
    esp_gmf_app_setup_sdcard(NULL);
    esp_gmf_app_wifi_connect();

    esp_asp_cfg_t cfg = {
        .out.cb = out_data_cb,
        .out.user_ctx = esp_gmf_app_get_playback_handle(),
        .task_prio = 5,
        .prev = embed_flash_io_set,
    };

    ESP_ERROR_CHECK(esp_audio_simple_player_new(&cfg, &s_app.handle));
    ESP_ERROR_CHECK(esp_audio_simple_player_set_event(s_app.handle, event_cb, &s_app));
    ESP_ERROR_CHECK(esp_gmf_app_cli_init("gmf_music> ", register_cmds));

    ESP_LOGI(TAG, "示例已启动，可用命令: play/pause/resume/stop/prev/next/flash");
    ESP_LOGI(TAG, "示例: play http 0 | play sdcard 1 | flash interrupt 0 | flash resume 1");
}
