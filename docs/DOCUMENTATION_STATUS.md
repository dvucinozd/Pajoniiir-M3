# Documentation Status

Status: 2026-08-24.

## Aktivni dokumenti

- `README.md` — ulaz u projekt i build naredbe
- `docs/PROJECT_OVERVIEW.md` — scope i glavni tokovi
- `docs/ARCHITECTURE.md` — ownership i komponente
- `docs/HARDWARE_WIRING.md` — važeće spajanje
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

## Povijesno

Svi dokumenti koji nisu navedeni pod **Aktivni dokumenti** ili **Reference** —
uključujući bench bilješke, stare odluke, audite te mape `docs/validation`,
`docs/superpowers/specs` i `docs/migration` — datirani su zapisi prethodnih
implementacija. Ne smiju se koristiti za aktualni wiring, build target ili
runtime ownership. Spominjanje ranije pomoćne kontrolne ploče u njima je
povijesna evidencija, ne podržana konfiguracija.

Dokumenti za umirovljeni UART protocol, profile transfer, peer debug/OTA i
stari control-board decision uklonjeni su zajedno s pripadajućim kodom.
