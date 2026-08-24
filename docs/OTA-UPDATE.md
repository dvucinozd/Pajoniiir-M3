# P4 OTA Update

Status: važeći P4-only postupak, hardverski potvrđen 2026-08-24.

Projekt proizvodi samo jedan OTA target: `main-deck-p4` za chip ID `0x0012`.
Bundle mora biti `.ddjota`, potpisan pouzdanim ECDSA P-256 ključem, s ispravnim
projektom, chip ID-em, verzijom, veličinom i SHA-256 sažetkom.

## Pakiranje

Nakon clean P4 builda:

```powershell
.\tools\package_ota_release.ps1 -BuildName build
```

Privatni ključ nije u repozitoriju. Firmware sadrži samo javni ključ i prihvaća
`rel-001` key ID.

## Instalacija

- Web upload: odaberi potpisani P4 `.ddjota` u Maintenance stranici.
- Pull OTA: postavi servisni Wi-Fi i base URL, provjeri dostupnu verziju te
  potvrdi install.

Playback se zaustavlja prije pisanja flasha. Ne prekidaj napajanje dok se P4 ne
restarta. Novi pending image mora proći firmware-health potvrdu; u suprotnom se
bootloader vraća na prethodnu valjanu particiju.

Za pull OTA kanal objavljuje `latest.json` na base URL-u, a relativni `url`
iz manifesta pokazuje na potpisani bundle, primjerice:

```text
/ota/latest.json
/ota/M3-22-gd7466ea/main-deck-p4.ddjota
```

Prije objave provjeri veličinu i SHA-256 bundlea te njegov potpis javnim
release ključem. Privatni ključ ne učitava se na uređaj ni u repozitorij.

## Verifikacija

- odbij krivi potpis, target, chip, projekt, hash i preveliku sliku;
- potvrdi running version/slot kroz `/api/firmware`;
- nakon restarta provjeri UI, USB2 FLX4, USB3 storage i oba audio izlaza.

## Hardware acceptance M3-34, 2026-08-24

Clean ESP-IDF 6.0.2 build `M3-34-gafee129` zapakiran je ključem `rel-001` i
lokalno instaliran preko `POST /api/ota/p4`. Unutarnji image imao je 2.364.192 B
i SHA-256 `b4b8a0d375a933586a6264a7a816ba1f3f17a24eb8c351d680208641d7500eb2`.
Uređaj je podigao `ota_1`, vratio OTA u `idle`, SoftAP `Pajoniiir-M3`, FLX4
MIDI In/Out/UAC i USB3 biblioteku od 191 trake. Image dodaje statusnu
observability za `sync_master` i `censor_active` te host pokrivanje shifted
Censor, Sync Master i Reloop Stop/Forget adresa na oba decka.

Na hardveru je D1 Sync Master postavljen držanjem BEAT SYNC najmanje 3 s, a
obični D2 Sync zatim je pratio D1 master. D1 `SHIFT + RELOOP/EXIT` zaustavio je
i zaboravio aktivnu petlju. D1 Censor state, LED i čujno kratko ponavljanje
prošli su, a release se vratio na kontinuirano napredujuću vremensku liniju.
Trenutačni seek-based Censor nije gapless: press i release dodali su po jedan
output-late događaj i 256 D1 PCM-underrun frameova. Kontrolni start, 8 s
playbacka i stop bez Censora dodali su 0/0; UAC dropped/overflow i service-log
dropped ostali su 0.

Na istom imageu naknadno je hardverski zatvoren shifted Beat FX blok. Beat-size
je prošao dvostruke korake i obje saturacije od `1/4` do `4 beats`. FLANGER
state, ON LED i čujan sweep potvrđeni su, a live prijelaz na DELAY proizveo je
očekivani one-shot tap od 470 ms. `SHIFT + BEAT FX ON/OFF` vratio je točno
`FILTER / 1 beat / target 1&2 / depth 64 / OFF`, ugasio DELAY DSP i fizičku
LED. Sam live FX prozor imao je jedan izolirani output-late od 14.714 us bez
PCM underruna ili UAC drop/overflowa; service-log dropped ostao je 0.

