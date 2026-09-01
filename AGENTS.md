# AGENTS.md

## Uloga

Djeluj kao senior embedded C / ESP-IDF / real-time audio inženjer za projekt
Pajoniiir-M3. Ovo nije NovaPlayout/Avalonia projekt.

Komunikacija s korisnikom neka bude na hrvatskom jeziku. Kod, nazivi datoteka,
commit poruke, C simboli i tehnička dokumentacija mogu ostati na engleskom ako
je to prirodnije za firmware projekt.

## Projekt

Cilj je standalone dual-deck DJ sustav bez računala (single-chip ESP32-P4):

- **Pioneer DDJ-FLX4** je operator surface spojen izravno na **USB2** (FS USB Host @ 12 Mbps) za MIDI In/Out i UAC1 streaming zvuka u slušalice.
- **Rekordbox USB Flash disk** spojen je izravno na **USB3** (HS USB Host @ 480 Mbps) za brzo čitanje baze i waveform analiza.
- **USB1 (USB-TTL / CH340C)** služi za 5V napajanje cijele ploče, programiranje (flashing) i serijsku dijagnostiku.
- **ESP32-P4 (JC-ESP32P4-M3-DEV)** je autoritativni single-chip host: USB MIDI i Audio host, playback engine, Rekordbox library, 800×480 DSI UI i DSP mixer.
- **5.0" MIPI-DSI IPS zaslon (800×480)** EYOYO `DSI506 / DYL0023` fizički je prihvaćen za sliku i FT5426 touch u nativnom landscape formatu (0° PPA hardware blit). Touch na I2C `0x38` radi na 100 kHz uz `swap_xy=0`, `mirror_x=1`, `mirror_y=1`.
- **Master audio izlaz** ide preko PCM5102A I2S DAC modula (`GPIO1/2/3` na JP1 headeru).
- **Wi-Fi 6** je osiguran preko integriranog **ESP32-C6** modula (ESP-Hosted preko SDIO).
- **Neaktivne periferije**: Ugrađeni mikrofon, NS4150 mono zvučničko pojačalo i RJ45 Ethernet su namjerno isključeni u softveru radi nultog šuma i oslobađanja GPIO pinova.

## Trenutni handoff

Na benchu je app-only UI/Hot Cue kandidat `M3-48-g435bcfe-dirty` u `factory`
particiji, SHA-256
`2CDAB5D7C859F28F26E2BB02CDC5B711DA4A25480EA081B18A2C3EF963DF3455`.
Posljednji potpisani rollback/release baseline je `M3-41-g133f399` u `ota_0`.
FLX4 MIDI In/Out/UAC, USB3 knjižnica od 191 trake, Wi-Fi SoftAP/web kontrola i
potpisani OTA rade. Oba decka su zaustavljena, a Wi-Fi treba ostati uključen
dok se ne završe touch/UI i integration provjere.

PCM5102A je 2026-08-26 hardverski prihvaćen na `M3-41` baselineu. Wiring je
`BCK=GPIO1`, `LCK=GPIO2`, `DIN=GPIO3`, `SCK=GND`, `VIN=5V`, zajednički GND;
konfiguracijski mostovi modula su `H1=L`, `H2=L`, `H3=H`, `H4=L`. L/R, tihi
idle, 44,1/48-kHz switching, mixed-rate dual-deck i full-master limiter prošli
su bez čujnog clippinga, PCM underruna ili UAC drop/overflowa. Dva izolirana
output-late događaja nisu imala audio posljedicu i ostaju za monitoring.

