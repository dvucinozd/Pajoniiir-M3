# Pajoniiir-M3

Standalone dual-deck DJ system built around a Pioneer DDJ-FLX4 and the **JC-ESP32P4-M3-DEV** board with a **5.0" MIPI-DSI (800×480) IPS touchscreen**. It reads Rekordbox media directly and does not require a PC during performance.

Canonical repository: `https://github.com/dvucinozd/Pajoniiir-M3.git`.

## System at a Glance

| Device / Module | Responsibility |
| --- | --- |
| **Pioneer DDJ-FLX4** | Operator surface: transport, jogs, tempo, mixer, pads, cue and LEDs |
| **ESP32-P4 (JC-ESP32P4-M3-DEV)** | Single-chip host: native USB MIDI & Audio host, authoritative playback/deck state, Rekordbox library, 800×480 LVGL UI, audio DSP/mixer and MAIN/cue routing |
| **5.0" MIPI-DSI IPS + FT5426** | 800×480 WVGA landscape touchscreen interface |
| **PCM5102A DAC** | Master audio output (I2S) |

The ESP32-P4 natively utilizes the board's 3 dedicated USB Type-C ports:
- **USB1 (TTL)**: 5V power input, flashing, and serial monitor.
- **USB2 (FS)**: Pioneer DDJ-FLX4 (USB MIDI + UAC1 Headphone audio).
- **USB3 (HS)**: Rekordbox USB Flash Media (@ 480 Mbps High Speed).

## Current Capabilities

- **Single-chip ESP32-P4 Architecture**: 3 onboard USB-C ports eliminate external USB hubs while providing isolated 5V power and dedicated high-speed media and low-latency controller connectivity.
- **5.0" MIPI-DSI Touch Display**: Native 800×480 WVGA display @ 30 MHz DPI video mode with FocalTech FT5426 capacitive touch and 0° PPA hardware blitting.
- **Two independent decks**: Rekordbox library browsing, MP3, WAV and FLAC playback with bounded LRU page cache.
- **FLX4 Control**: Transport, jog/vinyl scratch, tempo, Master Tempo, mixer/EQ, headphone cue, hot cues, loops, beat jump/sync, Pad FX and Beat FX control.
- **Master Audio Output**: High-quality PCM5102A I2S DAC stereo output.
- **LVGL UI**: Overview, Library (paginated 8-row table), Hot Cues and Settings tabs.

## Repository Layout

```text
controllers/                 Compiled and source controller profiles
firmware/
  main-deck-p4/              ESP32-P4 playback/audio/UI/USB Host firmware
docs/                        Product, protocol, validation and design records
tests/                       PC-side regression tests
tools/                       Profile compiler, OTA packager and support tools
```

## Build and Test

Required baseline: **ESP-IDF v6.0.2** and its matching Espressif Python and
toolchain environment. Host tests additionally require native GCC/Make and
PowerShell 5.1 (ili noviji) na Windowsima, odnosno standardni shell na Linuxu.

A standard ESP-IDF installation can be initialized on Windows with:

```powershell
$env:IDF_PATH = "C:\Espressif\frameworks\esp-idf-v6.0.2"
. "$env:IDF_PATH\export.ps1"
```

Verify the selected environment before configuring either target:

```powershell
idf.py --version
```

It must report `ESP-IDF v6.0.2`. For the first build after switching from IDF
5.5.4, remove the previous generated configuration and managed components:

```powershell
Remove-Item -Recurse -Force build, managed_components -ErrorAction SilentlyContinue
Remove-Item sdkconfig, sdkconfig.old -ErrorAction SilentlyContinue
```

Build each target from the repository root:

```powershell
cd firmware\control-board-s3
idf.py set-target esp32s3
idf.py build

cd ..\main-deck-p4
idf.py set-target esp32p4
idf.py build
```

Run the host regression suites from the repository root. These are the same two
entry points CI uses, and both run under Windows PowerShell 5.1 and PowerShell 7:

```powershell
.\tests\run_s3_host_tests.ps1
.\tests\run_p4_host_tests.ps1
```

If `gcc` is not already on `PATH`, append msys2 rather than prepending it —
prepending shadows the system `python.exe` with msys2's, which cannot run the
OTA signing suite:

```powershell
$env:Path = "$env:Path;C:\msys64\ucrt64\bin"
```

Run the headless LVGL navigation and exact-framebuffer screenshot gate:

```powershell
.\tests\ui_simulator\run_ui_simulator_e2e.ps1
```

The first run fetches the pinned LVGL source into the ignored `.cache`
directory. The gate covers Overview Deck 1/2 selection, Library, Hot Cues,
Settings, the screensaver and exact Settings restoration. See
[`tests/ui_simulator/README.md`](tests/ui_simulator/README.md) for baseline
review and update instructions. This PC gate does not replace P4 display,
touch or waveform-motion hardware acceptance.

Both default firmware configurations include the FLX4 USB-headphone path.
Build, flashing, signed release packaging and rollback procedures are covered
by [OTA Update](docs/OTA-UPDATE.md). Hardware bring-up and recurring acceptance
checks are in the [Startup Checklist](docs/STARTUP_CHECKLIST.md).

## Documentation

Start with the [complete documentation index](docs/README.md). The primary
operational documents are:

| Topic | Document |
| --- | --- |
| Product status and source-of-truth policy | [Documentation Status](docs/DOCUMENTATION_STATUS.md) |
| Product shape and implemented scope | [Project Overview](docs/PROJECT_OVERVIEW.md) |
| P4/S3 responsibilities and data flow | [Architecture](docs/ARCHITECTURE.md) |
| FLX4 inputs, outputs and acceptance ledger | [DDJ-FLX4 MIDI Map](docs/DDJ_FLX4_MIDI_MAP.md) |
| UART events and bulk/status transport | [Control Link Protocol](docs/CONTROL_LINK_PROTOCOL.md) |
| Wiring, USB and audio connections | [Hardware Wiring](docs/HARDWARE_WIRING.md) |
| Current phases and remaining engineering work | [Development Plan](docs/DEVELOPMENT_PLAN.md) |
| Open and accepted risks | [Risk Register](docs/RISK_REGISTER.md) |

Controller-profile schema/update guides, OTA records, validation evidence,
historical design decisions and upstream/vendor references are linked from the
documentation index. Dated design records explain intent; they do not override
current firmware or active operational documents.