Dodatni screen-independent MIDI smoke potvrdio je D1 jednobeatne
`SHIFT + CUE/LOOP CALL < / >` skokove po ANLZ gridu
(`30000→29574→30058 ms`) bez dvostrukog release skoka ili counter delte.
`SHIFT + SMART CFX/FADER` ostali su namjerno inertni: normalni state i fizičke
LED-ice ostali su OFF.

## Hardware acceptance M3-32, 2026-08-24

Clean ESP-IDF 6.0.2 build `M3-32-g1038234` zapakiran je ključem `rel-001` i
lokalno instaliran preko `POST /api/ota/p4`. Unutarnji image imao je 2.363.856 B
i SHA-256 `2e10652122c8137d8b8f6a8d68837d3ef81450ffb0233d217400ebda19d31f2d`.
Uređaj je podigao `ota_0`, vratio OTA stanje u `idle`, obnovio SoftAP,
FLX4 MIDI In/Out/UAC i USB3 biblioteku od 191 trake. Output-late, oba PCM
underruna, UAC dropped/overflow i service-log dropped ostali su 0.

Ovaj build odvaja lokalni screensaver wake od udaljenih web naredbi. Nakon
više od dvominutnog mirovanja poslan je točno jedan D1 PFL POST: HTTP 200 i
`pfl1 false→true` potvrđeni su u prvom pokušaju. Web kontrola sada istodobno
probudi lokalni UI i izvrši udaljenu naredbu; fizički FLX4 wake pritisak i dalje
se potroši samo na sigurno buđenje.

## Hardware acceptance 2026-08-23

`M3-22-gd7466ea` (`rel-001`) instaliran je s HTTPS kanala iz factory
`M3-20-g9f24b19` u `ota_0`. Uređaj je preuzeo i verificirao image od
2.361.984 B, rebootao sa SW reset razlogom te ga je startup health gate označio
valjanim. NVS servisna konfiguracija, SoftAP `Pajoniiir-M3`, 191-track library i
FLX4 MIDI In/Out/UAC ostali su dostupni; nije bilo rollbacka, output-latea, PCM
underruna, UAC drop/overflowa ni service-log dropa.

Kontrolirani check jednake objavljene verzije vratio je
`already running this build` bez reseta i uz isti boot ID 175. Jedan raniji
neposredni same-version check nakon lokalnog web-upload OTA boota izazvao je
izolirani PANIC reset koji se u odgođenim provjerama nije ponovio. Namjerni
health-failure rollback sada je potvrđen. Tri dodatna svježa post-OTA ciklusa
s uključenim flash coredumpom nisu reproducirala PANIC, pa događaj ostaje
rezidualna monitoring stavka i ne blokira OTA acceptance.

### Missing-bundle fault, 2026-08-24

Odvojeni base URL `https://pajoniiir.zadar.click/ota/ota-test/` poslužio je
valjani channel dokument koji je nudio `M4`, ali je relativna bundle putanja
namjerno vraćala HTTP 404. Check je objavio `update available: M4`, install je
prihvaćen s HTTP 202, prešao u `downloading 0%` i završio s
`bundle not on the server`.

Uređaj nije rebootao niti promijenio slot: boot ID ostao je 175, a running
firmware `ota_0 / M3-22-gd7466ea`. SoftAP se obnovio, NVS zaporka ostala je
spremljena, FLX4 MIDI In/Out/UAC i 191-track library ostali su dostupni, a
output-late, PCM underrun, UAC drop/overflow i service-log dropped ostali su 0.
Nakon testa produkcijski `/ota/` base URL vraćen je u NVS.

### Firmware-health rollback, 2026-08-24

