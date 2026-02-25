# AGENTS.md

## Cursor Cloud specific instructions

### Project overview

ESP-GMF (Espressif General Multimedia Framework) is a C-based embedded firmware framework for ESP32 microcontrollers. It uses **ESP-IDF v5.4.3** as its build system and cross-compilation toolchain. There is no web UI, Node.js, or database — this is purely an embedded C project.

### Environment setup

ESP-IDF is installed at `$HOME/.espressif/esp-idf` (v5.4.3). Before any build or test commands, activate the environment:

```bash
export IDF_PATH=$HOME/.espressif/esp-idf
source $IDF_PATH/export.sh
```

The `python3.12-venv` system package is required for ESP-IDF's Python virtual environment.

### Building

Individual test apps or examples are built with `idf.py`:

```bash
cd gmf_core/test_apps
idf.py set-target esp32
idf.py build
```

The CI build script can also be used (requires `PROJECT_PATH` env var):

```bash
export PROJECT_PATH=/workspace
python tools/ci/build_apps.py /workspace/gmf_core -t esp32 --target-dir-type test_apps --build-dir build_@t_@w --no-require-pytest -vv
```

Supported targets (from CI): `esp32`, `esp32s3`, `esp32p4`. Each target requires a corresponding `AUDIO_BOARD` value (e.g., `lyrat_mini_v1_1` for esp32, `esp32_s3_korvo2_v3` for esp32s3).

### Linting

```bash
pre-commit run --all-files
```

The `double-quote-string-fixer` hook may auto-modify Python files in `packages/esp_board_manager/` — this is expected behavior.

### Testing

- **Python-based tests** (no hardware required): `packages/esp_board_manager/test_apps/test_scripts/run_pytest.sh`
- **Target tests** require physical ESP32 hardware and cannot run in this cloud environment.
- The build itself (`idf.py build`) is the primary verification step for code changes to C components.

### Key gotchas

- After `source $IDF_PATH/export.sh`, `pip install` goes into the ESP-IDF Python venv at `~/.espressif/python_env/idf5.4_py3.12_env/`.
- The `build_apps.py` script requires `PROJECT_PATH` to be set to the repo root.
- Build artifacts (build/, managed_components/, dependencies.lock, sdkconfig) should be cleaned before switching targets.
