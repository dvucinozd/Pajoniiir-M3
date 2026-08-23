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

Preostaju pravi test s najmanje dva fizička klijenta i konfigurirani
AP→STA→AP/OTA round-trip. Osnovni kombinirani Wi-Fi/USB/audio stress zatvoren je
M3 mjerenjem niže; duži i reconnect stress ostaju release gate.

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

Brojač je rastao kada u 2.048-frame UAC ring nije stao cijeli 128-frame producer
chunk. Rezultat nije bio dovoljan za zatvaranje audio acceptance gatea, pa je M3
zahvat dodao broj stvarno izgubljenih frameova, ring fill/high-water,
underflow/overflow i trim/duplicate telemetriju te jednoframe clock-drift
regulaciju između PCM5102A i FLX4 USB audio domena.
Pri gotovo istodobnom zaustavljanju oba decka nakon mjernih prozora zabilježen
je još jedan odvojeni tail događaj: jedan output-late od 12.347 µs i 1.792
underrun framea. Nije nastao tijekom 189,5-sekundnog soaka, ali ga treba uključiti
u ponovljeni start/stop acceptance test.

Nakon mjerenja oba decka vraćena su u `READY`, playback je zaustavljen, channel
faderi su ostali na nuli, FLX4 MIDI/UAC ostao je spojen, web root je vratio HTTP
200 i library je i dalje sadržavao 191 track. Wi-Fi i Windows profil
`Pajoniiir-M3` ostavljeni su uključeni.

## M3 UAC A/B acceptance

Clean tag build `M3` (`1f9ad7c`) izgrađen je s ESP-IDF v6.0.2, imao je binarnu
veličinu `0x240040` i nakon flasha se prijavio kao factory / `M3`. Wi-Fi se nakon
reseta automatski vratio, FLX4 je prijavio MIDI In/Out i UAC, a USB library je
ostao na 191 tracku. Nova `/api/status` telemetrija bila je prisutna.

Prvi 180-sekundni dual-deck/Wi-Fi run prošao je svih 180 status, 18 library i 12
firmware zahtjeva bez HTTP greške. Oba decka napredovala su po 181.894 ms, bez
controller disconnecta, output-latea, PCM underruna ili aktivnog UAC underflowa.
Ipak je zabilježeno 60 `dropped_blocks` i 6.757 `overflow_frames`; ring je dosegao
2.048/2.048. Jednoframe regulator ispravno je obradio stabilni clock drift, ali
nije mogao spriječiti burst punjenje dok je zajednički FLX4 USB event task na
prioritetu 5 čekao ispod audio output producenta na prioritetu 6.

FLX4 USB event task zato je podignut na prioritet 7 i preimenovan u `flx4_usb`.
Host regresije i ESP-IDF build prošli su prije flasha, a popravljena slika
prijavila se kao `M3-1-g243e996`.

Identičan ponovljeni test dao je:

| Provjera | Rezultat |
| --- | --- |
| stvarno trajanje mjernog prozora | 183,724 s |
| `GET /api/status` | 179/180, jedan izolirani timeout |
| `GET /api/library` | 18/18, svaki 191 track |
| `GET /api/firmware` | 12/12 |
| dual-deck napredak | D1/D2 po 183.757 ms |
| FLX4 MIDI In/Out i UAC | spojeni na završnom uzorku |
| submitted blocks/frames | 31.655 / 8.103.428 |
| dropped blocks / overflow frames | **0 / 0** |
| aktivni underflow frames | **0** |
| ring start/final/high-water | 1.253 / 1.394 / 1.640 od 2.048 frameova |
| clock trim / duplicate | 295 / 43 framea |
| output late / D1 underrun / D2 underrun | **0 / 0 / 0** |
| heap free start/final | 25.308.036 / 25.308.056 B |
| internal free start/final | 109.371 / 109.391 B |

Nakon zaustavljanja oba decka nije nastao prijašnji tail output-late/underrun:
oba su decka ostala `READY`, FLX4 MIDI/UAC spojen, web API dostupan, a library na
191 tracku. Channel faderi ostali su na nuli. Wi-Fi i Windows profil
`Pajoniiir-M3` namjerno su ostavljeni uključeni za daljnje učitavanje traka.

