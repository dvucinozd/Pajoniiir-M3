# Documentation Status

Status: 2026-08-31.

## Trenutni handoff

- aktualni bench image: app-only `M3-46-gee004d6-dirty`, `factory`, SHA-256
  `00A131B3CE5A1DB9B009007316A3940DA9EBD6E58864E2F25EC4CB2676742988`;
- posljednji potpisani rollback/release baseline: `M3-41-g133f399`, `ota_0`;
- potvrđeno: FLX4 MIDI In/Out/UAC, USB3 knjižnica od 191 trake, Wi-Fi
  SoftAP/web kontrola, signed OTA i gapless Censor na D1/D2;
- Wi-Fi ostaje uključen tijekom nastavka razvoja;
- EYOYO `DSI506 / DYL0023` spojen je i slika je prihvaćena: 800×480 RGB888,
  nativni landscape, ispravne boje i poravnanje, burst/no-frame-ACK;
- aktivan je `bsp_p4_m3`, a legacy `bsp_jc4880` je izuzet iz produkcijskog
  linkanja; normalni boot više nema privremene testne trake;
- FT5426 touch na I2C `0x38` radi na 100 kHz uz `swap_xy=0`, obje mirror osi;
  potvrđene su sve četiri kartice, Backlight drag i kontrole na obje strane;
- PCM5102A je spojen i hardverski prihvaćen: L/R, tihi idle, 44,1/48 kHz,
  mixed-rate dual-deck, headroom i limiter;
- nastavak: UI Master Tempo i Shift + Browse/Load gate, screensaver/multitouch
  rubna provjera, zatim zajednički display/master/headphones/dual-deck/Wi-Fi
  integration soak.

## Aktivni dokumenti

- `AGENTS.md` — obvezne projektne upute i trenutni handoff za sljedeću sesiju
- `README.md` — ulaz u projekt i build naredbe
- `docs/README.md` — indeks aktivne dokumentacije
- `docs/PROJECT_OVERVIEW.md` — scope i glavni tokovi
- `docs/ARCHITECTURE.md` — ownership i komponente
- `docs/HARDWARE_WIRING.md` — važeće spajanje
- `docs/DISPLAY_DSI506_BRINGUP.md` — identifikacija, bring-up i display acceptance
- `docs/DDJ_FLX4_MIDI_MAP.md` — MIDI acceptance ledger
- `docs/DEVELOPMENT_PLAN.md` — prioriteti nastavka
- `docs/STARTUP_CHECKLIST.md` — bench acceptance
- `docs/RISK_REGISTER.md` — otvoreni rizici
- `docs/OTA-UPDATE.md` — P4-only OTA
- `firmware/main-deck-p4/PINOUT_P4.md` — sažetak aktivnih pinova

## Reference

- `docs/reference/Pioneer-DDJ-FLX4.midi.xml` je autoritativan za FLX4 MIDI
  adrese.
- `docs/reference/DDJ-FLX4_MIDI_message_List.md` i PDF dopunjuju LED i settings
  poruke.
- `docs/reference/DSI506_PRODUCT_REFERENCE.jpg` je korisnikova prodajna
  referentna fotografija. Isporučeni modul naknadno je identificiran kao EYOYO
  `DSI506 / DYL0023`; fotografija sama i dalje nije dokaz bridge identiteta.

## Povijesno

Svi dokumenti koji nisu navedeni pod **Aktivni dokumenti** ili **Reference** —
uključujući bench bilješke, stare odluke, audite te mape `docs/validation`,
`docs/superpowers/specs` i `docs/migration` — datirani su zapisi prethodnih
implementacija. Ne smiju se koristiti za aktualni wiring, build target ili
runtime ownership. Spominjanje ranije pomoćne kontrolne ploče u njima je
povijesna evidencija, ne podržana konfiguracija.

Dokumenti za umirovljeni UART protocol, profile transfer, peer debug/OTA i
stari control-board decision uklonjeni su zajedno s pripadajućim kodom.