5,0-inčni EYOYO `DSI506 / DYL0023` spojen je na J2 i display-image gate je
zatvoren 2026-08-31. Aktivni `bsp_p4_m3` koristi 1 lane / 800 Mbps, RGB888,
27,777 MHz, HFP/HSW/HBP `59/2/45`, VFP/VSW/VBP `109/2/22` (50,0146 Hz), burst sync pulses i
bez frame ACK-a. Boje, nativni landscape, GUI redoslijed i horizontalno
poravnanje fizički su potvrđeni; non-burst način je odbačen jer je davao
ciklički pomak `70123456`. Ne nagađaj nepoznati bridge/init. FT5426 touch je
2026-08-31 stabiliziran na 100 kHz i korisnik je potvrdio kartice, Backlight
drag i kontrole na obje strane. Shift + Browse force-open, ubrzano pomicanje i
Shift + Load D1/D2 routing prihvaćeni su 2026-09-01. Screensaver wake također
je prihvaćen nakon popravka direct-FLX4 puta: prvi PLAY samo budi UI, drugi
PLAY izvršava naredbu, a touch ne aktivira kontrolu ispod. Corner/edge i
two-finger safety gate također su prihvaćeni bez ghost akcije ili stuck pressa;
LVGL put ostaje namjerno single-pointer i ne tvrdi dva neovisna kursora.
Početni D1/D2 target selektori sada imaju eksplicitno stanje i prije prvog
dodira. Lokalni Hot Cue overlay ima prednost nad Rekordbox ANLZ cueovima i
osvježava se nakon fizičkog set/clear događaja. D1 cue A fizički je prošao set,
shifted clear, nenulti recall i ponovni prikaz nakon reboota/reloada iste trake;
vidi `docs/validation/2026-09-01-hot-cue-ui.md`.
Waveform scanout kandidat prihvaćen je 2026-09-01: update je vezan uz panel
refresh, a kad oba decka sviraju direct-PPA blit uvijek ide odozgo prema dolje.
Korisnik je prvo potvrdio oštar i fluidan solo D1, zatim oba oštra i fluidna
waveforma bez bljeskanja ili audio posljedice. Vidi
`docs/validation/2026-09-01-dsi506-waveform-sync.md`.
Naknadni desetominutni zajednički integration soak prihvaćen je 2026-09-01:
oba 44,1-kHz decka s aktivnim loopovima svirala su svih 600 s uz PCM5102A
master, FLX4 slušalice/UAC, fluidne waveforme, touch/backlight, USB3 knjižnicu
od 191 trake i kontinuirani Wi-Fi API promet. Prošlo je 1840 status pollova
bez API/library/firmware greške; PCM underrun, UAC drop/overflow/underflow i
service-log drop delte ostale su 0. Pet izoliranih output-late događaja,
maksimum 12522 us, nije imalo čujnu ni vizualnu posljedicu i ostaje monitoring
nalaz, pa ovo nije zero-late tvrdnja. Vidi
`docs/validation/2026-09-01-integration-soak.md`. Settings eyes-on/touch i
pregledani 800×480 screenshot gate zatvoreni su 2026-09-01; `settings` i
`settings_restored` daju isti očekivani hash. Produženi cold-power/reconnect
gate također je zatvoren nakon tri cold boota, FLX4 reconnecta pod playbackom,
sigurnog USB3 reconnecta, 300-s dual-deck load testa i završnog PFL smokea.
Vidi `docs/validation/2026-09-01-settings-ui.md` i
`docs/validation/2026-09-01-cold-power-reconnect.md`.

