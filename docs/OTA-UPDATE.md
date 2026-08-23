# P4 OTA Update

Status: važeći P4-only postupak, hardverski potvrđen 2026-08-23.

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
health-failure rollback i reprodukcija tog panica ostaju otvoreni acceptance
scenariji.

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
