# P4 Wi-Fi/C6 lifecycle smoke — 2026-08-22

## Opseg

Ovaj zapis pokriva prvi hardverski gate nakon Wi-Fi transition/status
učvršćivanja i promjene operatorskog SoftAP naziva na `Pajoniiir-M3`. Test nije
multi-client, audio/USB stress ni OTA acceptance.

## Konfiguracija

- ploča: JC-ESP32P4-M3-DEV, ESP32-P4 revizija v1.3;
- serijski port: `COM17`;
- ESP-IDF: v6.0.2;
- testna slika: `RC2-73-g8dbca9c-dirty`, factory slot, na bazi commita
  `8dbca9c` i promjena opisanih u ovom zapisu;
- aplikacija: `main-deck-p4.bin` veličine `0x23fd10`, 44% slobodno u najmanjoj
  app particiji;
- ekran/touch: novi 5,0-inčni FT5426 ekran još nije spojen, pa Settings dodirni
  tok nije bio dio testa.

## Početni Wi-Fi smoke

Privremeni NVS s jedinom postavkom `cdjcfg/wifi_rem=1` pokrenuo je stvarni
ESP32-C6/ESP-Hosted SDIO put. Windows sken je pronašao AP, klijent je dobio
`192.168.4.2`, a P4 je bio dostupan na `192.168.4.1`.

Read-only provjere:

| Provjera | Rezultat |
| --- | --- |
| SoftAP beacon | PASS |
| WPA2 association i DHCP | PASS, klijent `192.168.4.2` |
| `GET /api/status` | HTTP 200 |
| `GET /api/firmware` | HTTP 200, factory / `RC2-73-g8dbca9c-dirty` |
| Reset loop, panic ili watchdog | Nije opažen |

## Pronađeni lifecycle problem i popravak

Nakon vraćanja spremljenog Wi-Fi OFF stanja i P4 reseta prvotno pokrenuti AP
ostao je vidljiv. Razlog je hardverski: P4-only reset ne gasi zasebno napajani
C6, a GPIO54 je C6 `EN` linija. NVS je bio ispravno vraćen; C6 je samo nastavio
izvršavati prethodno pokrenuti SoftAP.

`wifi_link` sada konfigurira GPIO54 kao izlaz i drži ga LOW:

- odmah u `wifi_link_init()`, prije odluke o spremljenom ON/OFF stanju;
- nakon `esp_hosted_deinit()` pri svakom punom Wi-Fi teardownu.

`esp_hosted_init()` i dalje preuzima GPIO54 kada operator zatraži Wi-Fi ON, pa
normalni SDIO/C6 bring-up ostaje nepromijenjen.

## Završni SSID i lifecycle rezultat

Nakon builda i flasha popravljene slike:

1. Sa spremljenim OFF stanjem nisu bili vidljivi ni stari `Pajoniiir` ni novi
   `Pajoniiir-M3` AP.
2. S privremenim `wifi_rem=1` bio je vidljiv samo `Pajoniiir-M3`.
3. Klijent se spojio, dobio `192.168.4.2`, a `/api/status` i `/api/firmware`
   ponovno su vratili HTTP 200.
4. Nakon vraćanja originalnog NVS-a oba su AP naziva odmah nestala bez fizičkog
   power-cyclea.
5. Privremeni Windows profil je obrisan, a radna stanica vraćena na početnu
   mrežu.

## NVS integritet

Izvorna NVS particija spremljena je prije testa, a nakon testa ponovno zapisana
i read-back verificirana:

- veličina: 24.576 bajtova;
- izvorni SHA-256:
  `48C626B2D0871CD64320FD7166F1E1BF7DDCE7700C3C05DC613F431BA2A95967`;
- završni read-back SHA-256: isti;
- rezultat: `VERIFIED_IDENTICAL`.

## Software gate

- `tests/run_p4_host_tests.ps1`: PASS;
- ESP-IDF v6.0.2 `idf.py build`: PASS;
- `git diff --check`: PASS;
- dodan je statički regression guard za SSID `Pajoniiir-M3` i C6 EN gašenje.

## Preostalo

- najmanje dva istodobna AP klijenta i Settings client counter;
- konfigurirani AP→STA→AP probe;
- potpisani P4 OTA uspjeh/neuspjeh/rollback;
- paralelni Wi-Fi + USB2 + USB3 + dual-deck audio stress;
- vizualna i touch provjera na novom 5,0-inčnom ekranu nakon njegova dolaska.

## Nastavak provjera s microSD karticom

Isti clean firmware `RC2-74-g017f62e` ponovno je pokrenut nakon što je operator
umetnuo microSD karticu. Boot je potvrdio:

- SDHC karticu veličine 59.688 MB, 4-bit bus i 20 MHz;
- uspješan `SD_MOUNTED` bez mount greške;
- aktivan servisni journal na `/sd`;
- USB Rekordbox library od 191 tracka;
- bez reset loopa, panica ili watchdoga.

### AP reconnect i API

Jedan Windows klijent odradio je tri uzastopna disconnect/reconnect ciklusa na
`Pajoniiir-M3`. U svakom ciklusu:

- AP je ostao vidljiv dok je klijent bio odspojen;
- DHCP je ponovno dodijelio `192.168.4.2`;
- `GET /api/status` vratio je HTTP 200;
- service-log drop counter ostao je 0.

Dodatno su prošli:

| Provjera | Rezultat |
| --- | --- |
| četiri paralelna `GET /api/status` zahtjeva | 4 × HTTP 200 |
| 100 uzastopnih status zahtjeva | 100/100, bez greške |
| 10 uzastopnih library zahtjeva | 10/10, svaki 191 track |
| internal heap | start 158.651 B, min 158.443 B, final 158.647 B |
| service log | dostupan, queue 0, dropped 0, last_error 0 |
| `GET /api/diagnostic-log` | HTTP 200, 54 retka / 3.839 B |