MT test otkrio je da correlation offseti mogu akumulirati pomak zvuka unatoč
ispravnom brojilu položaja. Nominalni grain sada se sidri na integrirani source
clock. Prošli su novi 18-case PCM onset gate, puni host suite, 300-s PC soak i
ESP-IDF build. Detalji:
`docs/validation/2026-08-31-master-tempo-response.md`. API BPM/position sam po
sebi nije dokaz akustičkog tempa. Raniji izolirani FLX4 disconnect i jedan
11997-us output-late događaj ostaju otvoreni za monitoring.
Negativni pitch na timing-only kandidatu naknadno je reproducirao pucketanje,
waveform stutter i IDLE0 WDT (ae_output/PCM reads te ae_decode); povremeni
display flash također je prijavljen. Trenutni cache kandidat iste dirty
verzije razlikuje se SHA-om gore: per-deck 640-frame search cache je u internoj
BSS, ne na output stacku. PC PCM hash i tempo očuvani su uz manje source read
poziva; D2 pri -5,01% sada je potvrđen bez pucketanja uz fluidan waveform.
Solo MT pitch prijelazi sada su potvrđeni na D1 i D2 uz čist zvuk i fluidan
waveform. Prvi zajednički 48/48-kHz pokušaj je odbačen zbog teškog trzanja,
PCM/UAC gubitka i bljeska zaslona. Aktualni SHA dodaje consumer-owned PCM
playhead/read mapiranje nakon reproducirane half-published producer-index
greške te bounded hijerarhijsku correlation pretragu. Puni host suite,
313-assert timeline gate, build i 300-s PC soak prolaze. Kratki fizički
48/48-kHz dual-deck retest s D1 +5 % i D2 -5 % prihvaćen je 2026-09-01:
operator je potvrdio čist zvuk, fluidan waveform i stabilan zaslon, PCM i UAC
drop/overflow delte ostale su 0, a jedan završni output-late od 11024 us nije
imao čujnu ili vizualnu posljedicu. Naknadni 44,1/48-kHz mixed-rate MT test s
istim suprotnim pitch vrijednostima također je prihvaćen: nije bilo novih
output-late događaja ni PCM/UAC gubitka, a operator je potvrdio zvuk, waveform
i zaslon. Ne pripisuj raniji bljesak heap overflowu bez dokaza.

## Najvažnije putanje

```text
<repo-root>
  README.md
  AGENTS.md
  docs\PROJECT_OVERVIEW.md
  docs\ARCHITECTURE.md
  docs\DDJ_FLX4_MIDI_MAP.md
  docs\HARDWARE_WIRING.md
  docs\DISPLAY_DSI506_BRINGUP.md
  docs\DEVELOPMENT_PLAN.md
  docs\STARTUP_CHECKLIST.md
  docs\RISK_REGISTER.md
  docs\DOCUMENTATION_STATUS.md
  docs\OTA-UPDATE.md
  docs\reference\Pioneer-DDJ-FLX4.midi.xml
  firmware\main-deck-p4
  tests
```

Prije većih promjena pročitaj relevantne dokumente iz `docs\` i postojeće
komponente koje diraš. Mixxx XML se smatra provjerenim i autoritativnim izvorom
MIDI adresa za DDJ-FLX4 kontrole (sva dosadašnja mapiranja su se pokazala 100%
točnima). Fizički raw MIDI capture više nije preduvjet za razvoj, te se
preostale kontrole mogu implementirati izravno iz XML reference.

## ESP-IDF okruženje

Obavezna verzija je **ESP-IDF v6.0.2** — ista koju koristi
`.github/workflows/esp-idf-6-migration.yml` (`espressif/idf:v6.0.2`).
Inicijaliziraj ESP-IDF ovako:

```powershell
. C:\Espressif\tools\Microsoft.v6.0.2.PowerShell_profile.ps1
```

Nakon inicijalizacije provjeri (mora javiti `ESP-IDF v6.0.2`):

```powershell
idf.py --version
```

Napomena o putanjama: 6.0.2 je na `C:\Espressif\v6.0.2\esp-idf` — **nije** pod
`.espressif\` ni pod `frameworks\` kao stariji instalati, pa listanje tih
direktorija krivo sugerira da 6.0.2 nije instaliran. Provjeri popis
`*.PowerShell_profile.ps1` u `C:\Espressif\tools`.

Ostali alati:

- Git iz Espressif toolchaina: `C:\Espressif\tools\idf-git\2.44.0\cmd`
- Host-test GCC: `C:\msys64\ucrt64\bin`

Za host testove koji traže `gcc`, ako nije već u `PATH`, **dodaj msys2 na kraj**
putanje, ne na početak:

```powershell
$env:Path = "$env:Path;C:\msys64\ucrt64\bin"
```

Host suite se pokreće i na Windows PowerShellu 5.1 i na PowerShellu 7.

## Build naredbe

P4 firmware:

```powershell
# ESP-IDF 6.0.2 je obavezan - manifesti pinaju idf: "==6.0.2"
. C:\Espressif\tools\Microsoft.v6.0.2.PowerShell_profile.ps1
$repoRoot = git rev-parse --show-toplevel
Set-Location "$repoRoot\firmware\main-deck-p4"
idf.py build
```

Flešanje na razvojnu ploču (COM port npr. COM17):

```powershell
idf.py -p COM17 flash monitor
```

P4 host regresije pokreni preko:

```powershell
$env:Path = "$env:Path;C:\msys64\ucrt64\bin"
.\tests\run_p4_host_tests.ps1
```

To je isti runner koji koristi CI.

Za dugi deterministički dual-deck Master Tempo PC regression koristi:

```powershell
.\tests\audio_keylock_soak\run_audio_keylock_soak.ps1
```

Zadani run simulira pet minuta oba decka i provjerava drift, pitch, DSP
finite-state, velike sample skokove i clipping.

Za headless LVGL navigaciju i točnu screenshot regresiju koristi:

```powershell
.\tests\ui_simulator\run_ui_simulator_e2e.ps1
```

Gate automatski koristi pinani LVGL commit i pokriva Overview D1/D2, Library,
Hot Cues, Settings, screensaver i točnu obnovu Settings ekrana. Baseline
mijenjaj samo nakon vizualnog pregleda snimki uz
`-UpdateBaselines -KeepArtifacts`.

## Git i build artefakti

Repo koristi `.gitignore` za ESP-IDF artefakte:

- `build/`
- `managed_components/`
- `sdkconfig`
- `sdkconfig.old`
- `JC-ESP32P4-M3-DEV/` (dokumentacija i sheme dev ploče)

Iznimka: `firmware/main-deck-p4/dependencies.lock` **jest** commitan (`.gitignore` ima
`!` iznimku) da bi clean build bio reproducibilan. CI provjerava da se
lock nije promijenio tijekom builda.

Ne commitaj generirane build direktorije ili lokalni `sdkconfig` osim ako
korisnik eksplicitno traži drugačije.

Branch prefix za agent promjene je `codex/`.

Canonical repo je **`https://github.com/dvucinozd/Pajoniiir-M3.git`**.

