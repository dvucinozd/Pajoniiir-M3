# P4 OTA Update

Status: važeći P4-only postupak, 2026-08-22.

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

## Verifikacija

- odbij krivi potpis, target, chip, projekt, hash i preveliku sliku;
- potvrdi running version/slot kroz `/api/firmware`;
- nakon restarta provjeri UI, USB2 FLX4, USB3 storage i oba audio izlaza.
