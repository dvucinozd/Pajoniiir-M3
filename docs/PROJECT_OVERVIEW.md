# Project Overview

Status: važeći opis projekta, 2026-08-22.

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
