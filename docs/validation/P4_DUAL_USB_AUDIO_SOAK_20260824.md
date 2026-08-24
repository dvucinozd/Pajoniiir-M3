# P4 dual-USB audio soak — 2026-08-24

## Opseg

Ovaj hardware acceptance provjerava istodobni USB3 Rekordbox read/decode,
USB2 FLX4 MIDI/UAC, mixed-rate dual-deck playback, kontinuirani Wi-Fi/web API
promet, ponovljene load/seek/start/stop prijelaze i fizički FLX4 hot-plug.

## Konfiguracija

- ploča: JC-ESP32P4-M3-DEV, ESP32-P4 revizija v1.3;
- firmware: produkcijski `M3-22-gd7466ea`, `ota_1`, stanje `valid`/API `idle`;
- FLX4: USB2 FS Host, VID:PID `2B73:0045`, MIDI In/Out i UAC1;
- Rekordbox medij: USB3 HS Host, VID:PID `18A5:0302`, library 191 track;
- Wi-Fi: SoftAP `Pajoniiir-M3`, uključen tijekom cijelog testa;
- Deck 1: `Ken Laszlo - Mary Ann`, 48 kHz, 767 s;
- Deck 2: `Evelyn Thomas - High Energy`, 44,1 kHz, 661 s;
- zajednički output: 48 kHz; FLX4 UAC endpoint: 44,1 kHz;
- oba channel fadera: programski 0, counter/timing gate bez glasnog mastera.

## Desetominutni mixed-rate soak

Glavni prozor trajao je 600,502 s. Završni snapshot uzet je prije prirodnog
EOF-a kraće trake. Web promet je tijekom playbacka kontinuirano čitao puni
status, library i firmware stanje.

| Provjera | Rezultat |
| --- | --- |
| status API | 643/643, bez greške |
| library API | 64/64, svaki put 191 track |
| firmware API | 42/42, svaki put `ota_1 / M3-22-gd7466ea`, `idle` |
| Deck 1 napredak | 59.520 → 660.048 ms, +600.528 ms |
| Deck 2 napredak | 59.445 → 659.973 ms, +600.528 ms |
| playback/controller miss | 0/0 |
| regresija pozicije | 0 |
| PCM underrun D1/D2 | 0/0 |
| UAC dropped/overflow/aktivni underflow | 0/0/0 |
| UAC ring raspon | 926–1.397 od 2.048 frameova |
| UAC clock trim/duplicate | 505/10 frameova |
| output late | +4, maksimum 11.659 µs uz prag 10.668 µs |
| service-log dropped | 0 |
| minimalni heap/internal/PSRAM | 25.081.984 / 107.547 / 25.006.172 B |

Četiri output-late događaja bila su vremenski odvojena i trajala su
10.732–11.659 µs. Svaki je zadržao veliku PCM zalihu; nijedan nije proizveo
PCM underrun, UAC gubitak, čujni simptom, reset ili daljnji burst.

Deck 2 dosegnuo je EOF neposredno nakon završnog snapshot-a, između dvije stop
naredbe. Njegov `play_pause` zato ga je pokrenuo od početka; odmah je ponovno
zaustavljen. Taj post-prozor nije dio desetominutnog mjerenja i PCM je ostao
0/0.

## Load/seek/start/stop rub

Deset dodatnih ciklusa ponavljalo je:

1. reload obje iste trake s USB3;
2. paused seek na različite pozicije;
3. gotovo istodobni start oba decka;
4. pet sekundi stabilnog aktivnog playbacka;
5. stop oba decka.

Svih 10/10 aktivnih prozora završilo je s PCM 0/0, UAC
dropped/overflow/underflow 0/0/0, bez controller miss-a, deadline miss-a ili
service-log dropa. Heap i UAC ring ostali su stabilni.

