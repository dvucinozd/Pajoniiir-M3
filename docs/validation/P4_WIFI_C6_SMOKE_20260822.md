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
- ponovljeni AP reconnect i AP→STA→AP probe;
- potpisani P4 OTA uspjeh/neuspjeh/rollback;
- paralelni Wi-Fi + USB2 + USB3 + dual-deck audio stress;
- vizualna i touch provjera na novom 5,0-inčnom ekranu nakon njegova dolaska.
