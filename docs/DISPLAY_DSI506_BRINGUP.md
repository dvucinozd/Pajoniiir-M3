# DSI-506 / DSI5061 Display Arrival Dossier

Status: priprema prije dolaska hardvera, 2026-08-26. Ovo nije hardware
acceptance ni potvrda panel-controller naredbi.

## Identifikacija

Korisnik je dostavio prodajnu fotografiju 5-inčnog plavog DSI modula i naveo
oznaku `DSI-506`. Fotografija se podudara s proizvodnom obitelji koja se prodaje
kao `DSI5061` odnosno `DSI5061-A`, ali puna oznaka, proizvođač i PCB revizija
moraju se pročitati sa stvarnog primjerka.

Referentna fotografija spremljena je kao
`docs/reference/DSI506_PRODUCT_REFERENCE.jpg` (SHA-256
`DDDC23FE0EC29081B3EA64A9CAB05A954CE2A86C93A8CD8A827E2B763BEFCCC3`).

Trenutno je razumno očekivati:

- 5,0-inčni IPS TFT, 800×480, nativni landscape;
- približno 60 Hz i 108,00×64,80 mm aktivne površine;
- Raspberry Pi-style 15-pin, 1,0-mm MIPI-DSI priključak;
- dvije DSI podatkovne lane;
- kapacitivni touch, vjerojatno FT5426/FT5x06 na I2C adresi `0x38`;
- tipku `Backlight` i zasebne `PWM/GND` padove za opcionalnu regulaciju;
- 3,3-V logiku/napajanje kroz DSI priključak.

Ovo još nije dovoljno za potvrdu DSI bridgea, video formata, porch timinga,
touch INT/RESET izvedbe, FFC orijentacije ili backlight polariteta.

Referentni izvori, provjereni 2026-08-26:

- Elecrow `DSI05379I` proizvod iste vidljive PCB obitelji:
  <https://www.elecrow.com/5-inch-dsi-display-ips-800-480-touch-screen-compatible-with-raspberry-pi-4b-3b-3b.html>
- Elecrow V1.4 user manual:
  <https://www.elecrow.com/download/product/DSI05379I/5inch-dsi-display_user_manual-v1.4.pdf>
- DSI5061 listing s istim specifikacijama i priborom:
  <https://eckstein-shop.de/50-Zoll-800x480-MIPI-DSI-IPS-Display-fuer-Raspberry-Pi-Kapazitiver-Touch-EN>

Dobavljačka dokumentacija navodi 3,3 V i približno 340 mA. Prodajna stranica na
jednom mjestu pogrešno ispisuje 3400 mA; manual i ostali zapisi navode 340 mA.
Desni `3V3/GND` header na fotografiji opisan je kao izlaz za mali 3,3-V
ventilator i ne smije se tretirati kao ulaz napajanja zaslona bez provjere.

## J2 kandidat pinout

Postojeći `bsp_p4_m3` dokumentira JC-ESP32P4-M3-DEV J2 ovako:

| J2 pin | Signal |
|---|---|
| 1 | GND |
| 2 | DSI data lane 1 N |
| 3 | DSI data lane 1 P |
| 4 | GND |
| 5 | DSI clock N |
| 6 | DSI clock P |
| 7 | GND |
| 8 | DSI data lane 0 N |
| 9 | DSI data lane 0 P |
| 10 | GND |
| 11 | touch I2C SCL |
| 12 | touch I2C SDA |
| 13 | GND |
| 14 | +3,3 V |
| 15 | +3,3 V |

Raspored odgovara standardnom Raspberry Pi 15-pin DSI konektoru, ali to ne
potvrđuje smjer kontakata priloženog FFC-a. Prije umetanja treba identificirati
pin 1, stranu izloženih kontakata i treba li kabel s kontaktima na istoj ili
suprotnoj strani. Kabel se umeće samo uz isključeno napajanje.

## Backlight

