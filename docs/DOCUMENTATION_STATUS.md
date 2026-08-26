# Documentation Status

Status: 2026-08-26.

## Trenutni handoff

- instalirani baseline: `M3-41-g133f399`, `ota_0`, OTA state `idle`;
- potvrđeno: FLX4 MIDI In/Out/UAC, USB3 knjižnica od 191 trake, Wi-Fi
  SoftAP/web kontrola, signed OTA i gapless Censor na D1/D2;
- oba decka su zaustavljena, a Wi-Fi ostaje uključen radi rada bez zaslona;
- 5,0-inčni zaslon još nije stigao; prodajna fotografija i oznaka `DSI-506`
  upućuju na obitelj `DSI5061/DSI5061-A`, a pre-arrival dossier je pripremljen;
- PCM5102A je spojen i hardverski prihvaćen: L/R, tihi idle, 44,1/48 kHz,
  mixed-rate dual-deck, headroom i limiter;
- nastavak: DSI/touch bring-up, UI/Master Tempo gate, zatim zajednički
  display/master/headphones/dual-deck/Wi-Fi integration soak.

## Aktivni dokumenti

- `AGENTS.md` — obvezne projektne upute i trenutni handoff za sljedeću sesiju
- `README.md` — ulaz u projekt i build naredbe
- `docs/README.md` — indeks aktivne dokumentacije
- `docs/PROJECT_OVERVIEW.md` — scope i glavni tokovi
- `docs/ARCHITECTURE.md` — ownership i komponente
- `docs/HARDWARE_WIRING.md` — važeće spajanje
- `docs/DISPLAY_DSI506_BRINGUP.md` — identifikacija i sigurni display arrival gate
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
- `docs/reference/DSI506_PRODUCT_REFERENCE.jpg` je korisnikova pre-arrival
  referentna fotografija za usporedbu s isporučenim display modulom; nije dokaz
  controller/revision identiteta.

## Povijesno

Svi dokumenti koji nisu navedeni pod **Aktivni dokumenti** ili **Reference** —
uključujući bench bilješke, stare odluke, audite te mape `docs/validation`,
`docs/superpowers/specs` i `docs/migration` — datirani su zapisi prethodnih
implementacija. Ne smiju se koristiti za aktualni wiring, build target ili
runtime ownership. Spominjanje ranije pomoćne kontrolne ploče u njima je
povijesna evidencija, ne podržana konfiguracija.

Dokumenti za umirovljeni UART protocol, profile transfer, peer debug/OTA i
stari control-board decision uklonjeni su zajedno s pripadajućim kodom.
