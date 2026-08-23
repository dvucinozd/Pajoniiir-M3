# Development Plan

Status: plan nakon mixed-rate, FLX4 headphone i disconnect/EOF acceptancea,
2026-08-23.

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

## Handoff za sljedeću sesiju

Završni hardverski baseline 2026-08-23 je `M3-20-g9f24b19` na P4 ploči. FLX4
je spojen s MIDI In/Out i UAC-om, oba decka su zaustavljena u `IDLE`, USB3
library sadrži 191 track, service log nema dropova, a SoftAP i Windows profil
`Pajoniiir-M3` ostavljeni su uključeni. Servisni SSID, zaporka i HTTPS update
URL spremljeni su u NVS-u; status izlaže samo SSID, URL i `has_password`, ne
zaporku. Fizički APSTA round-trip dobio je servisnu adresu `192.168.0.210`,
vratio `round trip complete` i sačuvao SoftAP klijentu valjanu adresu
`192.168.4.2`. HTTPS update check zatim je ispravno odbio stariju objavljenu
verziju bez downloada ili pisanja u flash.
Firmware sadrži dvostupanjski prioritet
FLX4 USB taska: transition rad je ispod audio outputa, a prioritet se podiže tek
nakon početnog UAC queue priminga. Stateful linearni UAC resampler sada pretvara
44,1–48-kHz output u FLX4-ov fiksni 44,1-kHz format uz kontinuitet faze između
output blokova. FLX4 MIDI callback više ne radi packet/event logiranje iz
real-time puta, a `HEADPHONES LEVEL` gain mijenja se kontinuiranim per-frame
rampom bez skoka na granici 256-frame output bloka. Otkazani USB transferi sada
spuštaju prioritet već prije `DEV_GONE` događaja i ne predaju novi MIDI/UAC
transfer. Prirodni decoder EOF više se ne broji kao PCM underrun kada zadnji
izlazni blok ostane djelomičan.

Prioritetni redoslijed nastavka:

1. objaviti noviji potpisani M3 testni bundle i dovršiti OTA install,
   neuspjeli-download i rollback acceptance; osnovni APSTA connectivity i HTTPS
   channel check sada su zatvoreni;
2. zadržati start/seek PCM brojače u sljedećim dugim audio soak provjerama;
   izolirani D1=202 događaj nije se ponovio u 12 kontroliranih ciklusa, uključujući
   pet load → paused seek → simultaneous start → stop ciklusa;
3. započeti 800×480 DSI/FT5426 bring-up tek nakon dolaska i identifikacije novog
   zaslona.

## Sljedeće faze

### 1. Učvrstiti izravni FLX4 host

- [x] dodati host testove za `p4_flx4_map`, MIDI generation gate, LED encoder,
  UAC packetizer i audio ring;
- [x] potvrditi sve interface/endpoint/alternate-setting izbore na stvarnom
  FLX4;
- [x] provjeriti reconnect tijekom playbacka i puni LED resync;
- [x] dodati stateful 44,1–48→44,1-kHz UAC resampling i potvrditi 48-kHz
  dual-deck counter gate na stvarnom FLX4.

FLX4 host gate i rate-aware resampler testovi dio su
`tests/run_p4_host_tests.ps1`. Uz testove su ispravljeni kombinirani Beat FX
CH1/CH2 target, odbijanje
nevažećeg LED decka te MIDI OUT admission/reset pri disconnectu.

Acceptance: bez stale događaja, LED mismatcha ili kontinuiranih UAC dropova.

### 2. Dovršiti Wi-Fi 6 implementaciju

- [x] potvrditi ESP32-C6 firmware, ESP-Hosted i SDIO link na završnoj P4 slici;
- [x] dovršiti trajnu Wi-Fi konfiguraciju za normalni SoftAP rad i servisne STA
  vjerodajnice, bez ispisa tajni u statusu ili logovima;
- [x] dovršiti Settings tok za enable/disable, AP/STA stanje, IP adresu, pogreške i
  siguran povratak iz privremenog STA načina u SoftAP;
- [x] hardverski provjeriti web UI/API, reconnect, više klijenata i paralelni rad
  Wi-Fi prometa s USB2, USB3 i audio putanjama;
