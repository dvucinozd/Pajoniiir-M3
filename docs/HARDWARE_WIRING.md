# Hardware Wiring (Pajoniiir-M3)

Status: current single-chip ESP32-P4 architecture with **JC-ESP32P4-M3-DEV** board.

## Single-Chip 3-Port USB Architecture Overview

The **JC-ESP32P4-M3-DEV** board features **3 dedicated USB Type-C ports**, eliminating the need for an external USB hub:

```text
                        +-----------------------------------------------+
                        |            JC-ESP32P4-M3-DEV Board            |
                        |                                               |
                        |     [5.0" MIPI-DSI (800x480) IPS Screen]      |
                        |     [FT5426 Capacitive Touchscreen]           |
                        |                                               |
                        |            [ESP32-P4 Single Chip]             |
                        |                                               |
                        |   [USB1 (TTL)]    [USB2 (FS)]   [USB3 (HS)]   |
                        +--------+---------------+-------------+--------+
                                 |               |             |
                                 v               v             v
                          +------------+   +-----------+  +-----------+
                          |  5V Power  |   |  Pioneer  |  | Rekordbox |
                          |  Adapter   |   |  DDJ-FLX4 |  | USB Flash |
                          |  & Flashing|   | (MIDI+UAC)|  |   Drive   |
                          +------------+   +-----------+  +-----------+
```

## USB Port Assignments & Topology

| Port on Board | Hardware Chip / PHY | Speed / Mode | Assigned Function |
| :--- | :--- | :--- | :--- |
| **USB1 (USB-TTL)** | CH340C UART (`GPIO37/38`) | 5V Power / Serial | **Board Power Supply (5V IN), Firmware Flashing & Serial Logs** |
| **USB2 (Full-Speed)** | P4 FS USB PHY (`USB1P1_P/N`) | Full-Speed (12 Mbps) | **Pioneer DDJ-FLX4 (USB-MIDI In/Out + UAC1 Headphone Audio)** |
| **USB3 (High-Speed)** | P4 HS USB PHY (`ESP_USB_P/N`) | High-Speed (480 Mbps)| **Rekordbox USB Media Drive (High-Speed MSC Track Loading)** |

### Power Supply & VBUS Stability
- **No external USB Hub required**: The DDJ-FLX4 and the USB flash drive plug directly into `USB2` and `USB3`.
- **Power Isolation & Stability**: `USB1` receives clean 5V power from a dedicated USB power supply, powering the internal buck converter (`TLV62569` for `3.3V`) and feeding the common `USB5V_IN` rail, preventing brownouts.

## Master Audio Output (PCM5102A I2S DAC)

Master audio output uses an external PCM5102A I2S DAC breakout connected to the P4 header:

| Signal | ESP32-P4 Pin | PCM5102A Pin | Notes |
| :--- | :--- | :--- | :--- |
| **I2S BCLK** | GPIO50 | BCK | Bit Clock |
| **I2S WS / LRCK** | GPIO52 | LCK | Word Select / Frame Clock |
| **I2S DOUT** | GPIO51 | DIN | Serial Audio Data |
| **Power (VCC)** | 3.3V / 5V | VCC | Dependent on DAC module |
| **Ground (GND)**| GND | GND | Common ground |

## Headphone / Cue Audio Output

- Headphone/cue audio is streamed directly from ESP32-P4 to the DDJ-FLX4 via USB Audio Class 1 (UAC1 Isochronous endpoint) over the USB connection.
- Headphones plug directly into the 3.5 mm jack on the front of the Pioneer DDJ-FLX4.

## Display & Touchscreen (JC-ESP32P4-M3-DEV)

The 5.0" MIPI-DSI IPS screen (800×480) and FT5426 capacitive touch controller connect directly via the onboard MIPI-DSI / I2C FPC connector:

| Signal / Peripheral | Hardware / Pin | Description |
| :--- | :--- | :--- |
| **Display Panel** | MIPI-DSI 2-lane | 800×480 @ 30 MHz DPI video mode, 0° PPA |
| **Touch I2C SDA** | GPIO7 | FT5426 I2C Data (`0x38`) |
| **Touch I2C SCL** | GPIO8 | FT5426 I2C Clock |
| **Touch Reset** | GPIO22 | Active-low reset |
| **Touch Interrupt** | GPIO23 | Active-low interrupt |
| **Backlight PWM** | GPIO26 | LCD Backlight control |

The XIAO ESP32S3 migration set GPIO7/GPIO8/GPIO9 avoids the control UART
GPIO5/GPIO6 and UART0 GPIO43/GPIO44. The retired CDJ panel firmware no longer
claims these pins; the product S3 firmware always owns USB OTG as the FLX4 host.

**Transport details (validated):** P4 `monitor_pcm_link` is an I2S TX master
that streams `P4HP` framed blocks (stereo 16-bit monitor PCM, sequence numbers,
CRC32 over protected header plus payload). S3 `p4_audio_link` is an I2S slave
RX that deframes into a 4096-frame ring. The pipe runs at
**64 kHz 16-bit stereo slots (2.048 MHz BCLK)** -- 96 kHz
slots corrupted over the jumper harness. The TX task writes at line rate (real
blocks or explicit zero filler) so the continuously-transmitting DMA never laps
the writer mid-block. 2026-07-06 XIAO bench confirmed raw I2S reception and
deframing on GPIO7/GPIO8/GPIO9 with zero steady-state deltas for `gaps`, `crc`,
I2S `timeouts`, I2S `errors`, `underruns`, and `overruns` during a five-minute
S3-only soak. Repeat this soak after I2S, task-priority, or FLX4 USB Audio
scheduling changes.

