# CLI Simple Player Example

This example uses `esp_audio_simple_player` to play audio from multiple media sources (HTTP, SD card, embedded flash) and provides CLI console commands for playback control.

## Features

- Play / Stop / Pause / Resume
- Next track / Previous track
- Volume up / down
- Mute / Unmute toggle
- Auto-advance to next track on completion

## Supported Media Sources

| Scheme | Description | Example URI |
|--------|-------------|-------------|
| `file://` | SD card | `file://sdcard/test.mp3` |
| `embed://` | Embedded flash | `embed://tone/0_test.mp3` |
| `https://` | HTTP stream | `https://dl.espressif.com/dl/audio/gs-16b-2c-44100hz.mp3` |

## CLI Commands

| Command | Description |
|---------|-------------|
| `play` | Start or restart the current track |
| `stop` | Stop playback |
| `pause` | Pause playback |
| `resume` | Resume playback |
| `next` | Skip to the next track |
| `prev` | Go back to the previous track |
| `vol+` | Increase volume by 10 |
| `vol-` | Decrease volume by 10 |
| `mute` | Toggle mute on/off |
| `status` | Display player state, current track, and volume |

## Playlist Component

This example includes a temporary lightweight playlist component (`esp_gmf_playlist`) in the `components/` directory. Its public API is designed to remain stable so it can be replaced by the official playlist component once available.

## How to Use

### Hardware Required

- An ESP32 or ESP32-S3 development board with an audio codec (e.g., LyraT Mini, ESP32-S3-Korvo-2)
- An SD card with audio files (e.g., `test.mp3`)
- Wi-Fi network access (for HTTP streaming)

### Configure

```bash
idf.py menuconfig
```

Set your Wi-Fi SSID and password under **Example Connection Configuration**.

### Build and Flash

```bash
idf.py -p PORT flash monitor
```

### Interact

Once booted, type `help` at the `player>` prompt to see available commands.
