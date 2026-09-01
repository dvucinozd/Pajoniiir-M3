# DSI506 waveform refresh synchronization — 2026-09-01

## Scope

This is a focused physical display gate for the moving Overview waveforms on
the EYOYO DSI506 / DYL0023. It does not replace the long combined
display/touch/master/headphones/dual-deck/USB/Wi-Fi soak.

## Symptom and isolation

The operator reported that only the moving main-waveform lines looked as if
they were in water. Static UI, text and the green playhead remained sharp.
When playback stopped, the waveform was sharp and still. A host regression
then advanced the RGB565 waveform cache for 160 one-pixel steps, crossing more
than one edge refill and ring wrap, and proved that every overlapping column
remained bit-identical. This isolated the remaining symptom to physical update
timing rather than waveform data or cache deformation.

## Timing candidate

The accepted horizontal and packetization parameters remain unchanged:

- one DSI lane at 800 Mbps;
- RGB888 scanout;
- 27.777 MHz pixel clock;
- HFP/HSW/HBP `59/2/45`;
- burst with sync pulses, no frame ACK.

Only VFP changed from `7` to `109`, giving VFP/VSW/VBP `109/2/22` and:

`27,777,000 / (906 * 613) = 50.0146 Hz`.

The firmware UI remains driven from the panel refresh callback. The operator
confirmed the first candidate as sharp and fluid with solo D1 playback.

## Dual-deck correction

With D1 and D2 playing together, the upper waveform still showed a slight
watery effect. The scheduler was alternating D1/D2 direct-PPA order every
frame. A two-redraw tick now always writes the upper deck before the lower deck
to follow physical top-to-bottom scanout. One-redraw ticks retain alternating
fairness for paused/restore work.

The scheduler host test covers both guarantees. The waveform-cache long-scroll
regression remains part of the full P4 host runner.

## All-zoom dual-deck regression correction

The first post-release dual-deck smoke exposed a narrower residual case. With
both decks playing, the operator could still occasionally see bent/watery main
waveform lines, but only at the two closest zoom levels (4 and 8 visible
beats). The symptom affected both decks, did not reproduce with D2 playing
alone, and still reproduced with Master Tempo disabled. PCM underrun and UAC
drop/overflow counters stayed at zero during isolation. One new output-late
event was observed while both decks played, but the maximum remained 12,634 us;
that event is not treated as proof of the visual cause.

The cache data and two-redraw top-to-bottom policy remain unchanged. The frame
orchestrator now runs `ui_overview_update()` before Library and Status work,
immediately after the panel refresh callback wakes the LVGL task. Both PPA
waveform transfers therefore get the accepted 109-line vertical blanking
interval instead of starting after noncritical UI work. This preserves the
full 50.0146-Hz cadence for both decks; the previously rejected one-deck-per-
tick stagger is not restored. A static host gate pins this ordering.

## Verification and installed image

- ESP-IDF: `v6.0.2`;
- full `tests/run_p4_host_tests.ps1`: PASS;
- `idf.py build`: PASS;
- binary size: 2,370,224 bytes (`0x242ab0`);
- binary SHA-256:
  `EFDEFAF4269F635D4A44B5590D54D3DE6A4CD53F34906080B1780F0867571EBD`;
- version: `M3-47-g3f23bd2-dirty`;
- app-only flash: COM6, `factory` at `0x20000`, flash hash verified;
- boot restored the 191-track USB3 library and FLX4 MIDI/UAC. The known
  simultaneous USB enumeration collision occurred once at boot and recovered
  automatically when FLX4 was reclaimed at address 3.

Serial monitoring during the focused playback window showed no new log output:
no panic, WDT, reset, display underrun or reported audio/USB loss.

The all-zoom correction was verified separately:

- full `tests/run_p4_host_tests.ps1`: PASS, including the new frame-order gate;
- ESP-IDF `v6.0.2` incremental build: PASS;
- binary size: 2,370,992 bytes (`0x242db0`);
- binary SHA-256:
  `C5F018558BA91729A7F16DA4D3056E34E44FA7BBE2E07AA13E1558C4272E30F1`;
- source identity at build time: `M3-50-gc4897b7-dirty`;
- full flash: COM6, `factory`, flash hash verified;
- boot restored the 191-track USB3 library and automatically recovered the
  known initial FLX4/MSC enumeration collision by reclaiming FLX4 at address 3.

The first focused all-zoom run ended without a post-run HTTP snapshot. The
subsequent clean release run below added the independent API and OTA evidence.

## Clean M3-51 release confirmation

The accepted source was committed as `afb20993381ad5e9a245d83de79692a04e7a9db9`
and rebuilt from an empty `build_release` directory with ESP-IDF 6.0.2:

- version: `M3-51-gafb2099`;
- image: 2,371,008 bytes, SHA-256
  `7FB9C78E4918117E60848B1BF4D34277412B722341DACD41DEAEAF2E94C815B7`;
- signed bundle: 2,371,196 bytes, SHA-256
  `C2658E3C55647745DB8142D691E8A9EBF5A27FA584DED9469B6AA36335BEEBC7`;
- full flash: COM6 / `factory`, write hash verified;
- signed local OTA: HTTP 200, booted `ota_0`, health gate marked valid and API
  returned to `idle`;
- post-boot: 191 tracks, FLX4 MIDI In/Out/UAC active and three stable API
  snapshots with zero PCM underrun, output-late and service-log drop counters.

The operator then repeated GUI, touch/Backlight, dual-deck master/headphones
and all five zoom levels on the clean release and confirmed all items correct.

## Physical acceptance

The operator first confirmed solo D1 as **sharp and fluid**. After installing
the scanout-order correction, the operator ran both decks and confirmed that
everything was correct: both waveforms were sharp and fluid, with no display
flash or audible consequence.

After the narrower 4/8-beat regression was corrected, the operator repeated
the dual-deck test and confirmed both main waveforms as fluid at every zoom
level (4, 8, 12, 16 and 24 visible beats).

Result: **focused solo, dual-deck and all-zoom waveform synchronization PASS**.

Still open: isolated output-late events and simultaneous-enumeration recovery
remain monitoring findings; neither is attributed to this visual regression.
