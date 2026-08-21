# Architecture

Status: važeća single-chip arhitektura, 2026-08-22.

## Sustav

```text
DDJ-FLX4 USB2 FS ── MIDI/UAC1 ─┐
Rekordbox USB3 HS ── MSC ──────┼─> ESP32-P4 ── I2S ──> PCM5102A master
FT5426 + DSI panel ─────────────┤       │
ESP32-C6 ── SDIO/ESP-Hosted ───┘       └─> FLX4 MIDI LED + headphones
```

USB1/CH340C je izvan aplikacijskog data puta i služi za napajanje, flashing i
serijsku dijagnostiku.

## Ownership

ESP32-P4 jedini posjeduje:

- deck state, playback position, cue/hot-cue/loop i sync odluke;
- mixer, EQ, filter, FX, scratch i Master Tempo DSP;
- Rekordbox bazu, ANLZ metapodatke i audio decode;
- FLX4 USB host, MIDI mapiranje, LED feedback i UAC1 stream;
- LVGL UI, Wi-Fi remote API, servisni log i P4 OTA.

## Komponente

- `p4_flx4_host`: USB enumeracija, MIDI IN/OUT, UAC1 i FLX4 mapper.
- `control_link`: lokalni semantic-event queue i LED sink adapter. Ime je
  naslijeđeno; komponenta nema UART, framing, heartbeat ni peer transport.
- `deck_core`: actor-like state machine i semantička ponašanja kontrola.
- `audio_engine`: dual-deck decode, PCM timeline, mixer, DSP i izlazni tapovi.
- `usb_storage` + `library`: MSC lifecycle, Rekordbox PDB/ANLZ i metadata cache.
- `ui`: LVGL ekrani, touch i PPA waveform render.
- `wifi_link` + `web_server`: C6 mreža, remote kontrola i dijagnostika.
- `p4_ota` + `p4_ota_pull`: potpisani P4-only OTA.

## Kontrolni put

FLX4 MIDI paket prolazi kroz `p4_flx4_map`, zatim se lokalno ubrizgava u
`ctrl_event_t` red. `deck_core` obrađuje događaj i publicira novi snapshot.
LED stanje se računa iz P4 statea i preko registriranog sinka šalje izravno
`p4_flx4_host` komponenti. Reconnect ponovno šalje kompletan LED snapshot.

## Audio put

Svaki deck dekodira u bounded PCM timeline. Output task radi DSP i mixer u
blokovima, šalje master na PCM5102A te cue/headphone miks u FLX4 UAC1 ring.
UAC packetizer prilagođava broj frameova USB mikroframe ritmu. Brojači predanih
i odbačenih blokova dostupni su u dijagnostici.

## Uklonjena arhitektura

Raniji pomoćni USB kontroler, UART control protocol, bulk/profile transfer,
peer OTA/status, debug AP i međupanački I2S PCM link više nisu dio builda ni
runtimea. Povijesni validation/spec zapisi mogu ih spominjati isključivo kao
evidenciju prethodne izvedbe.
