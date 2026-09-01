# Documentation

Use these documents for the current Pajoniiir-M3 hardware and firmware:

- [Project Overview](PROJECT_OVERVIEW.md)
- [Architecture](ARCHITECTURE.md)
- [Hardware Wiring](HARDWARE_WIRING.md)
- [DSI506 Bring-up and Acceptance](DISPLAY_DSI506_BRINGUP.md)
- [DDJ-FLX4 MIDI Map](DDJ_FLX4_MIDI_MAP.md)
- [Development Plan](DEVELOPMENT_PLAN.md)
- [Startup Checklist](STARTUP_CHECKLIST.md)
- [Risk Register](RISK_REGISTER.md)
- [Reliability Monitoring Plan](RELIABILITY_MONITORING_PLAN.md)
- [P4 OTA Update](OTA-UPDATE.md)
- [Documentation Status](DOCUMENTATION_STATUS.md)

## Current handoff

The board is running the clean and signed `M3-51-gafb2099` release from
`ota_0`. FLX4 MIDI In/Out/UAC, the 191-track USB3 library, PCM5102A master
audio, Wi-Fi, the 800×480 DSI506 display, FT5426 touch, and signed OTA are
working. The final release smoke confirmed both decks, master and headphone
audio, touch/backlight, and fluid waveforms at every zoom level.

The repeatable USB-recovery and long output-timing campaigns are intentionally
deferred. Their exact matrix, telemetry, and PASS/FAIL rules are preserved in
the [Reliability Monitoring Plan](RELIABILITY_MONITORING_PLAN.md).

## Evidence and historical records

`reference/` contains source material for MIDI mapping, the display, and the
Rekordbox format. Files under `validation/`, `superpowers/specs/`, and
`migration/`, plus older audits and bench notes, are dated evidence or
historical decisions. They may describe superseded hardware or firmware and
must not override the active documents listed above.

Useful current evidence includes:

- [M3-51 clean release acceptance](validation/2026-09-01-m3-51-clean-release.md)
- [DSI waveform synchronization](validation/2026-09-01-dsi506-waveform-sync.md)
- [Combined integration soak](validation/2026-09-01-integration-soak.md)
- [Cold-power and reconnect gate](validation/2026-09-01-cold-power-reconnect.md)
- [Master Tempo response](validation/2026-08-31-master-tempo-response.md)
