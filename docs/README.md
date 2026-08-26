# Dokumentacija

Aktualni ulaz:

- [Project Overview](PROJECT_OVERVIEW.md)
- [Architecture](ARCHITECTURE.md)
- [Hardware Wiring](HARDWARE_WIRING.md)
- [DDJ-FLX4 MIDI Map](DDJ_FLX4_MIDI_MAP.md)
- [Development Plan](DEVELOPMENT_PLAN.md)
- [Startup Checklist](STARTUP_CHECKLIST.md)
- [Risk Register](RISK_REGISTER.md)
- [P4 OTA](OTA-UPDATE.md)
- [Documentation Status](DOCUMENTATION_STATUS.md)

Trenutni handoff: na uređaju je `M3-41-g133f399 / ota_0`; FLX4, USB3 knjižnica
od 191 trake, Wi-Fi i PCM5102A master rade, deckovi su zaustavljeni, a Wi-Fi
ostaje uključen. PCM5102A L/R, tihi idle, 44,1/48-kHz switching, mixed-rate
dual-deck i limiter prihvaćeni su 2026-08-26. Daljnji hardverski rad čeka
5,0-inčni DSI/FT5426 zaslon.

`reference/` sadrži izvore za MIDI i Rekordbox format. Svi ostali dokumenti
koji nisu navedeni u gornjem popisu — uključujući bench bilješke, stare odluke,
audite te mape `validation/`, `superpowers/specs/` i `migration/` — povijesni su
zapisi i ne opisuju nužno važeću topologiju.