Ovim je osnovni paralelni USB2 FLX4 + USB3 medij + dual-deck + Wi-Fi audio gate
prošao. Jedan izolirani HTTP timeout nije imao audio ni USB posljedicu; fizički
multi-client, AP→STA→AP/OTA, dugi soak i reconnect/medij stress ostaju otvoreni.

## FLX4 hot-plug tijekom playbacka

Prvi fizički unplug/replug test na `M3-1-g243e996` otkrio je stvarni lifecycle
kvar. Status je odmah prijavio FLX4 disconnect, Deck 2 je nastavio playback,
Wi-Fi API je ostao dostupan i USB3 library je ostao na 191 tracku, ali se MIDI i
UAC nisu vratili ni nakon 120 sekundi. P4 reset odmah je ponovno enumerirao isti
FLX4 (`2B73:0045`) i uspješno otvorio Audio Streaming interface 1 / alt 2 te MIDI
Streaming interface 4, čime su kabel, napajanje i descriptor izbori isključeni
kao uzrok.

Uzrok je bio inline poziv `usb_host_interface_release()` i
`usb_host_device_close()` iz `USB_HOST_CLIENT_EVENT_DEV_GONE` callbacka. U tom
trenutku otkazani MIDI i UAC URB-ovi još nisu stigli odraditi completion
callbackove, release je vraćao `ESP_ERR_INVALID_STATE`, a rezultat se ignorirao.
Stari device handle zato je ostao otvoren i blokirao novu enumeraciju.

Commit `4613c4a` (`fix(usb): recover FLX4 after hotplug`) premjestio je cleanup u
FLX4 USB client task nakon `usb_host_client_handle_events()`. Interfacei se sada
otpuštaju tek nakon što se pending URB-ovi povuku, a device handle zatvara se tek
nakon uspješnog releasea svih claimed interfacea. Dodan je statički regression
gate koji zabranjuje povratak release/close poziva u `DEV_GONE` callback.

Čisti ESP-IDF v6.0.2 build prijavio se kao `M3-2-g4613c4a`, veličine
`0x2402b0`, te je flashan na COM17. Nakon boota status je potvrdio FLX4 MIDI
In/Out + UAC, USB library od 191 tracka i aktivan `Pajoniiir-M3` SoftAP.

Deck 2 učitan je s USB3 medija, pozicioniran na približno 30 s, channel volume
postavljen je na nulu i pokrenut je playback. Fizički USB2 unplug/replug dao je:

| Provjera | Rezultat |
| --- | --- |
| disconnect status | detektiran; MIDI In/Out i UAC prešli na false |
| playback tijekom disconnecta | ostao aktivan, bez PCM underruna |
| puni reconnect | približno 6,1 s između status prijelaza |
| MIDI In / MIDI Out / UAC nakon reconnecta | true / true / true |
| Wi-Fi API greške tijekom prijelaza | 0 |
| USB3 library nakon reconnecta | generation 1, 191 track |
| LED resync | operator potvrdio vraćen PLAY LED za aktivni Deck 2 |
| output-late tijekom unplug/replug prijelaza | +2; nakon reconnecta nije rastao |

U sljedećem 30-sekundnom stabilizacijskom prozoru Deck 2 napredovao je s
127.872 ms na 157.936 ms. UAC je predao dodatnih 5.179 blokova / 1.325.790
frameova; `dropped_blocks` i `overflow_frames` ostali su 0, a aktivni
`underflow_frames` ostao je nepromijenjen na 1.234. Output-late ostao je 5,
Deck 2 PCM underrun 0, service-log dropped 0 i FLX4 je ostao potpuno spojen.
Nakon mjerenja Deck 2 vraćen je u `READY`. Wi-Fi i Windows profil
`Pajoniiir-M3` namjerno su ostavljeni uključeni.

Time su zatvoreni osnovni FLX4 descriptor, hot-plug i LED snapshot gateovi.
Duži ponavljani reconnect stress i analiza dvaju deadline promašaja tijekom
samog USB prijelaza ostaju dio dual-USB/audio stress faze.

## Stanje na kraju sesije

Uređaj je ostavljen na verificiranom firmwareu `M3-2-g4613c4a`. Deck 1 je
`IDLE`, Deck 2 `READY`, playback je zaustavljen, FLX4 prijavljuje MIDI In/Out i
UAC, USB3 library ima 191 track, a service-log dropped counter je 0. Wi-Fi radio,
SoftAP i spremljeni Windows profil `Pajoniiir-M3` namjerno ostaju uključeni za
sljedeću sesiju i učitavanje traka do dolaska zaslona.