- [x] hardverski potvrditi APSTA servisni round-trip bez gašenja lokalnog AP-a,
  DHCP-a, HTTP/DNS servisa ili ESP-Hosted/microSD transporta te pročitati HTTPS
  OTA kanal;
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
HTTP greške, service-log dropa ili vidljivog heap curenja. Stvarni Chromium
tok dodatno je potvrdio prikaz 191 tracka, pretragu i HTTP 200 D1 LOAD za
`Darude - Sandstorm.mp3`; Deck 1 ostao je `READY` i nakon reloadanja stranice,
bez JavaScript pogrešaka. Wi-Fi je zatim trajno ostavljen uključen uz očuvane
postojeće NVS postavke i hot cueove, kako bi web controller služio za učitavanje
traka do dolaska novog ekrana. Početni 189,5-sekundni USB2/USB3/audio/Wi-Fi soak
s priključenim FLX4 prošao je 180 status, 18 library i 12 firmware zahtjeva bez
HTTP greške, controller disconnecta, prekida playbacka, codec greške, output
deadline missova ili PCM underruna tijekom samog prozora. Međutim, stari UAC
brojač prijavio je 339 nepotpunih write blokova. M3 je zato dodao stvarne
overflow/underflow, ring fill/high-water i trim/duplicate brojače te jednoframe
clock korekciju izvan 3/8–5/8 dead-banda.

Prvi 180-sekundni M3 A/B run pokazao je da korekcija frekvencijskog drifta sama
nije dovoljna: 60 blokova / 6.757 frameova overflowa nastalo je dok je FLX4 USB
event task na prioritetu 5 čekao ispod audio producenta na prioritetu 6. Nakon
podizanja USB taska na prioritet 7, ponovljeni `M3-1-g243e996` run trajao je
183,724 s: oba decka napredovala su po 183.757 ms, 179/180 status, 18/18 library
i 12/12 firmware zahtjeva završili su uz jedan izolirani HTTP timeout, a FLX4 je
ostao spojen. UAC rezultat bio je 0 dropped blokova, 0 overflowa i 0 aktivnih
underflowa, uz high-water 1.640/2.048 frameova, 295 clock trimova i 43
duplikacije. Nije bilo output-latea ni PCM underruna tijekom reprodukcije ni
nakon zaustavljanja oba decka. Time je paralelni audio gate zatvoren za ovaj
profil; duži soak, fizički multi-client i USB reconnect/medij stress ostaju u
sljedećim fazama.

FLX4 reconnect gate zatvoren je na `M3-2-g4613c4a`. Početni test na prethodnoj
slici otkrio je da se interfacei pokušavaju otpustiti izravno u `DEV_GONE`
callbacku dok su otkazani MIDI/UAC URB-ovi još bili aktivni, pa se uređaj nije
mogao ponovno otvoriti bez P4 reseta. Cleanup je premješten u USB client task,
nakon obrade completion callbackova. U ponovljenom fizičkom testu Deck 2 nastavio
je playback kroz unplug/replug, a FLX4 se za približno 6,1 s vratio s MIDI In/Out
i UAC-om. Operator je potvrdio obnovljen PLAY LED. Sljedećih 30 s prošlo je bez
UAC dropa, overflowa, novog aktivnog underflowa ili PCM underruna; library je
ostao na 191 tracku i API nije imao prekid. Sam prijelaz povećao je output-late
brojač za dva, bez daljnjeg rasta nakon reconnecta, pa deadline ponašanje ostaje
dio dužeg stress gatea.

Duži stress 2026-08-23 prvo je na `M3-2-g4613c4a` odradio 10/10 fizičkih
unplug/replug ciklusa tijekom utišanog dual-deck playbacka. Svaki reconnect
vratio je MIDI In/Out i UAC bez P4 reseta, prekida playbacka ili PCM underruna,
ali svaki je ciklus dodao dva output-latea, a maksimum je bio 265.131 µs.
Fazna telemetrija pokazala je da FLX4 event task na prioritetu 7 deschedulira
audio output tijekom descriptor/cleanup transition rada.

`M3-6-g546fa58` zato spušta prioritet odmah na ulazu u `DEV_GONE`, enumerira i
prima početni UAC queue na transition prioritetu te se podiže iznad producenta
tek kada su periodični transferi predani. Ponovljena 3/3 ciklusa na 44,1-kHz
dual-deck profilu vratila su puni controller status za 4,916–6,665 s, bez PCM
underruna, UAC dropa, overflowa ili underflowa tijekom samog gatea. Reconnect
više ne dodaje deadline miss; ostaje po jedan događaj na fizičkom disconnectu,
s maksimumom 27.626 µs. Naknadni 20-sekundni aktivni prozor na Decku 2 bio je
potpuno stabilan. Detalji su u
`docs/validation/P4_FLX4_HOTPLUG_STRESS_20260823.md`.

