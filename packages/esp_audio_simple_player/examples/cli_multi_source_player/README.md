# CLI Multi-Source Player Example

This example uses `esp_audio_simple_player` (ESP-GMF based) and console commands to control multiple music sources:

- HTTP music
- SDCard local music
- Embedded flash prompts (`embed://tone/...`)

Supported commands:

- `play [http|sdcard] [index]`
- `pause`
- `resume`
- `stop`
- `prev`
- `next`
- `flash <interrupt|resume> [tone_idx]`

Flash playback supports two modes:

> Note: To keep this PR free of binary files, real prompt MP3 assets are not committed.
> `main/esp_embed_tone.h` currently uses placeholder payloads for control-flow demo only.
> Replace them with valid embedded audio data for real prompt playback.


1. `interrupt`: stop current playback immediately and play prompt tone.
2. `resume`: interrupt current playback, then restore previous source/track after prompt finishes.