Serijski log pokazao je šest marginalnih output-late događaja izvan mjerenih
aktivnih prozora. Tri dodatna fazno instrumentirana ciklusa izolirala su rub:
load 0, paused seek 0, stabilni playback 0 i stop 0; gotovo istodobni dual start
dao je 0/2/1 late događaj. Maksimum se nije povećao iznad 11.659 µs i PCM/UAC
brojači ostali su 0. Zajedno s ranijih 12 kontroliranih startova, povijesni
jednokratni PCM D1=202 događaj nije se ponovio u ukupno 25 start ciklusa, od
kojih je 18 uključivalo load i paused seek.

## FLX4 hot-plug pod playbackom

Nakon ponovnog muted dual-deck starta FLX4 je fizički odspojen i vraćen, dok su
USB3 medij, Wi-Fi i P4 ostali aktivni.

| Provjera | Rezultat |
| --- | --- |
| puni MIDI In/Out + UAC reconnect | 5,597 s |
| playback tijekom disconnecta | oba decka nastavila |
| 30-s post-reconnect napredak | +31.376/+31.376 ms |
| post-reconnect status greške | 0/30 |
| output-late delta | 0 |
| PCM underrun delta D1/D2 | 0/0 |
| UAC dropped/overflow | 0/0 |
| service-log dropped | 0 |
| P4 reset | nije se dogodio |

USB host jednom je zapisao prolazni `Root port reset failed`, ali je odmah
enumerirao FLX4 na novoj adresi i potpuno obnovio audio i MIDI interfacee. Nije
ostala funkcionalna ili brojačka greška.

## USB3 media remove/reinsert

Nakon hot-plug gatea i njegova završnog 30-s snapshot-a zabilježen je zaseban
P4 boot. Service journal ga jednoznačno vodi kao novi boot ID 192 s reset
razlogom `POWERON`, odnosno fizički prekid napajanja/USB1 veze, ne PANIC,
watchdog ili firmware reset. Uređaj je zatim uredno vratio produkcijski
firmware, FLX4 i početnih 191 traka. USB3 test počeo je iz tog čistog boota.

Rekordbox medij fizički je izvađen i vraćen dok su oba decka bila zaustavljena
u `IDLE`. Vađenje je promijenilo media generation `1→2`, library spustilo na 0
i ostavilo oba decka zaustavljena. Ponovno umetanje vratilo je generation `3`
i svih 191 traka za 7,405 s.

| Provjera | Rezultat |
| --- | --- |
| service journal remove | boot 192, `USB_UNMOUNTED`, 234.827 ms |
| service journal reinsert | boot 192, `USB_MOUNTED`, 242.284 ms |
| library reload | boot 192, `LIBRARY_LOADED a0=191`, 242.373 ms |
| media generation | 1 → 2 → 3 |
| library count | 191 → 0 → 191 |
| library restore vrijeme | 7,405 s |
| 20-s post-restore greške | 0 |
| FLX4 MIDI In/Out + UAC | prisutni cijelo vrijeme |
| output-late delta | 0 |
| PCM underrun delta D1/D2 | 0/0 |
| UAC dropped/overflow | 0/0 |
| service-log dropped | 0 |
| P4 reset tijekom media testa | nije se dogodio; boot ID ostao 192 |

## Zaključak i završno stanje

Testirani duži dual-USB/audio/Wi-Fi profil i FLX4 hot-plug prolaze bez gubitka
playbacka ili podataka. USB3 media remove/reinsert također pravilno invalidira
i ponovno gradi library bez resetiranja P4, FLX4 ili Wi-Fi puta. Marginalni
dual-start deadline događaji ostaju timing telemetrija, ali ne reproduciraju
PCM kvar i ne blokiraju ovaj funkcionalni gate. Dual-USB/storage acceptance
blok za testirani profil time je zatvoren.

Uređaj je ostavljen na produkcijskom `ota_1 / M3-22-gd7466ea`, s oba decka u
`IDLE`, FLX4 MIDI In/Out/UAC aktivnim, libraryjem od 191 trake, Wi-Fi mrežom
`Pajoniiir-M3` uključenom te PCM, UAC drop/overflow i service-log dropped
brojačima na nuli.
