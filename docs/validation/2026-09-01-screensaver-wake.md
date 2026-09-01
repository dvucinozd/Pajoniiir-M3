# Screensaver Wake Validation — 2026-09-01

## Result

PASS for the focused local-controller and touch wake gate on the EYOYO
`DSI506 / DYL0023` display.

Bench image:

- version: `M3-47-g3f23bd2-dirty`;
- running slot: `factory` (app-only flash at `0x20000`);
- image size: 2,370,224 B;
- SHA-256:
  `B20D36AF3715E8E2BD42EAB513FA505A39916B6BCC1E37E8C84C04C1D7519B28`;
- signed rollback baseline remains `M3-41-g133f399` in `ota_0`.

## Reproduced failure

The first physical D1 PLAY press while the screensaver was visible dismissed
the screensaver but also started the loaded track. The API showed D1 playing
and advancing even though the operator expected a wake-only press.

The direct FLX4 path was:

```text
p4_flx4_host -> control_link_inject_semantic() -> control_link event queue
```

It bypassed `deck_core_queue_event()`, which was the only path invoking
`ui_activity_notice()`. The earlier assumption that every FLX4 event crossed
that choke point was therefore false.

## Fix and automated gates

`control_link` now owns an activity callback for direct local FLX4 semantic
events. A valid non-state event that wakes the screensaver is consumed before
it enters the control queue. Connection-state events bypass the callback, and
invalid events do not count as activity. `app_main` wires the same
`ui_activity_notice()` callback into both `deck_core` and `control_link`.

Added host coverage verifies:

- wake-active local button is consumed and the queue remains empty;
- the same button is queued while the UI is awake;
- connection state bypasses wake consumption;
- invalid input does not register activity;
- product wiring registers both activity callbacks.

Validation completed before flashing:

- `tests/run_p4_host_tests.ps1`: PASS, including the new `control_link` suite
  and both static wiring gates;
- ESP-IDF version: `ESP-IDF v6.0.2`;
- `idf.py build`: PASS;
- `idf.py -p COM6 app-flash`: PASS, esptool flash hash verified.

## Physical acceptance

1. After boot, `The Traveller.mp3` was loaded on D1 and left in `READY` at
   position 0; D2 was stopped.
2. After the two-minute idle timeout, the first physical D1 PLAY press only
   dismissed the screensaver. Immediate API evidence remained D1 `READY`,
   `playing=false`, position 0 ms.
3. A second D1 PLAY press while the UI was awake started the track. Immediate
   API evidence showed D1 `PLAYING`, `playing=true` and an advancing position.
4. D1 was stopped through the web control API and returned to `READY`.
5. After the next idle timeout, one touch in the middle of the screensaver
   restored the previous screen without activating the control underneath.

Across the focused test, output-late, PCM underrun and UAC drop/overflow
counters remained zero and `data_loss=false`. The FLX4 remained present.

## Remaining scope

This result closes the screensaver wake-consumption gate. Explicit corner,
multitouch and repeated-edge touch checks plus the combined
display/touch/master/headphones/dual-deck/Wi-Fi soak remain open.
