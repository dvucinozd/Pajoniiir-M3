# FLX4 Official MIDI Gap-Closure Smoke

Document status: historical acceptance worksheet, reviewed 2026-08-24. Do not
convert a `PENDING` row to pass without new physical evidence and firmware ID.

Date: 2026-07-03
Feature: official DDJ-FLX4 MIDI gap closure (merged to `master`, merge `ccafd13a`).
Boards: P4 on COM15, S3 on COM3, DDJ-FLX4 on the S3 USB host.
Build: `flx4_hp_e2e` product profile on both boards.

Closes the seven gaps found by comparing the official Pioneer
`DDJ-FLX4_MIDI_message_List` against the firmware. The S3 maps the new physical
MIDI notes to semantic `control_link` events; the P4 owns the resulting state,
audio, and LED behavior. Host suites (P4 + S3) pass; the checks below are the
hardware smoke, to be filled after flashing the new builds.

## Controls to verify

| Control | Expected behavior | Result |
| --- | --- | --- |
| Censor (hold) | Playback reads retained canonical PCM backward while the forward playhead advances; release returns to the slip position through a 10-ms crossfade without seek. | PASS — D1 48-kHz and D2 44,1-kHz physical reverse/release on `M3-41-g133f399`, with zero controlled output-late/PCM/UAC/service-log deltas (2026-08-24) |
| Sync (set master) | Long/again designates the deck as sync master; the other deck's Sync matches this deck's BPM. | PENDING |
| Quantize toggle | Toggles quantize; subsequent loop set / loop-adjust snaps to the nearest beat. | PENDING |
| Loop Adjust In | Moves the active loop in-point to the (quantized) play position without dropping the loop. | PENDING |
| Loop Adjust Out | Moves the active loop out-point to the (quantized) play position. | PENDING |
| Reloop + Stop | Clears/forgets the active loop and its shadow state. | PENDING |
| Headphone Level knob | Scales the P4 headphone/monitor output gain (USB-headphones cue level). | PENDING |
| Shift + Browse rotate | Coarse browse (library x10 / overview zoom x4). | PENDING |
| Shift + Browse press | Shows/toggles the library view. | PENDING |
| Shift + Load Deck 1/2 | Loads the selected track to the shifted deck (same load path). | PENDING |
| Shift + Beat FX beat -/+ | Halves / doubles the Beat FX beat size in one step. | PENDING |

## Cross-checks

| Check | Result |
| --- | --- |
| FLX4 MIDI stays responsive during dual-deck playback | PENDING |
| USB headphones cue + PCM5102A RCA MAIN still simultaneous | PENDING |
| No S3 / P4 reboot during the pass | PENDING |
| Censor LED reflects P4 `censor_active` via the LED snapshot | PASS — physical LED path verified 2026-07-07; `M3-41-g133f399` status monitor captured `false→true→false` on both deck states 2026-08-24 |

## Result

PARTIAL — original build flashed 2026-07-03. Censor control/state/audio and LED
rows are closed by the evidence above; remaining `PENDING` rows still require
their own physical acceptance.
