# P4 FLX4 headphone-level acceptance — 2026-08-23

## Opseg

Fizički test provjerava da 14-bitni FLX4 `HEADPHONES LEVEL` burst ne prekida
audio output, da promjena gaina ne proizvodi zipper click te da kanalni PFL/CUE
ostaje neovisan o channel faderu.

## Otkriveni problem

Na prethodnom firmwareu korisnik je čuo pucketanje i kratke prekide pri
okretanju `HEADPHONES LEVEL`. Tijekom istog fizičkog pokreta output-late brojač
porastao je s 1 na 67, a najveći blok dosegnuo je 76.586 µs. PCM underrun i UAC
drop/overflow ostali su na nuli.

Uzrok je bio dvostruki `ESP_LOGW` za svaki sirovi FLX4 MIDI paket i svaki
prevedeni događaj iz priority-7 USB/MIDI callbacka. 14-bitni regulator šalje
MSB/LSB burstove, pa je serijski WARN promet deschedulirao priority-6 audio
output. Gain se uz to mijenjao jednom po cijelom 256-frame bloku, što je moglo
stvoriti čujni diskontinuitet i bez izgubljenog bloka.

## Korekcija — `M3-10-g638f542`

- uklonjeno je packet/event WARN logiranje iz FLX4 real-time callbacka;
- `HEADPHONES LEVEL` sada koristi kontinuirani per-frame linearni ramp koji
  točno doseže najnoviji target i sigurno se retargetira usred rampe;
- kada nijedan deck nije aktivan, ramp se inicijalizira na aktualni fizički
  gain kako prvi aktivni blok ne bi krenuo sa stale glasnoćom;
- host testovi pokrivaju kontinuitet, retarget i clamp, a statički gate zabranjuje
  povratak spornih raw/event logova.

## Fizički rezultat

Deck 2 koristio je `Megatron Man.mp3`, izvorni i zajednički output rate 48 kHz,
channel fader 2 na 12,5% samo za početnu slušnu provjeru. Tijekom mjerenog
30-sekundnog prozora operator je više puta okrenuo `HEADPHONES LEVEL` gore–dolje.

| Provjera | Rezultat |
| --- | --- |
| status uzorci / API greške | 110 / 0 |
| playback napredak | 30.230 ms |
| output late delta / maksimum | 0 / 0 µs |
| PCM underrun D1/D2 delta | 0/0 |
| UAC dropped/overflow/aktivni underflow delta | 0/0/0 |
| UAC ring početak/kraj/high-water | 1.160/1.166/1.475 od 2.048 frameova |
| service-log dropped delta | 0 |
| slušni rezultat | PASS — nema pucketanja ni prekida |

Nakon toga je s channel faderom na nuli uključen kanalni CUE/PFL, a
`HEADPHONES MIX` okrenut prema CUE. Operator je potvrdio da je Deck 2 čujan u
slušalicama prije fadera. To potvrđuje očekivani routing: PFL je post-trim i
pre-channel-fader, dok MASTER strana headphone miksa prati post-fader master.

## Software, build i završno stanje

- `tests/run_p4_host_tests.ps1`: PASS;
- ESP-IDF v6.0.2 `idf.py build`: PASS;
- aplikacija: `M3-10-g638f542`, `0x2406d0`, 44% slobodno;
- flash na `COM17`: PASS, zapisani hashovi verificirani;
- završno: Deck 1 `IDLE`, Deck 2 `READY`, oba channel fadera 0, FLX4 MIDI
  In/Out + UAC prisutni, library 191, service-log dropped 0 i Wi-Fi
  `Pajoniiir-M3` ostavljen uključen.
