# P4 FLX4 hot-plug stress — 2026-08-23

## Opseg

Fizički USB2 unplug/replug stress provjerava da DDJ-FLX4 tijekom utišanog
dual-deck playbacka vraća MIDI In/Out, UAC1 i LED state bez P4 reseta, prekida
playbacka, PCM underruna ili trajnog audio deadline problema. Test je uz to
izolirao rad FLX4 USB event taska tijekom disconnect i reconnect prijelaza.

## Konfiguracija

- ploča: JC-ESP32P4-M3-DEV, ESP32-P4 revizija v1.3;
- serijski port: `COM17`;
- ESP-IDF: v6.0.2;
- FLX4: USB2 FS Host, Pioneer VID:PID `2B73:0045`;
- Rekordbox medij: USB3 HS Host, library 191 track;
- Wi-Fi: SoftAP `Pajoniiir-M3`, uključen tijekom cijelog testa;
- audio: oba channel fadera programski na nuli, PCM5102A master aktivan;
- service log: microSD prisutan, dropped counter 0.

## Početni stress — `M3-2-g4613c4a`

Deset uzastopnih ciklusa završilo je bez API pogreške ili P4 reseta. Svaki je
ciklus vratio FLX4 `present`, MIDI In/Out i UAC; reconnect je trajao približno
4,9–7,7 s, a oba decka nastavila su playback.

| Brojač | Rezultat |
| --- | --- |
| puni reconnect | 10/10 |
| output late | +20, točno +2 po ciklusu |
| najveći output blok | 265.131 µs |
| PCM underrun D1/D2 | 0/0 |
| UAC dropped/overflow | 0/0 u 20-s post-prozoru |
| service-log dropped | 0 |

Fazni maksimumi (`main=264.058 µs`, `mix=257.838 µs`) pokazali su da audio
task nije radio sporo unutar jedne funkcije, nego je bio descheduliran tijekom
USB transition rada. FLX4 USB task i audio output bili su na istom CPU0, a USB
task je stalno ostajao na prioritetu 7.

## Prva korekcija — `M3-5-gec3850a`

FLX4 USB task počeo je na transition prioritetu 5, spuštao se nakon
`DEV_GONE`, a nakon enumeracije dizao na aktivni prioritet 7. Pet fizičkih
ciklusa i dalje je dalo +2 output-latea po ciklusu, ali je maksimum pao na
50.502 µs i PCM underrun ostao 0/0. Telemetrija je pokazala dva preostala ruba:

- disconnect rad prije stvarnog spuštanja prioriteta (`monitor` do 19.291 µs);
- početno UAC queue punjenje nakon prerano podignutog prioriteta (`main` do
  48.177 µs).

## Završna korekcija — `M3-6-g546fa58`

Prioritet se sada spušta odmah na ulazu u `DEV_GONE`. Reconnect ostaje na
transition prioritetu kroz descriptor parsing, kontrolne transfere, alokaciju i
početno punjenje tri isochronous transfera; na prioritet 7 diže se tek nakon što
je periodični UAC red predan hostu.

Ponovljeni test koristio je dvije 44,1-kHz trake i PCM5102A output na 44,1 kHz:

| Provjera | Rezultat |
| --- | --- |
| puni reconnect | 3/3 |
| reconnect vrijeme | 4,916–6,665 s |
| MIDI In/Out + UAC nakon reconnecta | 3/3 |
| playback nastavljen | 3/3 oba decka |
| output late na disconnectu | +1 po ciklusu |
| output late na reconnectu | 0 po ciklusu |
| najveći output blok tijekom gatea | 27.626 µs |
| PCM underrun tijekom gatea | 0/0 |
| završni aktivni UAC drop/overflow/underflow | 0/0/0 |
| service-log dropped | 0 |

Nakon posljednjeg reconnecta Deck 2 je dodatnih 20 s radio bez novog
output-latea, PCM underruna, UAC dropa, overflowa ili underflowa. Deck 1 je u
prethodnom prozoru došao do prirodnog EOF-a i tada je izvan hot-plug gatea
zabilježio 136 PCM underrun frameova; taj EOF/STOP rub ostaje zaseban zadatak.

