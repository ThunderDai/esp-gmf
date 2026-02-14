# CLI 多音源播放器示例

该示例基于 ESP-GMF 的 `esp_audio_simple_player`，通过控制台命令实现多音源播放控制：

- HTTP 音乐
- SDCard 本地音乐
- 嵌入式 Flash 提示音（`embed://tone/...`）

支持命令：

- `play [http|sdcard] [index]`
- `pause`
- `resume`
- `stop`
- `prev`
- `next`
- `flash <interrupt|resume> [tone_idx]`

其中 Flash 播放有两种模式：

> 注意：为避免 PR 包含二进制文件，本示例仓库中未提交真实提示音 MP3。
> 当前 `main/esp_embed_tone.h` 使用占位数据，仅用于演示控制流程。
> 如需实际播放提示音，请将其替换为有效音频二进制嵌入数据。


1. `interrupt`：直接打断当前播放并播放提示音。
2. `resume`：打断当前播放，提示音播完后自动恢复之前的音乐源与曲目。

## 使用说明

1. 准备 SDCard 测试音频：
   - `/sdcard/test.mp3`
   - `/sdcard/test.aac`
2. 在 menuconfig 中配置 Wi-Fi（用于 HTTP 音源）。
3. 烧录运行后，在串口控制台执行命令。

## 命令示例

```bash
play http 0
next
pause
resume
flash interrupt 0
flash resume 1
play sdcard 0
prev
stop
```
