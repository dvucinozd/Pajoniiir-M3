# Pajoniiir-M3

Standalone dual-deck DJ sustav na jednom ESP32-P4 čipu. Projekt izravno spaja
Pioneer DDJ-FLX4, Rekordbox USB medij, 800×480 MIPI-DSI zaslon, PCM5102A master
DAC i ESP32-C6 Wi-Fi modul, bez računala i bez pomoćnog kontrolnog MCU-a.

## Hardverska topologija

- USB1 / CH340C: 5 V napajanje, flashing i serijska dijagnostika
- USB2 / FS Host: DDJ-FLX4 MIDI In/Out i UAC1 slušalice
- USB3 / HS Host: Rekordbox USB Flash / MSC
- GPIO1/2/3: PCM5102A I2S master izlaz
- MIPI-DSI: fizički prihvaćen 800×480 landscape UI i FT5426 touch na EYOYO
  `DSI506 / DYL0023`; touch koristi I2C `0x38` na 100 kHz te obostrano
  zrcaljenje koordinata bez zamjene osi
- ESP32-C6 preko SDIO: Wi-Fi 6 / ESP-Hosted, operatorski SoftAP
  `Pajoniiir-M3` te privremeni APSTA servisni uplink za potpisani P4 OTA, bez
  gašenja lokalnog DHCP/HTTP puta

ESP32-P4 je jedini autoritet za playback, deck/mixer state, USB host klijente,
MIDI mapiranje, LED feedback, audio DSP, biblioteku i UI. `control_link` u P4
izvoru više nije serijski link: to je samo lokalni red semantičkih događaja i
LED callback prema izravnom FLX4 USB hostu.

## Trenutni bench status

Na benchu je app-only UI/Hot Cue kandidat `M3-48-g435bcfe-dirty` u `factory`
particiji, SHA-256
`2CDAB5D7C859F28F26E2BB02CDC5B711DA4A25480EA081B18A2C3EF963DF3455`.
Posljednji potpisani rollback/release baseline ostaje `M3-41-g133f399` u
`ota_0`. FLX4 MIDI In/Out/UAC, USB3 knjižnica od 191 trake, SoftAP
`Pajoniiir-M3`, web kontrola i potpisani OTA rade; Wi-Fi ostaje uključen.

PCM5102A je 2026-08-26 fizički spojen na GPIO1/2/3 i hardverski prihvaćen s
44,1-kHz i 48-kHz izvorima, mixed-rate dual-deck miksom, tihim idle izlazom te
kontroliranim full-master limiter testom bez PCM underruna ili UAC gubitka.
5,0-inčni EYOYO `DSI506 / DYL0023` sada je spojen i fizički prihvaćen za sliku.
Aktivni `bsp_p4_m3` koristi jednu DSI lane na 800 Mbps, RGB888, 27,777 MHz,
HFP/HSW/HBP `59/2/45`, VFP/VSW/VBP `109/2/22` (50,0146 Hz) i burst video bez frame ACK-a.
Korisnik je potvrdio ispravne boje, čitljiv nativni landscape, redoslijed GUI-ja
i uklonjeno horizontalno omatanje; privremene testne trake uklonjene su iz
normalnog boota. FT5426 touch je stabiliziran na 100 kHz; `swap_xy=0`,
`mirror_x=1`, `mirror_y=1`. Korisnik je potvrdio sve četiri kartice, Backlight
drag te kontrole na obje strane Overviewa. Shift+Browse force-open i ubrzano
pomicanje te Shift+Load routing na oba decka fizički su prihvaćeni 2026-09-01.
Početni D1/D2 target selektori sada su čitljivi prije prvog dodira, a lokalni
Hot Cue set/clear odmah osvježava zaslon. D1 cue A prošao je set, shifted clear,
nenulti recall i NVS reload nakon reboota; vidi
[Hot Cue UI zapis](docs/validation/2026-09-01-hot-cue-ui.md).
Screensaver wake gate sada je također prihvaćen: prvi lokalni PLAY samo budi
UI, sljedeći radi normalno, a touch ne aktivira kontrolu ispod screensavera.
Corner/edge i two-finger safety gate prošli su bez ghost akcije ili stuck pressa.
Refresh-sinkronizirani waveform kandidat također je fizički prihvaćen: solo D1
i oba istodobna decka imaju oštar, fluidan prikaz bez bljeskanja ili audio
posljedice. Dual-deck direct-PPA redoslijed prati scanout odozgo prema dolje;
vidi [waveform sync zapis](docs/validation/2026-09-01-dsi506-waveform-sync.md).
Desetominutni zajednički display/touch/master/headphones/dual-deck/USB/Wi-Fi
soak također je prihvaćen: 1840 status pollova bez greške, nulte PCM/UAC i
service-log loss delte te fizički čist zvuk, fluidni waveformi, responzivan
touch i stabilan zaslon. Pet izoliranih output-late događaja do 12522 us nije
imalo posljedicu i ostaje za monitoring. Vidi
[integration soak zapis](docs/validation/2026-09-01-integration-soak.md).
Produženi/cold-power/reconnect soak, preostali Settings eyes-on gate i obnova
provjerenog screenshot baselinea ostaju otvoreni.