DSI 15-pin ne nosi zaseban PWM signal. Modul ima vlastitu `Backlight` tipku i
`PWM/GND` padove. Za prvi light-up ostaviti tvornički KEY/fixed-brightness način
i ne lemiti PWM vod. Tek nakon stabilne slike treba multimetrom i dokumentacijom
potvrditi polaritet/razinu, zatim po potrebi spojiti slobodan P4 PWM GPIO na
`PWM` i zajednički GND.

Postojeći `bsp_p4_m3` generira 5-kHz aktivno-visoki PWM na GPIO23, ali taj GPIO
nije dio J2 15-pin konektora. Settings slider zato neće upravljati ovim modulom
dok se fizički PWM put zasebno ne potvrdi i spoji.

## Stanje firmwarea prije dolaska

Pripremljena, ali još neaktivna komponenta:

- `firmware/main-deck-p4/components/bsp_p4_m3/bsp_p4_m3.c`
- `firmware/main-deck-p4/components/bsp_p4_m3/include/bsp_p4_m3.h`
- `firmware/main-deck-p4/components/bsp_p4_m3/include/bsp_jc4880.h`

Ona trenutno pretpostavlja:

- dvije DSI lane na 500 Mbps;
- DPI clock 30 MHz;
- H timing `40/40/40` za sync/back/front porch;
- V timing `9/29/13`;
- 800×480, RGB565 i jedan framebuffer;
- čisti DPI video bez vendor DBI/DCS init sekvence;
- FT5x06 driver na `0x38`, GPIO7/8, bez zasebnih INT/RESET pinova;
- nativni landscape bez swap/mirror transformacije;
- GPIO23 kao 5-kHz backlight PWM.

UI backend već ima nativni 800×480 put s `PPA_SRM_ROTATION_ANGLE_0`, a
`dependencies.lock` već sadrži `esp_lcd_touch_ft5x06`.

Aktualni build ipak još zahtijeva i kompilira stari `bsp_jc4880` za
ST7701S/GT911. `bsp_p4_m3` nije aktivan samo zato što postoji u stablu. Prvi
display commit mora kontrolirano prebaciti component dependency s
`bsp_jc4880` na `bsp_p4_m3` najmanje u:

- `firmware/main-deck-p4/main/CMakeLists.txt`;
- `firmware/main-deck-p4/components/audio_engine/CMakeLists.txt`;
- `firmware/main-deck-p4/components/ui/CMakeLists.txt`;
- statičkim BSP provjerama u `tests/run_p4_host_tests.ps1`.

Postojeći `bsp_jc4880.h` compatibility redirect unutar novog BSP-a dopušta da se
C includeovi migriraju odvojeno. Stari BSP ne uklanjati dok novi panel, touch,
PCM5102A, microSD i C6/SDIO ne prođu zajednički acceptance.

## Što fotografirati odmah po dolasku

Bez napajanja snimiti:

1. cijelu prednju i stražnju stranu ravno odozgo;
2. punu `DSI-506...` oznaku, sufiks i `REV`;
3. oznake oba veća IC-a i svih naljepnica;
4. oznaku na centralnom panel FPC-u;
5. DSI konektor izbliza, s vidljivim pinom 1 i položajem zasuna;
6. oba kraja svakog priloženog FFC kabela, jedan pokraj drugoga;
7. označena IPS/touch polja na PCB-u, ako su popunjena;
8. `Backlight`, selector/0R, `PWM/GND` i `3V3/GND` područja;
9. pakiranje, SKU i eventualni priloženi list/pinout.

Zapisati broj pinova, pitch, jesu li kontakti FFC-a same-side/opposite-side te
izmjeriti koji su pinovi GND. Ne raditi continuity mjerenje preko DSI parova uz
napajanje.

## Sigurni redoslijed bring-upa

### 0. Priprema builda

