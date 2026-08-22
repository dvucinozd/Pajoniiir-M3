# Development Plan

Status: plan nakon single-chip čišćenja, 2026-08-22.

## Trenutna baza

- jedan ESP32-P4 firmware target;
- izravni FLX4 USB MIDI In/Out i UAC1 headphone put;
- zaseban USB3 MSC put za Rekordbox medij;
- dual-deck audio, DSP, UI, Wi-Fi remote i potpisani P4 OTA;
- uklonjeni pomoćni kontrolni firmware, UART/bulk protocol, `.s3bin` profili,
  peer debug/OTA i međupanački PCM link.

Wi-Fi programski temelj već postoji: ESP32-C6/ESP-Hosted preko SDIO-a,
SoftAP `Pajoniiir-M3`, web API, privremeni STA put i potpisani P4 OTA. Prije
releasea treba dovršiti konfiguraciju, operatorski tok i hardversku/stress
validaciju.

Ciljani 5,0-inčni MIPI-DSI zaslon (800×480, nativni landscape) s FT5426
dodirom je naručen i čeka se njegov dolazak. U dokumentaciji je to ciljana
konfiguracija; hardverski bring-up još nije potvrđen.

Prvi korisni firmware nakon `RC2` uvodi release identitet `M3`. Verzija se
dobiva iz `git describe`, pa je implementacijski commit namijenjen anotiranom
`M3` tagu. `p4_ota_pull` parser sada prihvaća `RC<number>` i `M<number>`, tretira
M porodicu kao monotono noviju od RC porodice te ima migracijske OTA testove za
`RC2` → `M3`. Clean build na tagu mora prijaviti točno `M3`.

## Sljedeće faze

### 1. Učvrstiti izravni FLX4 host

- [x] dodati host testove za `p4_flx4_map`, MIDI generation gate, LED encoder,
  UAC packetizer i audio ring;
- [ ] potvrditi sve interface/endpoint/alternate-setting izbore na stvarnom
  FLX4;
- [ ] provjeriti reconnect tijekom playbacka i puni LED resync.

Host gate sada pokriva 1.417 provjera i dio je `tests/run_p4_host_tests.ps1`.
Uz testove su ispravljeni kombinirani Beat FX CH1/CH2 target, odbijanje
nevažećeg LED decka te MIDI OUT admission/reset pri disconnectu.

Acceptance: bez stale događaja, LED mismatcha ili kontinuiranih UAC dropova.

### 2. Dovršiti Wi-Fi 6 implementaciju

- [x] potvrditi ESP32-C6 firmware, ESP-Hosted i SDIO link na završnoj P4 slici;
- [x] dovršiti trajnu Wi-Fi konfiguraciju za normalni SoftAP rad i servisne STA
  vjerodajnice, bez ispisa tajni u statusu ili logovima;
- [x] dovršiti Settings tok za enable/disable, AP/STA stanje, IP adresu, pogreške i
  siguran povratak iz privremenog STA načina u SoftAP;
- [ ] hardverski provjeriti web UI/API, reconnect, više klijenata i paralelni rad
  Wi-Fi prometa s USB2, USB3 i audio putanjama;
- [ ] potvrditi potpisani P4 OTA preko STA veze, uključujući neuspjeli download,
  rollback i obnovu SoftAP-a.

