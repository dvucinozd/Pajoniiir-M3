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

Trenutni handoff: na uređaju je app-only `M3-48-g435bcfe-dirty / factory`, dok
je `M3-41-g133f399 / ota_0` posljednji potpisani rollback baseline. FLX4, USB3
knjižnica od 191 trake, Wi-Fi i PCM5102A master rade. EYOYO
`DSI506 / DYL0023` prihvaćen je za 800×480 sliku, boje, nativni landscape i
poravnanje; aktivan je `bsp_p4_m3`. Touch `0x38` radi na 100 kHz s prihvaćenim
mapiranjem `swap_xy=0`, `mirror_x=1`, `mirror_y=1`; potvrđeni su kartice,
Backlight drag i kontrole na obje strane. Master Tempo, Shift+Browse/Load,
screensaver wake te corner/edge i two-finger safety gateovi sada su prihvaćeni;
solo i dual-deck waveform sync također je prihvaćen na 50,0146-Hz profilu;
desetominutni zajednički integration soak također prolazi. Settings eyes-on i
pregledani 800×480 screenshot gate te produženi cold-power/reconnect blok
također su zatvoreni; sljedeći rad je release hardening i reproducibilni clean
build. Detalji i odbijeni kandidati
zapisani su u display bring-up dokumentu.

Novi build ispravlja zaostajanje stvarnog audio-tempa uz uključen MT. PC
regresije, build, solo prijelazi i kratki 48/48-kHz dual-deck fizički retest
prolaze. Prihvaćen je i 44,1/48-kHz mixed-rate MT bez novih output-late, PCM ili
UAC gubitaka. Vidi
[Master Tempo response zapis](validation/2026-08-31-master-tempo-response.md).
Shift + Browse force-open/ubrzano kretanje i Shift + Load D1/D2 routing također
su fizički prihvaćeni; vidi
[Browse/Load validation zapis](validation/2026-09-01-shift-browse-load.md).
Prvi lokalni PLAY sada sigurno samo budi screensaver, drugi se izvršava, a
touch ne aktivira kontrolu ispod; vidi
[screensaver validation zapis](validation/2026-09-01-screensaver-wake.md).
Corner/edge odziv i sigurnost pri dva istodobna dodira također su prihvaćeni;
vidi [touch edge validation zapis](validation/2026-09-01-touch-edge-multitouch.md).
Waveform cache/scanout dijagnoza i završni dual-deck PASS zapisani su u
[DSI waveform sync validaciji](validation/2026-09-01-dsi506-waveform-sync.md).
Zajednički display/touch/audio/USB/Wi-Fi rezultat zapisan je u
[desetominutnoj integration soak validaciji](validation/2026-09-01-integration-soak.md).
Settings eyes-on, Backlight slider i točna screenshot obnova zapisani su u
[Settings UI validaciji](validation/2026-09-01-settings-ui.md).
Tri cold boota, FLX4/USB3 reconnect i završni load/PFL rezultat zapisani su u
[cold-power/reconnect validaciji](validation/2026-09-01-cold-power-reconnect.md).

`reference/` sadrži izvore za MIDI i Rekordbox format. Svi ostali dokumenti
koji nisu navedeni u gornjem popisu — uključujući bench bilješke, stare odluke,
audite te mape `validation/`, `superpowers/specs/` i `migration/` — povijesni su
zapisi i ne opisuju nužno važeću topologiju.
