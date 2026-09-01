# Combined integration soak — 2026-09-01

## Scope

This is a ten-minute physical combined-load gate for the current app-only
Pajoniiir-M3 bench candidate. It combines display, touch, master and headphone
audio, dual-deck playback, USB2 FLX4, USB3 library access and Wi-Fi API traffic.
It is not a zero-late claim and does not replace an extended cold-power or
disconnect/reconnect soak.

## Bench identity

- firmware: `M3-47-g3f23bd2-dirty`;
- running slot: `factory` at `0x20000`;
- image size: 2,370,224 bytes (`0x242ab0`);
- image SHA-256:
  `EFDEFAF4269F635D4A44B5590D54D3DE6A4CD53F34906080B1780F0867571EBD`;
- controller: Pioneer DDJ-FLX4, MIDI In/Out and UAC present;
- USB3 library: 191 tracks;
- Wi-Fi: Windows connected to `Pajoniiir-M3`, 2.4 GHz channel 6, initial RSSI
  `-43 dBm`;
- audio sources: D1 and D2 both 44.1 kHz;
- D1 loop: `2687..5358 ms`;
- D2 loop: `125138..130918 ms`.

Both decks and both loops were API-confirmed before the measured interval.

## Load and operator actions

During the complete 600-second window:

- both deck decode/playback paths remained active;
- PCM5102A carried the master output;
- FLX4 UAC carried headphone audio with both channel CUE/PFL paths exercised;
- Overview showed both refresh-synchronised moving waveforms;
- the operator was asked to visit Overview, Library, Hot Cues and Settings,
  adjust and restore Backlight, then return to Overview; the final operator
  confirmation covered responsive touch and a stable display;
- the USB3 library endpoint was checked every 40 status polls;
- the firmware endpoint was checked every 120 status polls;
- `/api/status` was polled every 250 ms to keep sustained Wi-Fi/web load.

An earlier warm-up interval was intentionally excluded because D2 reached EOF.
The measured interval began only after both loops were active.

## Measured result

| Metric | Result |
|---|---:|
| elapsed | 600 s |
| status polls | 1840 |
| status API errors | 0 |
| library checks / errors | 46 / 0 |
| firmware checks / errors | 15 / 0 |
| polls without both decks and loops active | 0 |
| PCM underrun delta D1 / D2 | 0 / 0 |
| UAC dropped / overflow / active-underflow delta | 0 / 0 / 0 |
| service-log dropped delta | 0 |
| output-late delta | 5 |
| output-late maximum | 12522 us |
| output-late threshold | 11610 us |
| minimum total heap free | 24,922,632 bytes |
| minimum internal heap free | 101,423 bytes |
| final UAC ring | `nominal`, 1351 / 2048 frames |

The five deadline misses had no PCM or UAC data-loss consequence. The operator
explicitly reported clean sound after the first miss and again while the count
increased; the final confirmation covered clean master and headphone output,
fluid waveforms, responsive touch, stable display and a functioning FLX4.

## Verdict

**PASS — ten-minute combined functional integration gate.**

The output-late count is retained as a monitoring finding, not reclassified as
zero-late. Extended cold-power/reconnect testing and the remaining complete UI
eyes-on/screenshot gate stay open.