1. Sačuvati poznati `M3-41-g133f399 / ota_0` rollback baseline.
2. Prebaciti dependencyje i statičke testove na `bsp_p4_m3` u zasebnom commitu.
3. Pokrenuti puni host suite i clean ESP-IDF 6.0.2 build.
4. Prvi display kandidat instalirati preko USB1/serijskog flashing puta, ne
   isključivo OTA-om, jer pogrešan rani display init može spriječiti Wi-Fi boot.

### 1. Prvi light-up

1. Isključiti napajanje prije umetanja FFC-a.
2. Potvrditi pin 1 i orijentaciju kontakata na oba konektora.
3. Ostaviti vanjski PWM nepovezan i koristiti tvornički backlight način.
4. Stišati vanjsko audio pojačalo; oba decka moraju biti zaustavljena.
5. Prvi boot promatrati na serijskom monitoru zbog brownouta, reseta, DSI/PSRAM
   grešaka ili neuobičajenog grijanja.
6. Potvrditi puni kadar najprije statičkim bijelim/crnim/crvenim/zelenim/plavim
   uzorkom, zatim LVGL UI-em.

Ako postoji backlight bez slike, ne mijenjati nasumično timing. Prvo pročitati
IC oznake, skenirati J2 I2C sabirnicu i utvrditi treba li DSI bridgeu I2C ili
DBI/DCS inicijalizacija. Ako postoji slika s krivim bojama, provjeriti očekuje li
bridge RGB565 ili RGB888 prije promjene cijelog UI framebuffera.

### 2. Touch

1. Napraviti I2C scan GPIO7/8 i zapisati sve adrese.
2. Potvrditi očekivani `0x38` prije vezivanja FT5x06 drivera.
3. Provjeriti sva četiri kuta, centar i rubove.
4. Utvrditi `swap_xy`, `mirror_x`, `mirror_y` iz mjerenja, ne iz fotografije.
5. Provjeriti press/release, drag i najmanje dva istodobna dodira ako controller
   i UI to podržavaju.

### 3. UI i Master Tempo

1. Proći Overview D1/D2, Library, Hot Cues, Settings i screensaver.
2. Potvrditi Shift + Browse/Load tok s vidljivim Library ekranom.
3. Provjeriti backlight slider tek nakon fizičkog PWM rješenja.
4. Uključiti Master Tempo kroz UI i odraditi dual-deck keylock test sa suprotnim
   pitch vrijednostima uz nadzor PSRAM-a i output deadlinea.
5. Vizualno pregledati snimke prije promjene screenshot baselinea.

### 4. Zajednički soak

Istodobno držati aktivnima DSI/touch/UI, PCM5102A master, FLX4 MIDI/UAC,
USB3 library/playback i Wi-Fi. Acceptance traži stabilnu sliku i touch, bez DSI
underruna, brownouta, reseta, PCM underruna, UAC drop/overflowa ili servisnog
log dropa.

## Obvezne provjere

```powershell
. C:\Espressif\tools\Microsoft.v6.0.2.PowerShell_profile.ps1
idf.py --version
$repoRoot = git rev-parse --show-toplevel
Set-Location "$repoRoot\firmware\main-deck-p4"
idf.py fullclean
idf.py build
Set-Location $repoRoot
$env:Path = "$env:Path;C:\msys64\ucrt64\bin"
.\tests\run_p4_host_tests.ps1
.\tests\ui_simulator\run_ui_simulator_e2e.ps1
```

Nakon vizualnog hardware acceptancea ponoviti build bez `-dirty`, instalirati
potpisani image, odraditi startup checklist, ažurirati aktivnu dokumentaciju i
tek tada zatvoriti display gate.

## Acceptance zapis koji treba popuniti

```text
PCB/model/revision:
Panel FPC oznaka:
DSI bridge IC:
Touch IC i I2C adresa:
FFC tip i orijentacija:
Napajanje i izmjerena struja:
DSI lane count/rate:
Pixel format i timing:
Backlight način/PWM polaritet:
Touch transformacija:
Cold/warm boot rezultat:
UI/Master Tempo rezultat:
DSI/PCM/UAC/service counter delte:
Prihvaćeni firmware commit/version/slot:
```