Journal je zabilježio `BOOT`, `SD_MOUNTED`, `WIFI_ENABLE_REQUESTED`,
`WIFI_STARTED`, `USB_MOUNTED` i `LIBRARY_LOADED` bez gubitaka.

### AP→STA→AP preduvjet

`GET /api/ota/config` vratio je prazne `ssid` i `url` vrijednosti te nije
izložio zaporku. Budući da servisna mreža nije konfigurirana, puni AP→STA→AP
round-trip nije pokrenut. Sigurni rejection path je provjeren: POST probe vratio
je HTTP 400 s `no service network or update URL configured`, probe state ostao
je `idle`, a AP i read-only API ostali su dostupni.

### Stvarni web controller

Web sučelje otvoreno je u stvarnom Chromium pregledniku i provjereno preko
operatorskog toka:

- početna stranica, firmware, OTA konfiguracija, status polling i library API
  vratili su HTTP 200;
- USB Browser prikazao je svih 191 trackova;
- pretraga `Darude` izdvojila je `Darude - Sandstorm.mp3`;
- D1 LOAD vratio je HTTP 200, Deck 1 prešao je u `READY` te prikazao 136,00 BPM
  i trajanje 3:52 bez automatskog pokretanja reprodukcije;
- učitana pjesma ostala je prikazana i nakon reloadanja stranice;
- browser konzola nije prijavila JavaScript pogreške; zabilježeno je samo jedno
  upozorenje za nestandardni CSS `appearance: slider-vertical`.

### Završno trajno stanje

Na zahtjev operatora Wi-Fi je ostavljen uključen kako bi web controller služio
za učitavanje traka dok novi ekran ne stigne:

- izvornih šest NVS zapisa, uključujući tri hot cue bloba, mehanički su
  preneseni bez promjene;
- dodan je samo `cdjcfg/wifi_rem=1`;
- generirana NVS slika prošla je integrity provjeru i flash verify;
- read-back je potvrdio iste postavke i hot cueove te `wifi_rem=1`; jedina
  očekivana runtime promjena bio je povećani `svc_log/boot_id`;
- nakon završnog reseta `Pajoniiir-M3` se automatski vratio, Windows profil je
  zadržan, web root i diagnostic log vratili su HTTP 200, a library 191 track;
- originalni OFF NVS backup ostao je lokalno sa SHA-256
  `48C626B2D0871CD64320FD7166F1E1BF7DDCE7700C3C05DC613F431BA2A95967`.

Preostaju pravi test s najmanje dva fizička klijenta, konfigurirani
AP→STA→AP/OTA round-trip i kombinirani Wi-Fi/USB/audio stress.

## Kombinirani FLX4 + USB media + Wi-Fi soak

Nakon priključivanja stvarnog DDJ-FLX4 status je potvrdio `present=true`, MIDI
In/Out i UAC1 USB Audio. Na Deck 1 i Deck 2 sekvencijalno su učitane dvije trake
s USB medija, oba channel fadera spuštena su na nulu i pokrenut je dual-deck
playback. PCM5102A izlaz bio je otvoren na 44,1 kHz, a FLX4 headphone endpoint
primao je četverokanalne UAC1 blokove.

Glavni prozor trajao je 189,5 sekundi i uključio:

| Provjera | Rezultat |
| --- | --- |
| `GET /api/status` | 180/180 |
| `GET /api/library` | 18/18, svaki 191 track |
| `GET /api/firmware` | 12/12 |
| controller/MIDI/UAC prisutnost | bez izgubljenog uzorka |
| dual-deck playback | oba decka napredovala po 189.503 ms |
| PCM output late / deck underrun tijekom prozora | 0 / 0 |
| service log dropped / last error | 0 / 0 |
| internal heap | 109.439 B start, 109.243 B min, 109.523 B final |

FLX4 UAC producer ipak je prijavio 339 `dropped_blocks` tijekom glavnog
prozora. Dodatna izolacija dala je:

| Profil | Rezultat |
| --- | --- |
| 30 s gotovo bez mrežnog prometa | 5.177 predanih blokova, 0 novih dropova |
| 30 s burst prometa | 135 zahtjeva, 0 HTTP grešaka, 5 novih dropova |
| 45 s normalnog 1 Hz status pollinga | 45/45, 7 novih dropova |

Brojač raste kada u 2.048-frame UAC ring ne stane cijeli 128-frame producer
chunk; trenutačno ne objavljuje broj stvarno izgubljenih frameova, ring fill ni
underflow. Rezultat zato nije dovoljan za zatvaranje audio acceptance gatea.
M3 zahvat dodaje tu telemetriju i jednoframe clock-drift regulaciju između
PCM5102A i FLX4 USB audio domena; isti test treba ponoviti na stvarnom M3 buildu.
Pri gotovo istodobnom zaustavljanju oba decka nakon mjernih prozora zabilježen
je još jedan odvojeni tail događaj: jedan output-late od 12.347 µs i 1.792
underrun framea. Nije nastao tijekom 189,5-sekundnog soaka, ali ga treba uključiti
u ponovljeni start/stop acceptance test.

Nakon mjerenja oba decka vraćena su u `READY`, playback je zaustavljen, channel
faderi su ostali na nuli, FLX4 MIDI/UAC ostao je spojen, web root je vratio HTTP
200 i library je i dalje sadržavao 191 track. Wi-Fi i Windows profil
`Pajoniiir-M3` ostavljeni su uključeni.