## Arhitektonska pravila

- **Single-Chip ESP32-P4**: ESP32-P4 je autoritativan za playback state, deck state, audio position, mixer state, LED odluke, LVGL UI i USB Host klijente.
- **3-Port USB Topologija**: USB1 za 5V napajanje/debug, USB2 za DDJ-FLX4 (FS), USB3 za Rekordbox MSC (HS). Nema potrebe za vanjskim USB hubom.
- **Master Audio**: Vanjski PCM5102A I2S DAC (`GPIO1/2/3` na JP1 headeru) pruža primarni master audio izlaz.
- **Headphones / Cue**: Izravni UAC1 Isochronous USB audio streaming prema DDJ-FLX4 3.5mm priključku.
- **Mreža**: Isključivo Wi-Fi 6 preko ESP32-C6 modula (ESP-Hosted preko SDIO). Ethernet EMAC je isključen u softveru kako bi se oslobodili RMII pinovi za I2S DAC.
- **MIDI**: Koristi se provjereni Mixxx XML mapping iz `docs/reference/Pioneer-DDJ-FLX4.midi.xml`.

## Verifikacija prije završetka

Prije tvrdnje da je posao gotov:

1. Pokreni relevantnu provjeru (P4 build i/ili host testove).
2. Pročitaj exit code i bitan output.
3. Navedi što je prošlo, a što nije pokrenuto.

Za dokumentacijske promjene minimalno:

```powershell
git diff --check
git status --short
```

Za firmware promjene pokreni `idf.py build` i `.\tests\run_p4_host_tests.ps1`.

## Stil rada

- Koristi `rg` / `rg --files` za pretragu.
- Koristi `apply_patch` ili alate za uređivanje datoteka.
- Ne revertaj korisničke promjene bez izričitog zahtjeva.
- Ako mijenjaš dokumentaciju o fazama, uskladi `README.md`,
  `docs\DEVELOPMENT_PLAN.md` i `docs\STARTUP_CHECKLIST.md` kad je relevantno.