## Otkriveni 48-kHz UAC problem

Za dulji test prvotno su odabrane dvije 48-kHz trake. PCM5102A/output engine
otvorio se na 48 kHz, dok FLX4 UAC packetizer i endpoint ostaju na 44,1 kHz.
U mirnom 20-sekundnom prozoru nastalo je:

- 3.759 predanih producer blokova;
- 1.245 dropped blokova;
- 70.846 overflow frameova;
- 7.518 clock-trim frameova;
- 0 PCM underruna i 0 output-latea.

To odgovara trajnom nominalnom rate mismatchu, a ne scheduler jitteru.
Postojeća korekcija od najviše jednog framea po producer bloku namijenjena je
ppm driftu između dva jednaka nominalna clocka. Potreban je stateful
48→44,1-kHz resampler prije FLX4 četverokanalnog UAC ringa, uz očuvanje faze
između output blokova.

## 48-kHz resampler acceptance — `M3-8-gffb9f42`

Dodani stateful linearni resampler koristi točno racionalno source/target
vrijeme, čuva prethodni frame i fazu preko granica output blokova te ostavlja
44,1-kHz ulaz bit-identičnim. Host testovi pokrivaju passthrough, točan
48→44,1-kHz omjer, jednu sekundu 48-kHz ulaza, split/whole-block kontinuitet i
nevažeće argumente.

Ponovljeni hardware gate koristio je dvije stvarne 48-kHz trake, zajednički
output na 48 kHz i FLX4 UAC endpoint na 44,1 kHz. Oba channel fadera ostala su
programski utišana; zato je ovo counter/timing acceptance, a ne završni slušni
quality gate.

| Provjera | Rezultat tijekom 60,028 s |
| --- | --- |
| oba decka nastavila playback | da, +60.053/+60.054 ms |
| HTTP status/library/firmware | 60/60, 6/6, 4/4 |
| FLX4 MIDI In/Out + UAC | prisutni cijelo vrijeme |
| UAC producer blokovi/frameovi | 11.260 / 2.648.293 |
| UAC dropped/overflow/aktivni underflow | 0/0/0 |
| ring početak/kraj/high-water | 1.161/1.161/1.454 od 2.048 frameova |
| clock trim/duplicate | 62/3 framea |
| PCM underrun D1/D2 | 0/0 |
| output late | +1, maksimum 11.218 µs |
| service-log dropped | 0 |
| slobodni heap početak/kraj | 25.095.292/25.095.292 B |

Nasuprot početnom 20-sekundnom prozoru s 1.245 dropped blokova i 70.846
overflow frameova, ponovljeni gate nema aktivni UAC gubitak i ring nema neto
pomak. Time je funkcionalni rate-mismatch counter gate zatvoren.

## Mixed-rate produženi acceptance

Naknadni test namjerno je kombinirao Deck 1 izvor od 44,1 kHz (`Evelyn Thomas -
High Energy`) i Deck 2 izvor od 48 kHz (`Megatron Man`) uz zajednički output na
48 kHz. Oba channel fadera bila su programski na nuli. Glavni soak trajao je
300 s, nakon čega je napravljen točno mjeren produžetak od 60,61 s.

| Provjera | Glavni prozor + produžetak |
| --- | --- |
| HTTP status/library/firmware | 300/30/20 + 60/6/4, bez greške |
| napredak u produžetku D1/D2 | 60.672/60.672 ms |
| PCM underrun D1/D2 | 0/0 |
| UAC dropped/overflow/aktivni underflow | 0/0/0 |
| UAC blokovi/frameovi u produžetku | 11.376 / 2.675.576 |
| UAC ring početak/kraj/high-water | 1.338/1.102/1.475 od 2.048 frameova |
| clock trim/duplicate u produžetku | 60/1 frame |
| output late | jedan izolirani događaj u glavnom prozoru, 0 u produžetku |
| service-log dropped | 0 |
| heap delta | -32 B |