Isti test otkrio je odvojenu sample-rate prazninu: s 48-kHz outputom FLX4 UAC
ostaje fiksan na 44,1 kHz. U 20 s nastalo je 70.846 overflow frameova i 1.245
dropped blokova, iako nije bilo output-latea ni PCM underruna. Jednoframe clock
regulator pokriva samo drift dvaju nominalno jednakih clockova; ne smije se
koristiti kao zamjena za 48→44,1-kHz resampler.

`M3-8-gffb9f42` dodao je stateful linearni resampler s točnim racionalnim
vremenom i host testovima za bit-identični 44,1-kHz passthrough, točan
48→44,1-kHz omjer te split/whole-block kontinuitet. Ponovljeni 60,028-sekundni
hardware gate s dvije 48-kHz trake i outputom na 48 kHz završio je s 0 novih
UAC dropped blokova, 0 overflowa i 0 aktivnih underflowa. Oba decka napredovala
su približno 60 s, ring je počeo i završio na 1.161 frameu, PCM underrun ostao
je 0/0, service log nije dropao zapise, a 60/60 status, 6/6 library i 4/4
firmware zahtjeva je prošlo. Jedan izolirani output-late, s maksimumom 11.218 µs,
nije proizveo PCM ili UAC grešku. Funkcionalni counter gate time je zatvoren.

Naknadni mixed-rate soak koristio je 44,1-kHz traku na Decku 1 i 48-kHz traku
na Decku 2 uz zajednički output na 48 kHz. Glavni 300-sekundni prozor i točno
60,61 s produženje završili su bez PCM underruna, UAC dropa, overflowa ili
aktivnog underflowa; u produženju su oba decka napredovala točno 60.672 ms, a
UAC ring je ostao stabilan. Jedan izolirani output-late u glavnom prozoru nije
se ponovio u produženju.

Isti baseline otkrio je da burst 14-bitnih `HEADPHONES LEVEL` MIDI poruka uz
dvostruko WARN logiranje iz priority-7 callbacka deschedulira audio output:
okretanje regulatora povećalo je output-late s 1 na 67, do 76.586 µs. Build
`M3-10-g638f542` uklanja ta dva real-time loga i uvodi kontinuirani gain ramp.
Ponovljeni 30-sekundni fizički test s višestrukim okretanjem regulatora završio
je s 0 output-latea, 0 PCM underruna, 0 UAC dropa/overflowa/aktivnog underflowa
i 30.230 ms neprekinutog napretka. Operator je potvrdio da više nema pucketanja
ni prekida. Dodatno je potvrđeno da kanalni PFL ostaje čujan sa spuštenim
faderom kada je kanalni CUE uključen i `HEADPHONES MIX` okrenut prema CUE.
Detalji su u `docs/validation/P4_FLX4_HEADPHONE_LEVEL_20260823.md`.

Na mixed-rate 44,1/48-kHz profilu prirodni EOF 44,1-kHz decka prvotno je
prijavljivao 38 PCM underrun frameova. To nije bio gubitak podataka nego
očekivani source miss u zadnjem djelomičnom output bloku nakon decoder EOF-a.
`M3-12-g6535f92` broji source miss samo dok decoder još nije na EOF-u; isti
fizički test zatim je završio s PCM deltom 0/0. Zasebni STOP/reload rub također
je ostao na 0/0.

Završni USB2 A/B test prvo je na `M3-12-g6535f92` reproducirao još jedan
disconnect-only deadline miss od 27.658 µs. Canceled/no-device callbackovi
stizali su prije `DEV_GONE` događaja dok je USB task još bio na aktivnom
prioritetu, a callback je mogao pripremiti i ponovno predati transfer.
`M3-13-gc95bd4b` na prvom takvom terminalnom statusu odmah spušta task na
transition prioritet i prekida resubmit. Ponovljeni fizički unplug/replug uz
utišani mixed-rate dual-deck playback vratio je FLX4 MIDI In/Out i UAC, oba
decka nastavila su playback, a output-late, PCM underrun, UAC dropped/overflow
i service-log dropped ostali su 0. Time su disconnect i EOF rubovi zatvoreni.

