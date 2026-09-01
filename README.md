# Pajoniiir-M3

Pajoniiir-M3 is a standalone, dual-deck DJ system built around a single
ESP32-P4. It connects directly to a Pioneer DDJ-FLX4, a Rekordbox USB drive, a
5-inch touch display, a PCM5102A master DAC, and an ESP32-C6 Wi-Fi coprocessor.
No computer or secondary control MCU is required during normal use.

## Current status

The current clean, signed, and hardware-accepted release is
`M3-51-gafb2099`, running from `ota_0`.

| Item | Status |
| --- | --- |
| DDJ-FLX4 | MIDI In/Out and UAC1 headphone audio working |
| Rekordbox media | USB3 library, browsing, and dual-deck loading working |
| Master output | PCM5102A stereo output working at 44.1 and 48 kHz |
| Display | 800×480 native landscape DSI output accepted |
| Touch | FT5426 working across the full screen |
| Wi-Fi | `Pajoniiir-M3` SoftAP, web control, and signed OTA working |
| Playback | Dual-deck, loops, pitch, Master Tempo, Censor, and Beat FX accepted |
| Waveforms | Sharp and fluid on all five zoom levels |

Release artifact identity:

- firmware image: 2,371,008 bytes
- firmware SHA-256:
  `7FB9C78E4918117E60848B1BF4D34277412B722341DACD41DEAEAF2E94C815B7`
- signed `.ddjota` bundle: 2,371,196 bytes
- bundle SHA-256:
  `C2658E3C55647745DB8142D691E8A9EBF5A27FA584DED9469B6AA36335BEEBC7`

See the
[M3-51 release acceptance record](docs/validation/2026-09-01-m3-51-clean-release.md)
for the full build, signing, flashing, OTA, and physical smoke evidence.

## How the system is connected

| Connection | Purpose |
| --- | --- |
| USB1 / CH340C | 5 V board power, flashing, and serial diagnostics |
| USB2 / FS Host | Pioneer DDJ-FLX4 MIDI and UAC1 headphone audio |
| USB3 / HS Host | Rekordbox USB mass-storage media |
| GPIO1 / GPIO2 / GPIO3 | PCM5102A BCK / LCK / DIN master-audio signals |
| MIPI-DSI J2 | EYOYO DSI506 / DYL0023 display and FT5426 touch |
| ESP32-C6 over SDIO | Wi-Fi 6 through ESP-Hosted |

The ESP32-P4 is the single source of truth for playback, deck and mixer state,
USB host clients, MIDI mapping, LED feedback, audio DSP, the music library,
and the UI.

### PCM5102A wiring

| PCM5102A | ESP32-P4 board |
| --- | --- |
| BCK | GPIO1 |
| LCK | GPIO2 |
| DIN | GPIO3 |
| SCK | GND |
| GND | GND |
| VIN | 5 V |

Required module straps are `H1=L`, `H2=L`, `H3=H`, and `H4=L`. Leaving these
straps open produced loud modulated noise during bring-up.

For the complete and authoritative wiring reference, read
[Hardware Wiring](docs/HARDWARE_WIRING.md) before reconnecting hardware.

## Using the device

1. Connect board power through USB1 / CH340C.
2. Connect the DDJ-FLX4 to USB2 and the Rekordbox drive to USB3.
3. Wait for the Overview screen and controller LEDs to settle.
4. Load tracks from the touch Library, the FLX4 browser controls, or the web
   controller.
5. For web control, connect to the configured `Pajoniiir-M3` Wi-Fi network and
   open `http://192.168.4.1/`.

Wi-Fi is intentionally left enabled during the current development phase so
tracks can be loaded without a computer-side DJ application. Service Wi-Fi
credentials and OTA URLs are stored in NVS and are not hard-coded in source.

## Developer quick start

### Requirements

