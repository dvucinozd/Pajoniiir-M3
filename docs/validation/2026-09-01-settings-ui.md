# Settings UI and 800x480 screenshot gate — 2026-09-01

## Scope

Focused physical eyes-on/touch acceptance of the Settings screen, followed by
the deterministic headless 800x480 screenshot regression. This gate covers
layout, readability, the local Backlight slider and exact restoration of the
Settings screen after the screensaver. It does not replace the separate Wi-Fi
AP/STA transition, client-count or prolonged cold-power/reconnect gates.

## Physical result

On the connected EYOYO `DSI506 / DYL0023`, the operator confirmed that the
requested Settings content was visible and readable and that the Backlight
slider responded correctly to touch. The screen remained in the accepted
native landscape orientation with the previously accepted colors and
horizontal alignment.

## Deterministic screenshot result

The current `master` source was checked again after the Hot Cue UI change with:

```powershell
.\tests\ui_simulator\run_ui_simulator_e2e.ps1 -KeepArtifacts
```

The build, scripted navigation and all seven 800x480 RGB framebuffer hashes
passed. The Settings captures were also reviewed visually:

- `settings`: `10ddd37ddfc5889a5f1bddecca410cdd47bdd54144c78a9ee5000e138935bd03`;
- `settings_restored`:
  `10ddd37ddfc5889a5f1bddecca410cdd47bdd54144c78a9ee5000e138935bd03`.

The two hashes are identical, confirming exact restoration after dismissing
the screensaver. `tests/ui_simulator/baselines.json` did not require
regeneration because the current rendering already matched the reviewed
baseline exactly.

## Verdict

**PASS — Settings eyes-on/touch and reviewed 800x480 screenshot gate closed.**

The prolonged cold-power/reconnect integration gate was subsequently closed.
