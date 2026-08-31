# Pajoniiir-M3

Standalone dual-deck DJ sustav na jednom ESP32-P4 čipu. Projekt izravno spaja
Pioneer DDJ-FLX4, Rekordbox USB medij, 800×480 MIPI-DSI zaslon, PCM5102A master
DAC i ESP32-C6 Wi-Fi modul, bez računala i bez pomoćnog kontrolnog MCU-a.

## Hardverska topologija

- USB1 / CH340C: 5 V napajanje, flashing i serijska dijagnostika
- USB2 / FS Host: DDJ-FLX4 MIDI In/Out i UAC1 slušalice
- USB3 / HS Host: Rekordbox USB Flash / MSC
- GPIO1/2/3: PCM5102A I2S master izlaz
- MIPI-DSI: fizički prihvaćen 800×480 landscape UI na EYOYO
  `DSI506 / DYL0023`; touch adresa `0x38` postoji, ali FT5x06 čitanje i
  koordinate još čekaju hardware acceptance
- ESP32-C6 preko SDIO: Wi-Fi 6 / ESP-Hosted, operatorski SoftAP
  `Pajoniiir-M3` te privremeni APSTA servisni uplink za potpisani P4 OTA, bez
  gašenja lokalnog DHCP/HTTP puta

ESP32-P4 je jedini autoritet za playback, deck/mixer state, USB host klijente,
MIDI mapiranje, LED feedback, audio DSP, biblioteku i UI. `control_link` u P4
izvoru više nije serijski link: to je samo lokalni red semantičkih događaja i
LED callback prema izravnom FLX4 USB hostu.

## Trenutni bench status

Na benchu je app-only display acceptance kandidat `M3-45-g5bb55bc-dirty` u
`factory` particiji, SHA-256
`52A324421F59BA6AA6E48B409FDA286E8BB6AA7086315C7EEF01813DC8DE437E`.
Posljednji potpisani rollback/release baseline ostaje `M3-41-g133f399` u
`ota_0`. FLX4 MIDI In/Out/UAC, USB3 knjižnica od 191 trake, SoftAP
`Pajoniiir-M3`, web kontrola i potpisani OTA rade; Wi-Fi ostaje uključen.

PCM5102A je 2026-08-26 fizički spojen na GPIO1/2/3 i hardverski prihvaćen s
44,1-kHz i 48-kHz izvorima, mixed-rate dual-deck miksom, tihim idle izlazom te
kontroliranim full-master limiter testom bez PCM underruna ili UAC gubitka.
5,0-inčni EYOYO `DSI506 / DYL0023` sada je spojen i fizički prihvaćen za sliku.
Aktivni `bsp_p4_m3` koristi jednu DSI lane na 800 Mbps, RGB888, 27,777 MHz,
HFP/HSW/HBP `59/2/45`, VFP/VSW/VBP `7/2/22` i burst video bez frame ACK-a.
Korisnik je potvrdio ispravne boje, čitljiv nativni landscape, redoslijed GUI-ja
i uklonjeno horizontalno omatanje; privremene testne trake uklonjene su iz
normalnog boota. Touch `0x38` odgovara na I2C scan, ali runtime FT5x06 čitanje
javlja greške, pa touch, Master Tempo/Shift+Browse/Load i zajednički
display/audio/USB/Wi-Fi soak ostaju sljedeći razvojni blok.

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
