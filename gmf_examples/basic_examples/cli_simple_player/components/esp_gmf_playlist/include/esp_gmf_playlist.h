/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file esp_gmf_playlist.h
 * @brief  Temporary playlist manager for ESP-GMF examples
 *
 * @note  This is a lightweight temporary implementation. It will be replaced
 *        by the official esp_gmf_playlist component once available.
 *        The public API is designed to remain stable across that transition.
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *esp_gmf_playlist_handle_t;

/**
 * @brief  Create a new playlist instance
 *
 * @param[out]  handle  Pointer to store the created playlist handle
 *
 * @return
 *       - ESP_OK    On success
 *       - ESP_FAIL  On failure
 */
esp_err_t esp_gmf_playlist_create(esp_gmf_playlist_handle_t *handle);

/**
 * @brief  Destroy the playlist instance and free all resources
 *
 * @param[in]  handle  Playlist handle
 *
 * @return
 *       - ESP_OK    On success
 *       - ESP_FAIL  On failure
 */
esp_err_t esp_gmf_playlist_destroy(esp_gmf_playlist_handle_t handle);

/**
 * @brief  Add a URI to the end of the playlist
 *
 * @param[in]  handle  Playlist handle
 * @param[in]  uri     Null-terminated URI string
 *
 * @return
 *       - ESP_OK    On success
 *       - ESP_FAIL  On failure
 */
esp_err_t esp_gmf_playlist_add(esp_gmf_playlist_handle_t handle, const char *uri);

/**
 * @brief  Get the URI of the current track
 *
 * @param[in]   handle  Playlist handle
 * @param[out]  uri     Pointer to receive the URI string (do NOT free)
 *
 * @return
 *       - ESP_OK              On success
 *       - ESP_ERR_NOT_FOUND   Playlist is empty
 */
esp_err_t esp_gmf_playlist_get_current(esp_gmf_playlist_handle_t handle, const char **uri);

/**
 * @brief  Advance to the next track and return its URI
 *
 * @param[in]   handle  Playlist handle
 * @param[out]  uri     Pointer to receive the URI string (do NOT free)
 *
 * @return
 *       - ESP_OK              On success
 *       - ESP_ERR_NOT_FOUND   Playlist is empty
 */
esp_err_t esp_gmf_playlist_next(esp_gmf_playlist_handle_t handle, const char **uri);

/**
 * @brief  Go back to the previous track and return its URI
 *
 * @param[in]   handle  Playlist handle
 * @param[out]  uri     Pointer to receive the URI string (do NOT free)
 *
 * @return
 *       - ESP_OK              On success
 *       - ESP_ERR_NOT_FOUND   Playlist is empty
 */
esp_err_t esp_gmf_playlist_prev(esp_gmf_playlist_handle_t handle, const char **uri);

/**
 * @brief  Get the total number of tracks in the playlist
 *
 * @param[in]  handle  Playlist handle
 *
 * @return  Number of tracks, or 0 if handle is invalid
 */
int esp_gmf_playlist_get_count(esp_gmf_playlist_handle_t handle);

/**
 * @brief  Get the current track index (0-based)
 *
 * @param[in]  handle  Playlist handle
 *
 * @return  Current index, or -1 if playlist is empty
 */
int esp_gmf_playlist_get_current_index(esp_gmf_playlist_handle_t handle);

/**
 * @brief  Check if the playlist is empty
 *
 * @param[in]  handle  Playlist handle
 *
 * @return  true if empty or handle is invalid
 */
bool esp_gmf_playlist_is_empty(esp_gmf_playlist_handle_t handle);

#ifdef __cplusplus
}
#endif