Fizički multi-client gate zatvoren je na `M3-15-g70a082c` s Windows računalom
i mobitelom istodobno spojenima na `Pajoniiir-M3`. Početni run otkrio je još
jedan nepotreban WARN iz priority-7 isochronous callbacka svake 1.000 transfera;
log i njegova privatna telemetrija uklonjeni su iz real-time puta. Završni
190,722-sekundni mixed-rate dual-deck prozor prošao je 180/180 status, 18/18
library i 12/12 firmware zahtjeva, uz neprekinut FLX4 MIDI In/Out i UAC. Oba
decka napredovala su po 190.832 ms, a output-late, PCM underrun, UAC
dropped/overflow/underflow i service-log dropped delte ostale su 0. Prozor je
uključio i kratko fizičko okretanje `HEADPHONES LEVEL`; nije nastala audio
regresija. Mobitel je zasebno prošao AP disconnect/reconnect i ponovno prikazao
library i oba PLAY decka. Vizualna provjera Settings broja klijenata ostaje uz
bring-up novog zaslona.

Servisna mreža konfigurirana je 2026-08-23 kroz web UI i trajno spremljena u
NVS, bez hardkodiranja ili izlaganja zaporke. Prva implementacija koja je gasila
AP prije STA posjeta vratila bi beacon, ali ne i SDIO podatkovni put/DHCP; Windows
je završavao na link-local adresi. Pokušaj punog ESP-Hosted restarta dodatno je
potvrdio da je teardown zabranjen dok microSD koristi drugi slot istog SDMMC
kontrolera. `M3-20-g9f24b19` zato koristi APSTA: SoftAP netif, DHCP, HTTP/DNS i
Hosted ostaju živi, a privremeno se dodaje samo STA netif. Fizički connectivity
probe završio je s `round trip complete`, STA adresom `192.168.0.210` i očuvanom
AP adresom klijenta `192.168.4.2`. Zasebni HTTPS OTA check pročitao je kanal i
ispravno odbio stariju objavljenu verziju bez preuzimanja ili flashanja. FLX4
MIDI In/Out/UAC, 191-track library i service-log dropped=0 ostali su uredni.
Puni noviji signed-bundle install, download fault i rollback ostaju otvoreni.
Probe zahtjev tijekom utišanog dual-deck playbacka zasebno je ispravno odbijen
s HTTP 400 `a deck is playing`: oba decka nastavila su napredovati, AP je ostao
na `192.168.4.2`, a output-late, PCM underrun, UAC drop/overflow i service-log
drop brojači ostali su 0. OTA acceptance zato se izvodi samo sa zaustavljenim
deckovima; aktivni playback acceptance provjerava fail-closed odbijanje bez
mrežnog prijelaza.

Izolirani predmjerni PCM D1=202 start događaj dodatno je provjeren bez promjene
firmwarea. Sedam kontroliranih no-seek startova, uključujući tri cold boota i
četiri dual-deck redoslijeda, te pet zasebnih load → paused seek na 100.000 ms →
simultaneous start → stop ciklusa završili su s PCM D1/D2=0/0 i bez UAC
dropped/overflow događaja. U dva od pet seek ciklusa nastao je po jedan
izolirani output-late pri startu, ali bez PCM ili UAC posljedice. PCM tranzijent
zato nije potvrđen kao reproducibilan kvar; brojači ostaju dio dužeg soak
monitoringa.

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

- [x] hardware-verify PFL prije channel fadera, cue/master routing te
  `HEADPHONES LEVEL`/`HEADPHONES MIX` bez prekida ili pucketanja;
- [ ] hardware-verify da simultani dual-deck start i seek/start prijelazi ne
  povećavaju PCM underrun brojače nakon prebuffera;
- [ ] hardware-verify PCM5102A headroom i završnu limiter marginu;
- dovršiti scratch/Master Tempo rubne slučajeve uz loop i pitch promjene;
- postaviti pragove za UAC ring i output timing alarme.

### 7. Release hardening

- P4-only reproducibilni clean build i OTA package gate;
- UI screenshot baseline nakon vizualne provjere Settings promjene i novog
  800×480 zaslona;
- ažurirani startup smoke, risk register i release validation zapis.