Sljedeći acceptance blok počinje ponovljenim FLX4 hot-plug ciklusima i mjerenjem
output deadlinea tijekom samog prijelaza. Nakon toga slijede fizički AP
multi-client te AP→STA→AP/potpisani OTA test kada servisna mreža bude
konfigurirana. Generirani lokalni browser artefakti nisu dio izvornog koda ni
release dokumentacije.

## Fizički multi-client acceptance — `M3-15-g70a082c`, 2026-08-23

Završni multi-client gate koristio je dva stvarna SoftAP klijenta: Windows
računalo i mobitel. Na USB2 je cijelo vrijeme bio DDJ-FLX4 s MIDI In/Out i
UAC-om, na USB3 Rekordbox medij s 191 trackom, a oba channel fadera i oba PFL-a
bili su ugašeni. Deck 1 je reproducirao 44,1-kHz `Evelyn Thomas - High Energy`,
Deck 2 48-kHz `Megatron Man`, uz zajednički PCM5102A output na 48 kHz.

Mobitel je otvorio `http://192.168.4.1`, prikazao biblioteku od 191 tracka i oba
decka u PLAY stanju dok je računalo istodobno slalo status, library i firmware
zahtjeve. Zasebni fizički disconnect/reconnect mobitela vratio je isti web
controller bez prekida API-ja prvog klijenta, FLX4-a ili playbacka.

Početni `M3-13-gc95bd4b` prozor funkcionalno je prošao, ali serijski trag je
otkrio da priority-7 FLX4 isochronous callback još svake 1.000 uspješnih
transfera emitira WARN `FLX4 ISOC audio alive`. U kontrolnom 127,061-sekundnom
prozoru svi 120 status, 12 library i 8 firmware zahtjeva prošli su bez greške,
PCM/UAC brojači nisu rasli, a output-late delta bila je 0. U zasebnom
75-uzoračnom phone reconnect prozoru nastao je jedan izolirani output-late od
11.494 µs, bez PCM ili UAC posljedice. Periodični success log i njegova dva
privatna brojača zato su uklonjeni iz real-time puta; submit-failure log ostaje.
Statički host gate sprječava povratak tog loga.

Točan commit `70a082c` izgrađen je s ESP-IDF v6.0.2 kao
`M3-15-g70a082c`, veličine `0x240700`, uz 44% slobodne app particije. Puni
`tests/run_p4_host_tests.ps1`, clean commit build i flash na `COM17` prošli su;
svi flash hashovi su verificirani.

Završni A/B prozor trajao je 190,722 s. Početni start tranzijenti (output-late
1 / 10.925 µs i PCM D1=202) nastali su prije mjernog prozora i ostali
nepromijenjeni do kraja. Rezultat prozora:

| Provjera | Rezultat |
| --- | --- |
| `GET /api/status` | 180/180 |
| `GET /api/library` | 18/18, svaki 191 track |
| `GET /api/firmware` | 12/12, svaki `M3-15-g70a082c` |
| FLX4 missing uzorci | 0/180 |
| napredak D1/D2 | +190.832 / +190.832 ms |
| output-late delta | 0 |
| PCM underrun delta D1/D2 | 0/0 |
| UAC dropped/overflow/aktivni underflow delta | 0/0/0 |
| UAC ring početak/kraj/opaženi maksimum | 1.141/1.338/1.338 od 2.048 frameova |
| UAC clock trim/duplicate delta | +158/+2 framea |
| service-log dropped delta | 0 |
| heap/internal/PSRAM delta | -2.968/+104/-3.072 B |

Korisnik je tijekom prozora kratko fizički pomaknuo `HEADPHONES LEVEL`; gain
ramp je ostao bez output-latea, PCM/UAC gubitka ili prekida playbacka. Time je
funkcionalni fizički two-client + USB2 + USB3 + mixed-rate audio gate zatvoren.
Settings brojač AP klijenata programski je host-testiran, ali njegova vizualna
provjera ostaje dio 800×480 zaslonskog bring-upa jer ciljani zaslon još nije
priključen.

### Naknadna izolacija start/seek tranzijenta

