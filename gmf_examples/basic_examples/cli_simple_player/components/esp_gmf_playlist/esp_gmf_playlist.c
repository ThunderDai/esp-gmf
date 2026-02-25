/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_gmf_playlist.h"

static const char *TAG = "GMF_PLAYLIST";

#define PLAYLIST_MAX_TRACKS  64

typedef struct {
    char *uris[PLAYLIST_MAX_TRACKS];
    int   count;
    int   current;
} playlist_t;

esp_err_t esp_gmf_playlist_create(esp_gmf_playlist_handle_t *handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    playlist_t *pl = calloc(1, sizeof(playlist_t));
    if (pl == NULL) {
        ESP_LOGE(TAG, "Failed to allocate playlist");
        return ESP_ERR_NO_MEM;
    }
    pl->current = -1;
    *handle = pl;
    return ESP_OK;
}

esp_err_t esp_gmf_playlist_destroy(esp_gmf_playlist_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    playlist_t *pl = (playlist_t *)handle;
    for (int i = 0; i < pl->count; i++) {
        free(pl->uris[i]);
    }
    free(pl);
    return ESP_OK;
}

esp_err_t esp_gmf_playlist_add(esp_gmf_playlist_handle_t handle, const char *uri)
{
    if (handle == NULL || uri == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    playlist_t *pl = (playlist_t *)handle;
    if (pl->count >= PLAYLIST_MAX_TRACKS) {
        ESP_LOGW(TAG, "Playlist full (%d tracks)", PLAYLIST_MAX_TRACKS);
        return ESP_ERR_NO_MEM;
    }
    pl->uris[pl->count] = strdup(uri);
    if (pl->uris[pl->count] == NULL) {
        return ESP_ERR_NO_MEM;
    }
    if (pl->count == 0) {
        pl->current = 0;
    }
    pl->count++;
    ESP_LOGI(TAG, "Added [%d]: %s", pl->count - 1, uri);
    return ESP_OK;
}

esp_err_t esp_gmf_playlist_get_current(esp_gmf_playlist_handle_t handle, const char **uri)
{
    if (handle == NULL || uri == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    playlist_t *pl = (playlist_t *)handle;
    if (pl->count == 0 || pl->current < 0) {
        return ESP_ERR_NOT_FOUND;
    }
    *uri = pl->uris[pl->current];
    return ESP_OK;
}

esp_err_t esp_gmf_playlist_next(esp_gmf_playlist_handle_t handle, const char **uri)
{
    if (handle == NULL || uri == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    playlist_t *pl = (playlist_t *)handle;
    if (pl->count == 0) {
        return ESP_ERR_NOT_FOUND;
    }
    pl->current = (pl->current + 1) % pl->count;
    *uri = pl->uris[pl->current];
    return ESP_OK;
}

esp_err_t esp_gmf_playlist_prev(esp_gmf_playlist_handle_t handle, const char **uri)
{
    if (handle == NULL || uri == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    playlist_t *pl = (playlist_t *)handle;
    if (pl->count == 0) {
        return ESP_ERR_NOT_FOUND;
    }
    pl->current = (pl->current - 1 + pl->count) % pl->count;
    *uri = pl->uris[pl->current];
    return ESP_OK;
}

int esp_gmf_playlist_get_count(esp_gmf_playlist_handle_t handle)
{
    if (handle == NULL) {
        return 0;
    }
    return ((playlist_t *)handle)->count;
}

int esp_gmf_playlist_get_current_index(esp_gmf_playlist_handle_t handle)
{
    if (handle == NULL) {
        return -1;
    }
    return ((playlist_t *)handle)->current;
}

bool esp_gmf_playlist_is_empty(esp_gmf_playlist_handle_t handle)
{
    if (handle == NULL) {
        return true;
    }
    return ((playlist_t *)handle)->count == 0;
}