Test-only overlay `firmware/common/sdkconfig.rollback_test.defaults` izgrađen
je odvojeno od produkcijskog `sdkconfig.defaults`. Potpisani
`M3-24-g3b2dc41` image od 2.360.864 B lokalno je učitan iz valjanog
`ota_0 / M3-22-gd7466ea` u `ota_1`. Novi slot podigao se kao
`pending_verify`, dovršio startup do health gatea i namjerno se restartao prije
`esp_ota_mark_app_valid_cancel_rollback()` potvrde.

Bootloader je na sljedećem bootu automatski odbacio nepotvrđeni slot i vratio
`ota_0 / M3-22-gd7466ea` u stanju `valid`. OTA API ostao je `idle`, SoftAP se
vratio, produkcijski HTTPS URL i `has_password=true` ostali su u NVS-u,
191-track library ponovno se učitao, a service-log dropped ostao je 0. FLX4 je
za završnu provjeru bio namjerno odspojen i njegova odsutnost nije tretirana kao
rollback regresija.

### Coredump dijagnostika

`firmware/common/sdkconfig.coredump_test.defaults` je test-only overlay koji
sprema jedan flash coredump u postojeću 64-KiB `coredump` particiju. Snimka je
ograničena na osam taskova, ne uključuje cijeli DRAM i koristi zaseban 1.792-B
stack. Produkcijski defaults eksplicitno se provjeravaju host testom i ne
uključuju ni coredump ni prisilni rollback.

Dijagnostički image izgrađen je s ESP-IDF 6.0.2 i pinanim
`PROJECT_VER=M3-22-gd7466ea`, tako da je produkcijski same-version kanal mogao
poslužiti kao equality gate bez promjene servera. Potpisani image tri puta je
svježe podignut preko `ota_0` i `ota_1`; svaki je boot prošao startup health
gate, a neposredni check završio je s `already running this build`. Nije bilo
PANIC-a ni neočekivanog reseta. `esp_coredump info_corefile` zatim je na
`0xc20000` / `0x10000` pročitao prazno `0xFFFF` zaglavlje, odnosno nijedan dump
nije nastao.

Ako se povijesni simptom ikada ponovi, dump treba pročitati prije novog testa
uz ELF točno tog builda. Nakon acceptancea vraćen je poznati potpisani
produkcijski `M3-22-gd7466ea` u `ota_1`; health gate označio ga je valjanim,
OTA API je `idle`, produkcijski HTTPS URL i NVS zaporka su očuvani, SoftAP je
uključen, USB3 library ima 191 track, a service-log, output-late i oba PCM
underrun brojača ostali su 0. FLX4 je namjerno odspojen.

### Dual-USB clean-build acceptance, 2026-08-24

Lokalni potpisani `M3-28-g809c203` OTA uredno je podignut u `ota_1`, vratio
SoftAP i FLX4 MIDI In/Out/UAC, ali USB3 nije proizveo attach događaj ni nakon
dva fizička remove/reinsert ciklusa. Uzrok nije bio OTA: clean build je uklonio
raniju ignored izmjenu `espressif__usb/src/hcd_dwc.c` koja je HS MSC i FS FLX4
kontrolerima davala različite FIFO raspodjele.

Popravak `M3-29-g2b0ad21` verzionira fail-closed CMake transformaciju. Ona
čita pinani, hash-ispravni `espressif/usb 1.5.0`, stvara patched HCD isključivo
pod `build/pajoniiir_usb` i njega dodaje component targetu; više ne mijenja
`managed_components`. Puni ESP-IDF 6.0.2 `fullclean` build i cijeli P4 host
suite prošli su. Potpisani image od 2.362.832 B (`rel-001`, SHA-256
`182f49d6f60dde12c25e957222ec620b95f527d541f21ad138b6ea156b156088`)
zatim je podignut u `ota_0`. Na istom bootu vratili su se SoftAP, FLX4
MIDI In/Out/UAC, USB3 mount i svih 191 track; OTA API ostao je `idle`, bez
rollbacka ili prijavljene greške.