Nakon zaustavljanja oba decka nije nastao novi output-late, PCM underrun, UAC
drop ni overflow. Rast idle UAC underflow brojača očekivan je jer endpoint tada
troši namjerno poslanu tišinu. Ovim su zatvoreni duži mixed-rate counter gate i
stabilnost 44,1→48→44,1-kHz puta. Slušni cue/master i level acceptance dovršen
je zasebno u `P4_FLX4_HEADPHONE_LEVEL_20260823.md`.

## Dodatna opažanja

- fizički donji položaji channel fadera jednom su očitani kao D1=1.521 i
  D2=17, pa prije konačnog mixer acceptancea treba potvrditi raw min/max i
  eventualnu per-deck kalibraciju.

## EOF/STOP counter izolacija — `M3-12-g6535f92`

Na 48-kHz source/output profilu prirodni EOF nije povećao PCM brojač. Na
mixed-rate profilu, s Deckom 1 na 44,1 kHz i zajedničkim outputom na 48 kHz,
isti rub je na prethodnoj slici deterministički prijavio +38 PCM frameova.
Ponavljanje bez web pollinga dalo je isti rezultat, dok je STOP/reload tijekom
playbacka ostao na 0/0. Time je isključen web promet i potvrđen EOF-specifičan
uzrok.

Zadnji resamplirani output blok može legitimno zatražiti još jedan source frame
nakon što je decoder već dosegnuo EOF. `audio_eof_policy` sada takav očekivani
miss ne broji kao underrun; stvarni miss prije decoder EOF-a i dalje se broji.
Ponovljeni fizički mixed-rate EOF na točnom `M3-12` buildu završio je s PCM
deltom 0/0 i UAC dropped/overflow 0/0. Jedan 10.829-µs deadline događaj nastao
je približno sekundu prije EOF-a i nije bio uzročno vezan uz završni blok.

## Završni disconnect rub — `M3-13-gc95bd4b`

Kontrolni A/B ciklus na `M3-12-g6535f92` koristio je isti utišani mixed-rate
dual-deck profil. FLX4 se potpuno vratio za približno 7,075 s, oba decka
napredovala su po 37.610 ms, PCM underrun ostao je 0/0, ali fizički disconnect
je dodao jedan output-late od 27.658 µs. Na samom rubu zabilježena su tri UAC
dropped bloka i 575 overflow frameova; reconnect reset je brojače vratio na
0/0 i sljedećih 15 s bilo je stabilno. Fazni maksimum `monitor=26.488 µs`, uz
`main=8.942 µs`, smjestio je zastoj prije PCM5102A writea.

USB host može dostaviti canceled/no-device transfer callback prije
`DEV_GONE` eventa. Takav callback je zato još radio na aktivnom prioritetu 7 i
mogao pripremiti ili predati novi MIDI/UAC transfer. `M3-13` na prvom
terminalnom transfer statusu odmah spušta USB task na transition prioritet 5 i
iz callbacka izlazi bez resubmita; `DEV_GONE` ostaje autoritativan za state i
cleanup.

Ponovljeni fizički unplug/replug vratio je FLX4 `present`, MIDI In/Out i UAC bez
P4 reseta. Oba mixed-rate decka nastavila su playback. Nakon punog reconnecta i
stabilizacijskog prozora rezultat je bio output-late 0, PCM underrun 0/0, UAC
dropped/overflow 0/0 i service-log dropped 0. Završni fazni snapshot ostao je
unutar bloka (`monitor=2.262 µs`, `main=8.750 µs`). Time je zatvoren posljednji
poznati disconnect-only deadline rub.

## Software i flash gate

- `tests/run_p4_host_tests.ps1`: PASS;
- ESP-IDF v6.0.2 `idf.py build`: PASS;
- aplikacija: `M3-13-gc95bd4b`, `0x2407d0`, 44% slobodno;
- flash na `COM17`: PASS, svi zapisani hashovi verificirani;
- završno stanje: FLX4 spojen s MIDI In/Out i UAC-om, oba decka `READY` i
  utišana, Wi-Fi uključen, library 191, service-log dropped 0.
