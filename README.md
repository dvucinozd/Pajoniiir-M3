# Pajoniiir-M3

Standalone dual-deck DJ sustav na jednom ESP32-P4 čipu. Projekt izravno spaja
Pioneer DDJ-FLX4, Rekordbox USB medij, 800×480 MIPI-DSI zaslon, PCM5102A master
DAC i ESP32-C6 Wi-Fi modul, bez računala i bez pomoćnog kontrolnog MCU-a.

## Hardverska topologija

- USB1 / CH340C: 5 V napajanje, flashing i serijska dijagnostika
- USB2 / FS Host: DDJ-FLX4 MIDI In/Out i UAC1 slušalice
- USB3 / HS Host: Rekordbox USB Flash / MSC
- GPIO1/2/3: PCM5102A I2S master izlaz
- MIPI-DSI + FT5426: 800×480 landscape UI i kapacitivni dodir
- ESP32-C6 preko SDIO: Wi-Fi 6 / ESP-Hosted, operatorski SoftAP
  `Pajoniiir-M3` te privremeni APSTA servisni uplink za potpisani P4 OTA, bez
  gašenja lokalnog DHCP/HTTP puta

ESP32-P4 je jedini autoritet za playback, deck/mixer state, USB host klijente,
MIDI mapiranje, LED feedback, audio DSP, biblioteku i UI. `control_link` u P4
izvoru više nije serijski link: to je samo lokalni red semantičkih događaja i
LED callback prema izravnom FLX4 USB hostu.

Hardverski prihvaćeni baseline 2026-08-24 je `M3-39-g3bc04fd`: FLX4 MIDI,
LED/UAC, USB3 biblioteka, modalni jog Loop Adjust, deck-local Quantize, prvi
Wi-Fi remote događaj nakon screensavera te shifted Censor, Sync Master i
Reloop Stop/Forget potvrđeni su na stvarnom uređaju. Shifted Beat FX beat-size,
FLANGER, DELAY i reset, jednobeatni CUE/LOOP CALL skokovi te inertni shifted
Smart helperi također su prošli hardverski acceptance. Taj instalirani baseline
još koristi povijesni seek-based Censor.

Sljedeći source kandidat zamjenjuje ga pravim gapless slip-reverse DSP-om:
reverse čita zadržanu povijest iz postojećeg četverosekundnog PCM timelinea,
normalni playhead i dekoder nastavljaju naprijed u pozadini, a otpuštanje radi
10-ms crossfade bez seeka ili dodatnog PCM buffera. Reverse, mixed-rate
interpolacija, bounded-history rub, release i deck-core no-seek ugovor prošli su
puni host suite i ESP-IDF 6.0.2 P4 build; hardverski acceptance još slijedi.

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
- [DDJ-FLX4 MIDI mapa](docs/DDJ_FLX4_MIDI_MAP.md)
- [Plan razvoja](docs/DEVELOPMENT_PLAN.md)
- [Startup checklist](docs/STARTUP_CHECKLIST.md)
- [Registar rizika](docs/RISK_REGISTER.md)
- [OTA ažuriranje](docs/OTA-UPDATE.md)
- [Status dokumentacije](docs/DOCUMENTATION_STATUS.md)

Povijesni dokumenti u `docs/validation`, `docs/superpowers/specs` i
`docs/migration` bilježe ranije izvedbe i nisu opis važeće topologije.
