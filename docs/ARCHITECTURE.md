# Architecture

Status: važeća single-chip arhitektura, 2026-08-24.

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
- `wifi_link` + `web_server`: C6 mreža, operatorski SoftAP, privremeni servisni
  STA, remote kontrola i dijagnostika. Probe i pull OTA dijele ekskluzivni
  AP→STA→AP transition lease; Settings gašenje čeka završetak tog prijelaza.
- `p4_ota` + `p4_ota_pull`: potpisani P4-only OTA.

## Kontrolni put

FLX4 MIDI paket prolazi kroz `p4_flx4_map`, zatim se lokalno ubrizgava u
`ctrl_event_t` red. `deck_core` obrađuje događaj i publicira novi snapshot.
LED stanje se računa iz P4 statea i preko registriranog sinka šalje izravno
`p4_flx4_host` komponenti. Reconnect ponovno šalje kompletan LED snapshot.

Lokalni FLX4/touch događaj koji zatekne screensaver aktivnim troši se samo na
sigurno buđenje, pa PLAY ne pokreće deck nenamjernim prvim pritiskom. Wi-Fi
remote koristi zaseban `deck_core_queue_remote_event`: bilježi istu aktivnost i
probudi UI, ali autoritativnu udaljenu naredbu ipak stavlja u red iz prvog
zahtjeva jer udaljeni operator ne vidi stanje lokalnog zaslona.

## Dual-USB host

ESP32-P4 istodobno koristi oba DWC host kontrolera. USB3/HS MSC treba RX i
non-periodic TX prostor za 512-B bulk pakete, dok USB2/FS FLX4 treba periodic
TX prostor za UAC1 pakete do 384 B te manji MIDI bulk prostor. Pinani
`espressif/usb 1.5.0` nudi samo jednu globalnu FIFO bias postavku, pa
`cmake/apply_espressif_usb_fifo_patch.cmake` tijekom konfiguracije stvara
fail-closed patched kopiju `hcd_dwc.c` pod `build/pajoniiir_usb` i zamjenjuje
samo taj source u component targetu. Originalni `managed_components` ostaje
hash-ispravan, zbog čega clean build i Component Manager provjera ostaju
reproducibilni.

## Audio put

Svaki deck dekodira u bounded PCM timeline. Isti četverosekundni timeline daje
i povijest za gapless Censor: output task čita reverse read-headom dok normalni
resampler ili Master Tempo renderer napreduje autoritativni playhead u pozadini.
Otpuštanje Censora linearno ukršta reverse i već poravnati forward signal kroz
10 ms, bez transport seeka, refill prekida ili druge PCM alokacije. Kada reverse
dođe do izbačenog ruba bounded povijesti, zadnji uzorak se kratko gasi prema
tišini, dok slip timeline nastavlja naprijed.

Output task radi DSP i mixer u
blokovima, šalje master na PCM5102A te cue/headphone miks u FLX4 UAC1 ring.
Kanalni PFL tap je post-trim/post-DSP, ali prije channel fadera; kanalni CUE ga
uključuje, a `HEADPHONES MIX` ga miješa s post-fader master signalom.
`HEADPHONES LEVEL` mijenja samo monitor izlaz i koristi per-frame gain ramp kako
14-bitni MIDI skokovi ne bi stvarali klikove na granici output bloka. FLX4 MIDI
callback ne radi packet-level logiranje iz priority-7 real-time puta.
PCM5102A prati zajednički output rate od 44,1 ili 48 kHz, dok stateful linearni
resampler pretvara FLX4 cue/headphone tap u njegov fiksni 44,1-kHz četverokanalni
format i čuva fazu između proizvoljno podijeljenih output blokova. UAC packetizer
prilagođava broj frameova USB mikroframe ritmu. Brojači predanih i odbačenih
blokova, ringa i clock korekcija dostupni su u dijagnostici.

UAC clock korekcija drži normalni rad unutar 3/8–5/8 ringa, dok zasebni health
monitor koristi širi alarmni omotač 1/4–3/4. Samo aktivni playback može otvoriti
`UAC_RING_PRESSURE` ili `UAC_DATA_LOSS`; idle USB zero-fill underflow osvježava
baseline bez alarma. Trenutni pragovi i klasificirano stanje ringa objavljeni su
u `diagnostics.usb_headphones` status API-ja. Audio output deadline ostaje dva
256-frame bloka: 10.668 us na 48 kHz odnosno 11.610 us na 44,1 kHz.

## Uklonjena arhitektura

Raniji pomoćni USB kontroler, UART control protocol, bulk/profile transfer,
peer OTA/status, debug AP i međupanački I2S PCM link više nisu dio builda ni
runtimea. Povijesni validation/spec zapisi mogu ih spominjati isključivo kao
evidenciju prethodne izvedbe.