Programsko učvršćivanje sada objavljuje stvarno OFF/STARTING/AP/STA/RESTORING/
ERROR stanje, AP/STA adresu i broj AP klijenata na Settings ekranu. Asinkrono
gašenje čeka aktivni probe/OTA transition lease umjesto rušenja C6 transporta
ispod prijelaznog taska; odluka START/STOP/WAIT pokrivena je host testom.
Hardverski smoke 2026-08-22 potvrdio je SDIO/C6 bring-up, novi SoftAP
`Pajoniiir-M3`, DHCP i HTTP 200 za `/api/status` i `/api/firmware`. U istom je
testu ispravljeno zadržavanje AP-a preko P4-only reseta: GPIO54 sada drži C6
isključenim pri bootu i nakon teardowna dok je spremljena postavka OFF.
Nastavak s umetnutom microSD karticom prošao je tri AP reconnect ciklusa,
četiri paralelna status zahtjeva te kratki 100× status / 10× library soak bez
HTTP greške, service-log dropa ili vidljivog heap curenja. Puni AP→STA→AP
ostaje otvoren jer servisna STA mreža još nije konfigurirana. Stvarni Chromium
tok dodatno je potvrdio prikaz 191 tracka, pretragu i HTTP 200 D1 LOAD za
`Darude - Sandstorm.mp3`; Deck 1 ostao je `READY` i nakon reloadanja stranice,
bez JavaScript pogrešaka. Wi-Fi je zatim trajno ostavljen uključen uz očuvane
postojeće NVS postavke i hot cueove, kako bi web controller služio za učitavanje
traka do dolaska novog ekrana. Kombinirani 189,5-sekundni USB2/USB3/audio/Wi-Fi
soak s priključenim FLX4 prošao je 180 status, 18 library i 12 firmware zahtjeva
bez HTTP greške, controller disconnecta, prekida playbacka, codec greške,
output deadline missova ili PCM underruna tijekom samog prozora. Međutim,
FLX4 UAC ring prijavio je 339 nepotpunih write blokova; naknadni prozori dali su
0/30 s uz gotovo bez mrežnog prometa, 5/135 zahtjeva pod burst prometom i 7/45 s
uz normalni status polling. Zato paralelni audio gate ostaje otvoren dok se ne
dodaju dropped-frame/ring-fill brojači i clock-drift regulacija te ponovi soak.
M3 implementacija dodaje overflow/underflow, ring fill/high-water i
trim/duplicate telemetriju te jednoframe korekciju izvan 3/8–5/8 dead-banda;
hardverski M3 soak tek treba potvrditi rezultat.

Acceptance: Wi-Fi se uključuje samo eksplicitno, SoftAP i privremeni STA rade
ponovljivo nakon cold boota i reconnecta, OTA se sigurno oporavlja, a mrežni
promet ne uzrokuje audio dropove, USB reset ni curenje vjerodajnica.

### 3. Bring-up novog 5,0-inčnog zaslona

- po dolasku evidentirati točan panel/controller, DSI lane konfiguraciju,
  timing, reset, backlight i FT5426 I2C adresu prema stvarnom primjerku;
- pokrenuti panel u 800×480 nativnom landscape načinu i potvrditi stabilan
  cold/warm boot bez tearinga, artefakata ili periodičnog gubitka slike;
- prilagoditi BSP/Kconfig, LVGL rezoluciju, PPA put i touch transformaciju tek
  nakon potvrde stvarnog panel ID-a i električnih parametara;
- provjeriti cijelu dodirnu površinu, orijentaciju, multitouch gdje ga UI
  koristi, svjetlinu, potrošnju i ponašanje screensavera;
- vizualno pregledati sve ekrane te tek nakon toga obnoviti 800×480 UI
  screenshot baseline i odraditi dugi display/touch/PSRAM soak.

Status: hardverski rad je blokiran do dolaska zaslona; priprema se može raditi
iz dokumentacije dobavljača, ali controller naredbe i timing ne treba nagađati.

Acceptance: puni kadar i touch koordinate rade u nativnom landscapeu na cijeloj
površini, svi postojeći UI tokovi su čitljivi i nema display/audio regresija.

### 4. Dual-USB stress

- istodobno streamati dva decka s USB3 dok FLX4 MIDI/UAC radi na USB2;
- mjeriti output deadline, cache miss, USB recovery i headphone drop brojače;
- ponoviti connect/disconnect i zamjenu medija tijekom sigurnih transport stateova.

Acceptance: nema audio artefakata, deadlocka ni reset loopa u dugom soaku.

### 5. MIDI/LED feature parity

- proći preostale redove u `DDJ_FLX4_MIDI_MAP.md` izravno iz XML reference;
- za svaku kontrolu dodati input behavior i LED reconnect test;
- ukloniti zastarjele numeričke semantičke ID-jeve tek nakon pokrivanja.

Acceptance: svi podržani FLX4 elementi imaju jednoznačan P4 state owner.

### 6. Audio acceptance

- hardware-verify cue/master routing, headphone level/mix i PCM5102A headroom;
- dovršiti scratch/Master Tempo rubne slučajeve uz loop i pitch promjene;
- postaviti pragove za UAC ring i output timing alarme.

### 7. Release hardening

- P4-only reproducibilni clean build i OTA package gate;
- UI screenshot baseline nakon vizualne provjere Settings promjene i novog
  800×480 zaslona;
- ažurirani startup smoke, risk register i release validation zapis.
