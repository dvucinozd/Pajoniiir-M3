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

## Physical acceptance

The operator first confirmed solo D1 as **sharp and fluid**. After installing
the scanout-order correction, the operator ran both decks and confirmed that
everything was correct: both waveforms were sharp and fluid, with no display
flash or audible consequence.

Result: **focused solo and dual-deck waveform synchronization PASS**.

Still open: the longer combined integration soak and the separately tracked
FLX4 simultaneous-enumeration recovery monitoring.