**Product e2e result (validated 2026-07-07; rate-match re-smoke 2026-07-09):** with the XIAO GPIO7/GPIO8/GPIO9
link wiring, P4 `build_flx4_hp_e2e_tcmguard`, and S3
`build_flx4_hp_e2e_xiao`, the full path from P4 playback to the FLX4 headphone
jack was confirmed audible by the operator. P4 `MONITOR_PCM_LINK` counters rose
with `dropped=0`; S3 `P4_AUDIO_LINK` counters rose with `gaps=0` and `crc=0`
before the FLX4 USB Audio consumer was attached. A 2026-07-09 S3 product flash
fixed the remaining intermittent product-path overruns by keeping the FLX4 USB
Audio endpoint rate and packetizer synchronized to the active P4 link rate while
the ring stream is already running; the follow-up COM6 log held `overruns=0`
with `FLX4_USB_AUDIO skipped=0 underrun=0`. See
`docs/validation/FLX4_USB_AUDIO_E2E_SMOKE.md`.

**Product I2S unit budget (P4 rev v1.3 / eco2):** I2S unit 2 freezes on
`i2s_new_channel`, leaving units 0 and 1. Product config: **monitor link on
unit 0** (`CONFIG_MONITOR_PCM_LINK_I2S_UNIT=0`), **PCM5102A RCA MAIN on unit 1**,
and **ES8311 onboard monitor disabled** (`CONFIG_BSP_ES8311_MONITOR=n`) to free
unit 0. The FLX4 USB headphones are the CUE/MONITOR output; the local ES8311
monitor is dropped. This audio config is now the default (folded into each
board's `sdkconfig.defaults` on 2026-07-10), so a plain `idf.py build` has sound.

PCM5102A MAIN OUT candidate pins for the photographed PCM5102MK/PCM5102A
breakout board. The board header silkscreen is:

```text
VCC
GND
GND
LRCK
DATA
BCK
```

`DATA` on this module is the DAC serial data input and must be driven by the
P4 I2S DOUT signal. `LRCK` is the same signal as I2S `WS`.

| PCM board header | Signal meaning | ESP32-P4 JC4880 JP1 candidate |
| --- | --- | --- |
| VCC | DAC board power | 3.3 V first; use 5 V only if this exact module requires it |
| GND | ground | GND |
| GND | ground | GND, optional second return |
| LRCK | I2S word select / left-right clock | GPIO52 / JP1 pin 5 |
| DATA | I2S serial data into DAC | GPIO51 / JP1 pin 7 |
| BCK | I2S bit clock | GPIO50 / JP1 pin 9 |

No MCLK/SCK pin is exposed on this module, so firmware keeps PCM5102A MCLK as
`I2S_GPIO_UNUSED` for first bring-up.

Runtime notes after hardware bring-up:

- PCM5102A uses the P4 I2S1 channel as MAIN OUT when
  `CONFIG_BSP_PCM5102A_MAIN_OUT=y` is enabled in the local P4 build config.
- ES8311/onboard monitor output is disabled in the current FLX4 USB headphones
  product profile; the P4-to-S3 monitor PCM link owns the CUE/MONITOR path.
- The audio engine must reconfigure the PCM5102A I2S clock to the current track
  sample rate when opening the shared output service. Leaving PCM5102A at its
  44.1 kHz BSP default while playing a 48 kHz track causes slow/popping audio
  and output-late diagnostics.
- Mixed sample-rate dual-deck playback is supported in the audio mixer path:
  each deck's resampler applies `source_sample_rate / output_sample_rate` on
  top of pitch, so a 48 kHz track can play correctly while the shared output is
  clocked at 44.1 kHz.
- 2026-06-27 COM15 hardware measurement with both decks playing reported
  stable decode timing, full PCM rings, and `late=0 late_max=0 us` after the
  PCM5102A sample-rate reconfiguration fix.
- The PCM5102A board's RCA and 3.5 mm connectors were hardware-smoked on
  2026-06-30 and both produced audio. Treat both as DAC board outputs for MAIN
  OUT validation; for final level/noise judgment prefer RCA or 3.5 mm into an
  active AUX/LINE IN, mixer, amplifier, or audio interface input.

PCM5102A line-out acceptance result:

1. 2026-06-30 boot probe confirmed `PCM5102A main out ready: BCLK=50 WS=52
   DOUT=51`, USB library load, and FLX4 reconnect:
   `logs/p4_pcm5102a_boot_probe_20260630_123558.log`.
2. 2026-06-30 RCA smoke confirmed playback through the PCM5102A board's RCA
   output and onboard 3.5 mm output. The capture showed
   `PCM5102A main out open @ 44100 Hz`, `late=0`, and no limiter activity for
   the Deck 1 test window:
   `logs/p4_pcm5102a_rca_smoke_20260630_123632.log`.
3. Remaining audio acceptance work is gain staging, dual-deck summed level, and
   limiter behavior; not basic PCM5102A wiring or I2S bring-up.

Rejected DAC pin proposal:

- GPIO22/GPIO23/GPIO24/GPIO25 must not be used for this DAC plan.
- GPIO23 is already LCD backlight PWM.

These must be verified against the JC4880 schematic, board examples, and actual
ESP-IDF I2S peripheral routing before committing PCB or harness work.

## Bench Bring-Up Order

1. Power S3 and P4 independently and confirm shared ground.
2. Verify S3/P4 UART heartbeat with no FLX4 connected.
3. Connect FLX4 to S3 and capture raw USB MIDI input.
4. Forward only Play/Cue/Load events to P4.
5. Add tempo/fader/crossfader events after raw ranges are confirmed.
6. Add LED feedback after P4 state transitions are stable.
7. Add external DAC wiring only after dual-deck software mixer can produce test
   buffers on a single known-good output.
