# Dokumentacija

Aktualni ulaz:

- [Project Overview](PROJECT_OVERVIEW.md)
- [Architecture](ARCHITECTURE.md)
- [Hardware Wiring](HARDWARE_WIRING.md)
- [DSI-506 Bring-up and Acceptance](DISPLAY_DSI506_BRINGUP.md)
- [DDJ-FLX4 MIDI Map](DDJ_FLX4_MIDI_MAP.md)
- [Development Plan](DEVELOPMENT_PLAN.md)
- [Startup Checklist](STARTUP_CHECKLIST.md)
- [Risk Register](RISK_REGISTER.md)
- [P4 OTA](OTA-UPDATE.md)
- [Documentation Status](DOCUMENTATION_STATUS.md)

Trenutni handoff: na uređaju je app-only `M3-46-gee004d6-dirty / factory`, dok
je `M3-41-g133f399 / ota_0` posljednji potpisani rollback baseline. FLX4, USB3
knjižnica od 191 trake, Wi-Fi i PCM5102A master rade. EYOYO
`DSI506 / DYL0023` prihvaćen je za 800×480 sliku, boje, nativni landscape i
poravnanje; aktivan je `bsp_p4_m3`. Touch `0x38` radi na 100 kHz s prihvaćenim
mapiranjem `swap_xy=0`, `mirror_x=1`, `mirror_y=1`; potvrđeni su kartice,
Backlight drag i kontrole na obje strane. Sljedeći rad su Master
Tempo/Shift+Browse/Load, screensaver/multitouch rubna provjera i zajednički
integration soak. Detalji i odbijeni kandidati zapisani su u display
bring-up dokumentu.

`reference/` sadrži izvore za MIDI i Rekordbox format. Svi ostali dokumenti
koji nisu navedeni u gornjem popisu — uključujući bench bilješke, stare odluke,
audite te mape `validation/`, `superpowers/specs/` i `migration/` — povijesni su
zapisi i ne opisuju nužno važeću topologiju.
