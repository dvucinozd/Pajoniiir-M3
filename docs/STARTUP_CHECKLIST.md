# Startup Checklist

Status: važeći bench postupak, 2026-08-23.

## Priprema

- [ ] P4 je napajan preko USB1/CH340C.
- [ ] DDJ-FLX4 je na USB2 FS Host portu.
- [ ] Rekordbox medij je na USB3 HS Host portu.
- [ ] PCM5102A je spojen na GPIO1/2/3 i zajednički GND.
- [ ] ESP-IDF profil javlja točno `ESP-IDF v6.0.2`.

## Build i flash

```powershell
. C:\Espressif\tools\Microsoft.v6.0.2.PowerShell_profile.ps1
idf.py --version
Set-Location firmware\main-deck-p4
idf.py build
idf.py -p COM17 flash monitor
```

- [ ] Build završi exit kodom 0.
- [ ] Boot nema reset loop, abort ni watchdog.
- [ ] DSI UI i FT5426 touch rade u 800×480 landscapeu.

## USB i kontrola

- [ ] FLX4 se enumerira na P4 kao MIDI/UAC uređaj.
- [ ] Play, Cue, jog, tempo, faderi, EQ i browse daju očekivani P4 state.
- [ ] LED feedback prati state i potpuno se obnovi nakon reconnecta.
- [ ] Rekordbox medij se mounta, library se učita i track se može loadati na oba
  decka.

## Audio

- [ ] Master L/R izlazi preko PCM5102A bez speaker-amp šuma.
- [ ] PFL/master cue i headphone mix izlaze preko FLX4 slušalica.
- [ ] Nema kontinuiranih UAC dropova, underruna ili clippinga.
- [ ] UAC je bez aktivnog dropa/overflowa i s 44,1-kHz i s 48-kHz izvornim
  trakama; promjena source ratea ne mijenja brzinu ni visinu tona.
- [ ] Dual-deck playback, pitch/Master Tempo i scratch ostaju stabilni.

## Mreža i servis

- [ ] C6/ESP-Hosted se inicijalizira.
- [ ] Settings razlikuje spremljeni Wi-Fi zahtjev od stvarnog
  OFF/STARTING/AP/STA/ERROR stanja i prikazuje aktualnu IP adresu.
- [ ] SoftAP `Pajoniiir-M3` prihvaća najmanje dva istodobna klijenta i broj
  klijenata na Settings ekranu prati connect/disconnect.
- [ ] Wi-Fi remote može se uključiti i vratiti u AP način nakon OTA probea.
- [ ] P4 reset sa spremljenim Wi-Fi OFF stanjem ne ostavlja C6 ni SoftAP
  aktivnim.
- [ ] Zahtjev za gašenje tijekom probea/OTA-a izvrši se tek nakon obnove AP-a;
  nema racea, srušenog netifa ni izgubljenog ESP-Hosted transporta.
- [ ] `/api/status` prikazuje FLX4 i USB-headphone dijagnostiku.
- [ ] P4-only potpisani OTA odbija pogrešan chip, projekt ili potpis.

Svaki hardware smoke zapisuje verziju firmwarea, wiring, COM port, medij,
rezultat i relevantne dijagnostičke brojače.