Master Tempo test otkrio je da zvuk može zaostajati za ispravno prikazanim
tempom. Ispravljeno je vremensko sidrenje audio-isječaka; novi PCM onset test
(18 slučajeva), puni host suite, 300-s PC soak i ESP-IDF build prolaze.
Fizički solo odziv i kratki 48/48-kHz dual-deck deadline sada su potvrđeni; vidi
[zapis ispravka](docs/validation/2026-08-31-master-tempo-response.md).
Naknadni negativni pitch test otkrio je pucketanje i zastajkivanje uz watchdog.
Aktualni kandidat dodaje internu predmemoriju PCM pretrage; PC regresije prolaze.
Solo pitch prijelazi na D1 i D2 potvrđeni su uz zvuk bez pucketanja i fluidan
waveform. Prvi zajednički 48/48-kHz test je odbačen zbog trzanja, PCM/UAC
gubitka i bljeska zaslona. Aktualni SHA ispravlja reproduciranu utrku između
producer indeksa i objavljenog PCM cursora te ograničava correlation rad
hijerarhijskom pretragom; puni host suite, 313-assert timeline gate, build i
300-s PC soak prolaze. Kratki fizički 48/48-kHz dual-deck retest s D1 +5 % i
D2 -5 % prihvaćen je uz čist zvuk, fluidan waveform, stabilan zaslon i nulte
PCM/UAC drop/overflow delte. Jedan završni output-late od 11024 us nije imao
čujnu ili vizualnu posljedicu. Naknadni 44,1/48-kHz mixed-rate MT test također
je prihvaćen bez novih output-late, PCM ili UAC gubitaka; operator je potvrdio
zvuk, waveform i zaslon. Desetominutni kombinirani soak sada prolazi; duži
cold-power/reconnect acceptance ostaje otvoren.

Aktualni baseline potvrđuje FLX4 MIDI,
LED/UAC, USB3 biblioteka, modalni jog Loop Adjust, deck-local Quantize, prvi
Wi-Fi remote događaj nakon screensavera te shifted Censor, Sync Master i
Reloop Stop/Forget potvrđeni su na stvarnom uređaju. Shifted Beat FX beat-size,
FLANGER, DELAY i reset, jednobeatni CUE/LOOP CALL skokovi te inertni shifted
Smart helperi također su prošli hardverski acceptance. Aktualni baseline koristi
gapless slip-reverse Censor: D1 48-kHz i D2 44,1-kHz testovi potvrdili su čujni
reverse, napredovanje forward playheada i gladak 10-ms release bez seeka.
`censor_active` je na oba decka prošao `false→true→false`, a kontrolirane
press/release delte za output-late, PCM underrun, UAC drop/overflow i service-log
drop bile su nula.

Screen-independent release hardening sada uključuje testirani UAC health
monitor: tijekom aktivnog playbacka alarmira izlazak FLX4 headphone ringa iz
1/4–3/4 kapaciteta te nove drop/overflow/underflow delte, dok zaustavljeni
deckovi samo osvježavaju baseline i ne stvaraju lažne idle-underflow događaje.
Pragovi i stanje (`idle`, `nominal`, `low`, `high`, `unavailable`) dostupni su
u `/api/status`, a incidenti ulaze u rate-limitirani servisni log.
Utišani single-deck i 48/44,1-kHz dual-deck hardware smoke potvrdili su
`nominal→idle` prijelaze bez novih audio/UAC brojača ili servisnih incidenata.

## Struktura

```text
firmware/main-deck-p4/   ESP-IDF firmware
firmware/common/         dijeljeni P4 OTA/health moduli
docs/                    aktivna dokumentacija i povijesni zapisi
tests/                   P4 host regresije i UI simulator
tools/                   P4 OTA i razvojni alati
```

Autoritativna MIDI referenca je
[`docs/reference/Pioneer-DDJ-FLX4.midi.xml`](docs/reference/Pioneer-DDJ-FLX4.midi.xml).

## Build i testovi

Obavezna verzija je ESP-IDF v6.0.2.

```powershell
. C:\Espressif\tools\Microsoft.v6.0.2.PowerShell_profile.ps1
idf.py --version
Set-Location firmware\main-deck-p4
idf.py build
```

Host regresije:

```powershell
$env:Path = "$env:Path;C:\msys64\ucrt64\bin"
.\tests\run_p4_host_tests.ps1
```

Build automatski stvara patched kopiju pinanog `espressif/usb 1.5.0` HCD-a
unutar `build/pajoniiir_usb`. Ona daje različitu FIFO raspodjelu USB3/HS MSC i
USB2/FS FLX4 MIDI/UAC kontrolerima; `managed_components` ostaje netaknut i ne
smije se ručno uređivati.

Dugi Master Tempo soak i headless UI screenshot gate:

```powershell
.\tests\audio_keylock_soak\run_audio_keylock_soak.ps1
.\tests\ui_simulator\run_ui_simulator_e2e.ps1
```

## Dokumentacija

- [Pregled projekta](docs/PROJECT_OVERVIEW.md)
- [Arhitektura](docs/ARCHITECTURE.md)
- [Spajanje hardvera](docs/HARDWARE_WIRING.md)
- [DSI-506 bring-up i acceptance zapis](docs/DISPLAY_DSI506_BRINGUP.md)
- [DDJ-FLX4 MIDI mapa](docs/DDJ_FLX4_MIDI_MAP.md)
- [Plan razvoja](docs/DEVELOPMENT_PLAN.md)
- [Startup checklist](docs/STARTUP_CHECKLIST.md)
- [Registar rizika](docs/RISK_REGISTER.md)
- [OTA ažuriranje](docs/OTA-UPDATE.md)
- [Status dokumentacije](docs/DOCUMENTATION_STATUS.md)

Povijesni dokumenti u `docs/validation`, `docs/superpowers/specs` i
`docs/migration` bilježe ranije izvedbe i nisu opis važeće topologije.
