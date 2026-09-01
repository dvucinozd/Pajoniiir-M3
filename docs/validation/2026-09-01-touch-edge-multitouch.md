# Touch Edge and Two-Finger Safety Validation — 2026-09-01

## Result

PASS for the focused corner/edge response and two-finger safety gate on the
EYOYO `DSI506 / DYL0023` with FT5426 touch.

Bench image:

- version: `M3-47-g3f23bd2-dirty`;
- slot: `factory` (app-only);
- image size: 2,370,224 B;
- SHA-256:
  `B20D36AF3715E8E2BD42EAB513FA505A39916B6BCC1E37E8C84C04C1D7519B28`.

## Physical checks

1. A touch at the outer upper-left area of the `OVERVIEW` tab selected
   Overview.
2. A touch at the outer upper-right area of the `SETTINGS` tab selected
   Settings.
3. On Settings, two simultaneous fingers were held for two seconds on the
   non-action `DISPLAY` and `SYSTEM STATUS` headings. Releasing both caused no
   ghost activation, stuck press, backlight/Wi-Fi/master-trim/cue-mode change,
   display flash or loss of responsiveness. A subsequent Overview tap worked.
4. Hot Cues target D2 was selected while D2 had no loaded track. Presses at the
   outer lower-left edge of `CUE E` and outer lower-right edge of `CUE H`
   produced only the expected visual press response. Neither press started D2
   or changed screens.

Immediate API evidence after the checks:

- D1 `READY`, `playing=false`;
- D2 `IDLE`, `playing=false`;
- FLX4 present;
- output-late `0`;
- PCM underrun D1/D2 `0/0`;
- UAC dropped blocks / overflow frames `0/0`;
- `data_loss=false`.

## Scope boundary

This is a two-finger **safety** acceptance: simultaneous contact must not cause
ghost actions, a stuck pointer or instability. The current LVGL input backend
intentionally requests one touch point, so this result does not claim two
independently tracked cursors or a two-finger gesture feature.

The focused corner/multitouch-safety block is closed. The long combined
display/touch/master/headphones/dual-deck/Wi-Fi integration soak remains open.