Predmjerni PCM D1=202 događaj iz završnog A/B runa pokušao se ponoviti bez
promjene `M3-15-g70a082c` firmwarea. Sedam no-seek provjera obuhvatilo je četiri
redoslijeda dual-deck starta na istom bootu i tri zasebna cold boota. Dodatnih
pet ciklusa ponovilo je točan sumnjivi slijed: load oba decka, paused seek oba
decka na 100.000 ms, simultaneous start, šest sekundi reprodukcije i stop.

Svih 12 ciklusa završilo je bez PCM underruna, UAC dropped frameova ili UAC ring
overflowa. U dva od pet paused-seek ciklusa output-late brojač porastao je za
jedan pri startu, bez PCM/UAC posljedice; preostala tri ciklusa nisu imala ni
late događaj. Početni D1=202 rezultat zato ostaje izolirani, nereproducirani
prijelaz. Nije opravdana promjena audio enginea bez reproducibilnog uzroka, ali
PCM i output-late telemetrija ostaju obavezni u sljedećem dugom dual-deck soaku.

Završno stanje: oba decka su `READY`, channel faderi su na nuli, PFL je
isključen, FLX4 ima MIDI In/Out i UAC, library sadrži 191 track, service-log
dropped je 0, a `Pajoniiir-M3` Wi-Fi ostavljen je uključen.

## Servisni APSTA i HTTPS channel acceptance — `M3-20-g9f24b19`, 2026-08-23

Servisni SSID, zaporka i update URL uneseni su kroz web UI i trajno spremljeni
u NVS. `GET /api/ota/config` vratio je servisni SSID, HTTPS URL i
`has_password=true`, bez čitanja ili izlaganja same zaporke.

Prvi fizički probe na starijem AP→STA→AP putu reproducibilno je vratio SoftAP
beacon, ali ne i klijentski podatkovni put: Windows nije dobio DHCP lease i
završio je na `169.254.x.x`. Bounded provjera lokalnog DHCP servera nije bila
dovoljna jer je P4 netif prijavljivao `STARTED` dok remote C6/SDIO put nije
prenosio klijentske frameove. Pokušaj punog ESP-Hosted restarta na
`M3-18-g0484fa5` pokazao je i zaseban hardverski uvjet: microSD već koristi drugi
slot istog SDMMC kontrolera, pa Hosted teardown/reinit završava konfliktom
registriranog host kontrolera i assertom. `M3-19-g9360ae6` zato je očuvao Hosted,
ali warm AP stop/start i dalje nije obnovio podatkovni put.

`M3-20-g9f24b19` uklanja taj warm stop/start iz servisnog toka. SoftAP netif,
DHCP server, HTTP/DNS servisi i ESP-Hosted ostaju cijelo vrijeme aktivni, a za
servisnu mrežu privremeno se stvara STA netif i koristi `WIFI_MODE_APSTA`.
Povratak uklanja STA netif i vraća `WIFI_MODE_AP`, bez rušenja zajedničkog SDMMC
transporta.

Hardware rezultat:

| Provjera | Rezultat |
| --- | --- |
| clean firmware | `M3-20-g9f24b19`, `0x240a80`, 44% app particije slobodno |
| flash | `COM17`, svi hashovi verificirani |
| početni SoftAP DHCP/API | `192.168.4.2`, firmware API prikazuje točnu verziju |
| NVS servisna konfiguracija | SSID + HTTPS URL + `has_password=true`; zaporka nije izložena |
| connectivity probe | PASS, `round trip complete` |
| privremena STA adresa | `192.168.0.210` |
| lokalni AP klijent nakon probea | ostao na valjanoj `192.168.4.2`, bez link-local fallbacka |
| HTTPS OTA channel check | PASS; stariji release ispravno odbijen bez downloada/flashanja |
| FLX4 nakon dvaju APSTA posjeta | present, MIDI In/Out i UAC true |
| USB3 library / service log | 191 track / dropped 0, last_error 0 |
| host regresije / ESP-IDF build | PASS / PASS, ESP-IDF v6.0.2 |

Test je namjerno završio bez OTA instalacije jer kanal nije nudio noviju verziju.
Preostaju objava novijeg potpisanog M3 testnog bundlea, uspješni install/reboot,
neuspjeli-download i rollback scenariji te ponavljanje APSTA/OTA prijelaza pod
aktivnim dual-deck audio opterećenjem. Wi-Fi i Windows profil `Pajoniiir-M3`
ostavljeni su uključeni.
