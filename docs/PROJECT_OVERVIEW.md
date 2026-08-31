# Project Overview

Status: važeći opis projekta i bench topologije, 2026-08-31.

Pajoniiir-M3 je standalone dual-deck DJ uređaj bez računala. Jedan ESP32-P4
obavlja USB host, playback, DSP, library, UI i kontrolnu logiku.

## Glavni tokovi

1. FLX4 na USB2 šalje MIDI P4 host klijentu.
2. `p4_flx4_host` mapira MIDI u lokalne semantičke događaje.
3. `deck_core` i `audio_engine` ažuriraju autoritativni deck/mixer state.
4. LED snapshot ide izravno natrag na FLX4 MIDI OUT.
5. Master stereo ide preko I2S-a na PCM5102A.
6. Cue/headphone miks ide izravno preko FLX4 UAC1 OUT endpointa.
7. Rekordbox medij na USB3 puni library, ANLZ i audio decode put.
8. LVGL prikazuje state na 800×480 DSI panelu; Wi-Fi remote koristi C6.

## Trenutno stanje

`M3-41-g133f399 / ota_0` ostaje potpisani rollback baseline za FLX4 MIDI/UAC,
USB3 library/decode, Wi-Fi remote/OTA i dual-deck DSP, uključujući gapless
Censor. App-only `M3-46-gee004d6-dirty / factory` dodaje aktivni
`bsp_p4_m3` i fizički prihvaćenu DSI sliku na EYOYO `DSI506 / DYL0023`:
800×480 RGB888, ispravne boje, nativni landscape i poravnanje bez cyclic wrapa.
FT5426 touch na `0x38` stabiliziran je na 100 kHz uz obje mirror osi. Korisnik
je 2026-08-31 potvrdio kartice, Backlight drag i kontrole na obje strane.
PCM5102A iz koraka 5 spojen je i hardverski prihvaćen 2026-08-26: L/R, tihi idle, 44,1/48-kHz switching,
mixed-rate dual-deck i full-master limiter prošli su bez čujnog clippinga, PCM
underruna ili UAC gubitka.

Bench ostaje na uključenom Wi-Fi SoftAP-u. Fokusirani touch gate je prihvaćen;
nastavak su Master Tempo i Shift + Browse/Load UI gate, screensaver/multitouch
rubna provjera te zajednički display/audio/USB/Wi-Fi soak.

## Granice sustava

- P4 je jedini vlasnik playbacka, timestampa, loopova, cueva, miksera i LED
  odluka.
- FLX4 je operator surface i USB audio uređaj, ne vlasnik aplikacijskog stanja.
- C6 je mrežni koprocesor preko ESP-Hosteda, ne kontrolni procesor.
- Ugrađeni mikrofon, NS4150 speaker amp i Ethernet ostaju ugašeni.
- Nema UART međuprocesorskog protokola, profila `.s3bin`, drugog firmware
  targeta ni međupanačkog PCM transporta.

## Izvor istine

- MIDI adrese: `docs/reference/Pioneer-DDJ-FLX4.midi.xml`
- Hardver: `docs/HARDWARE_WIRING.md`
- Komponente i ownership: `docs/ARCHITECTURE.md`
- Prioriteti: `docs/DEVELOPMENT_PLAN.md`
- Prihvatno pokretanje: `docs/STARTUP_CHECKLIST.md`
