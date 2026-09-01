# Documentation Status

Status: 2026-09-01.

## Trenutni handoff

- aktualni bench image: app-only `M3-48-g435bcfe-dirty`, `factory`, SHA-256
  `2CDAB5D7C859F28F26E2BB02CDC5B711DA4A25480EA081B18A2C3EF963DF3455`;
- posljednji potpisani rollback/release baseline: `M3-41-g133f399`, `ota_0`;
- potvrđeno: FLX4 MIDI In/Out/UAC, USB3 knjižnica od 191 trake, Wi-Fi
  SoftAP/web kontrola, signed OTA i gapless Censor na D1/D2;
- Wi-Fi ostaje uključen tijekom nastavka razvoja;
- EYOYO `DSI506 / DYL0023` spojen je i slika je prihvaćena: 800×480 RGB888,
  nativni landscape, ispravne boje i poravnanje, burst/no-frame-ACK te
  50,0146-Hz profil s VFP `109`;
- aktivan je `bsp_p4_m3`, a legacy `bsp_jc4880` je izuzet iz produkcijskog
  linkanja; normalni boot više nema privremene testne trake;
- FT5426 touch na I2C `0x38` radi na 100 kHz uz `swap_xy=0`, obje mirror osi;
  potvrđene su sve četiri kartice, Backlight drag i kontrole na obje strane;
- PCM5102A je spojen i hardverski prihvaćen: L/R, tihi idle, 44,1/48 kHz,
  mixed-rate dual-deck, headroom i limiter;
- MT zaostajanje zvuka reproducirano je i ispravljeno sidrenjem grainova na
  source clock; 18-case PCM onset gate, puni host suite, 300-s PC soak i build
  prolaze. [Validation zapis](validation/2026-08-31-master-tempo-response.md)
  razdvaja PC rezultat od fizičkih gateova;
- na stalnom negativnom tempu zabilježeni su pucketanje, waveform stutter,
  watchdog/CPU starvation i povremeni bljesak ekrana. Novi kandidat iste
  dirty verzije (SHA gore) dodaje bounded cache PCM pretrage; PC izlaz je
  identičan uz manje source read poziva. Solo pitch prijelazi na D1 i D2 sada
  imaju potvrđen zvuk bez pucketanja i fluidan waveform. Prvi zajednički
  48/48-kHz pokušaj je odbačen zbog trzanja, PCM/UAC gubitka i bljeska zaslona;
  aktualni SHA ispravlja reproduciranu producer/consumer PCM cursor utrku,
  dodaje bounded hijerarhijsku correlation pretragu i prolazi 313-assert
  timeline gate, puni host suite, build i 300-s PC soak. Kratki fizički
  48/48-kHz dual-deck retest s D1 +5 % i D2 -5 % prihvaćen je uz nulte PCM i
  UAC drop/overflow delte; operator je potvrdio čist zvuk, fluidan waveform i
  stabilan zaslon. Jedan završni output-late od 11024 us nije imao posljedicu.
  Naknadni 44,1/48-kHz mixed-rate MT test također je prihvaćen bez novih
  output-late događaja te uz nulte PCM i UAC drop/overflow delte; operator je
  potvrdio zvuk, waveform i zaslon;
- Shift + Browse force-open, ubrzano Library kretanje i Shift + Load D1/D2
  routing prihvaćeni su eyes-on i API provjerom bez audio counter delte;
- D1/D2 target selektori dobili su ispravno početno vizualno stanje. Fizički
  D1 Hot Cue A set/shifted-clear odmah osvježava UI, nenulti recall je
  trenutačan i čist, a NVS cue preživljava reboot i reload iste trake; vidi
  [validation zapis](validation/2026-09-01-hot-cue-ui.md);
- direct-FLX4 screensaver wake regresija reproducirana je i ispravljena;
  fizički test s učitanom trakom potvrdio je wake-only prvi PLAY, izvršavanje
  drugog PLAY-a i touch wake bez aktiviranja kontrole ispod. Vidi
  [validation zapis](validation/2026-09-01-screensaver-wake.md);
- corner/edge i two-finger safety provjera prihvaćena je bez ghost akcije,
  stuck pressa, promjene postavki ili audio counter delte. LVGL input ostaje
  namjerno single-pointer; vidi
  [validation zapis](validation/2026-09-01-touch-edge-multitouch.md);
- solo i dual-deck waveform sync gate prihvaćen je 2026-09-01. Refresh-bound
  50,0146-Hz prikaz i top-to-bottom direct-PPA raspored daju oštre, fluidne
  waveforme bez bljeskanja ili audio posljedice; vidi
  [validation zapis](validation/2026-09-01-dsi506-waveform-sync.md);
- desetominutni zajednički display/touch/master/headphones/dual-deck/USB3/
  Wi-Fi soak prihvaćen je 2026-09-01. Svih 600 s oba su decka i loopa ostala
  aktivna; 1840 status pollova prošlo je bez greške, a PCM underrun, UAC
  drop/overflow/underflow i service-log drop delte bile su 0. Operator je
  potvrdio čist master i slušalice, fluidne waveforme, responzivan touch,
  stabilan zaslon i FLX4. Pet output-late događaja do 12522 us nije imalo
  posljedicu i ostaje monitoring nalaz; vidi
  [validation zapis](validation/2026-09-01-integration-soak.md);
- nastavak: preostali Settings eyes-on gate, screenshot baseline te produženi
  cold-power/reconnect soak.

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