- Windows PowerShell
- ESP-IDF **v6.0.2**
- the ESP-IDF installation at `C:\Espressif\v6.0.2\esp-idf`
- MSYS2 UCRT64 GCC at `C:\msys64\ucrt64\bin` for host tests

Do not build with a different ESP-IDF version. Component manifests pin
`idf: "==6.0.2"`, and CI uses the same version.

### Build the firmware

From the repository root:

```powershell
. C:\Espressif\tools\Microsoft.v6.0.2.PowerShell_profile.ps1
idf.py --version
Set-Location firmware\main-deck-p4
idf.py build
```

`idf.py --version` must report `ESP-IDF v6.0.2`.

### Run host tests

From the repository root:

```powershell
$env:Path = "$env:Path;C:\msys64\ucrt64\bin"
.\tests\run_p4_host_tests.ps1
```

Additional deterministic gates:

```powershell
.\tests\audio_keylock_soak\run_audio_keylock_soak.ps1
.\tests\ui_simulator\run_ui_simulator_e2e.ps1
```

The UI simulator uses pinned LVGL sources and performs exact 800×480 screenshot
checks. Update screenshot baselines only after visually reviewing the rendered
artifacts.

### Flash a development board

After building from `firmware\main-deck-p4`:

```powershell
idf.py -p COM6 flash monitor
```

Replace `COM6` with the port assigned to the board. Exit the serial monitor
with `Ctrl+]`.

### Create a signed OTA package

After a clean build:

```powershell
.\tools\package_ota_release.ps1 -BuildName build
```

The private signing key is intentionally not stored in the repository. The
firmware contains only the trusted public key for release key ID `rel-001`.
Read [P4 OTA Update](docs/OTA-UPDATE.md) before publishing or installing a
bundle.

## Repository layout

```text
firmware/main-deck-p4/   ESP-IDF application and components
firmware/common/         shared OTA and firmware-health modules
docs/                    active documentation and validation records
tests/                   host regressions, DSP soak, and UI simulator
tools/                   OTA packaging and development utilities
```

The authoritative DDJ-FLX4 MIDI source is
[`docs/reference/Pioneer-DDJ-FLX4.midi.xml`](docs/reference/Pioneer-DDJ-FLX4.midi.xml).

Generated ESP-IDF build directories, `managed_components`, local `sdkconfig`
files, and release output are not committed. The pinned
`firmware/main-deck-p4/dependencies.lock` file is committed and must remain
unchanged after a reproducible build.

## Known monitoring items

The release is accepted, but two low-frequency findings remain under
observation:

- when FLX4 and USB storage enumerate at the same time, the first FLX4 claim
  can fail and then recover automatically on a new USB address;
- isolated `output-late` events have occurred without audible or visual
  consequences and without PCM or UAC data loss.

No reliability campaign is running now. The repeatable boot/reconnect matrix,
long worst-case playback soak, telemetry fields, and PASS/FAIL rules are
documented for later use in the
[Reliability Monitoring Plan](docs/RELIABILITY_MONITORING_PLAN.md).

## Documentation

- [Documentation index](docs/README.md)
- [Project overview](docs/PROJECT_OVERVIEW.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Hardware wiring](docs/HARDWARE_WIRING.md)
- [DSI506 display bring-up](docs/DISPLAY_DSI506_BRINGUP.md)
- [DDJ-FLX4 MIDI map](docs/DDJ_FLX4_MIDI_MAP.md)
- [Development plan](docs/DEVELOPMENT_PLAN.md)
- [Startup checklist](docs/STARTUP_CHECKLIST.md)
- [Risk register](docs/RISK_REGISTER.md)
- [P4 OTA update](docs/OTA-UPDATE.md)
- [Documentation status](docs/DOCUMENTATION_STATUS.md)

Files under `docs/validation`, `docs/superpowers/specs`, and `docs/migration`
are dated evidence or historical design records. Use the active documents
listed above for current hardware, architecture, and development decisions.
