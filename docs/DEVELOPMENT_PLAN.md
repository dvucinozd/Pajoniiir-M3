# Development Plan

Status: plan nakon mixed-rate, FLX4 headphone, disconnect/EOF, signed P4 OTA,
Beat Jump, Loop Adjust/Quantize, shifted transport/sync, Beat FX te UAC health
acceptance blokova, 2026-08-24.

## Trenutna baza

- jedan ESP32-P4 firmware target;
- izravni FLX4 USB MIDI In/Out i UAC1 headphone put;
- zaseban USB3 MSC put za Rekordbox medij;
- dual-deck audio, DSP, UI, Wi-Fi remote i potpisani P4 OTA;
- uklonjeni pomoćni kontrolni firmware, UART/bulk protocol, `.s3bin` profili,
  peer debug/OTA i međupanački PCM link.

Wi-Fi programski temelj već postoji: ESP32-C6/ESP-Hosted preko SDIO-a,
SoftAP `Pajoniiir-M3`, web API, privremeni STA put i potpisani P4 OTA. Prije
releasea preostaje hardverska/stress validacija; rollback, download fault i
post-OTA equality dijagnostički acceptance su zatvoreni.

Ciljani 5,0-inčni MIPI-DSI zaslon (800×480, nativni landscape) s FT5426
dodirom je naručen i čeka se njegov dolazak. U dokumentaciji je to ciljana
konfiguracija; hardverski bring-up još nije potvrđen.

Prvi korisni firmware nakon `RC2` uvodi release identitet `M3`. Verzija se
dobiva iz `git describe`, pa je implementacijski commit namijenjen anotiranom
`M3` tagu. `p4_ota_pull` parser sada prihvaća `RC<number>` i `M<number>`, tretira
M porodicu kao monotono noviju od RC porodice te ima migracijske OTA testove za
`RC2` → `M3`. Clean build na tagu mora prijaviti točno `M3`.

## Handoff za sljedeću sesiju

Završni hardverski baseline 2026-08-24 je produkcijski `M3-39-g3bc04fd` u
`ota_1` na P4 ploči. Potpisani lokalni web OTA prihvatio je bundle od
2.365.872 B, podigao novi image softverskim resetom i vratio OTA API u `idle`.
FLX4 je spojen s MIDI In/Out i UAC-om, USB3 library sadrži 191 track, oba decka
imaju učitane trake i zaustavljena su, service log nema dropova, a SoftAP i
Windows profil `Pajoniiir-M3` ostavljeni su uključeni.
Servisni SSID, zaporka i HTTPS update
URL spremljeni su u NVS-u; status izlaže samo SSID, URL i `has_password`, ne
zaporku. Fizički APSTA round-trip dobio je servisnu adresu `192.168.0.210`,
vratio `round trip complete` i sačuvao SoftAP klijentu valjanu adresu
`192.168.4.2`. HTTPS kanal zatim je objavio `M3-22-gd7466ea`; bundle je
preuzet, verificiran i podignut iz `ota_0`. Naknadna provjera jednake verzije
vratila je `already running this build` bez reseta, uz nepromijenjen boot ID
175. Jedan raniji, neposredni equality check nakon lokalnog web-upload OTA-a
uzrokovao je izolirani PANIC reset. Nije se ponovio u tri svježa post-OTA
equality ciklusa uz test-only flash-coredump build; sva tri checka vratila su
`already running this build`, bez neočekivanog reseta, a coredump particija
ostala je prazna. Događaj ostaje rezidualna monitoring stavka, ali više ne
blokira OTA acceptance. Izolirani testni kanal zatim je ponudio
`M4`, ali namjerno nije sadržavao bundle. Install je završio s
`bundle not on the server` na 0% bez reboota ili promjene `ota_0`; produkcijski
URL vraćen je u NVS, a boot ID ostao je 175.
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

1. instalirati i hardverski potvrditi novi gapless slip-reverse Censor na D1 i
   D2, uključujući release bez output-late/PCM/UAC delte;
2. započeti 800×480 DSI/FT5426 bring-up tek nakon dolaska i identifikacije novog
   zaslona, pa na istom UI-u odraditi Master Tempo hardware gate;
3. potvrditi PCM5102A headroom/limiter marginu nakon fizičkog spajanja DAC-a.

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
- [x] potvrditi potpisani P4 OTA preko STA veze, uključujući download, provjeru
  potpisa, boot novog slota, health potvrdu i obnovu SoftAP-a;
- [x] potvrditi missing-bundle download fault bez reboota ili promjene slota;
- [x] potvrditi firmware-health rollback scenarij.

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
Potpisani `M3-22-gd7466ea` (`rel-001`) verificiran je prvo izravnim dohvatom s
VPS-a, zatim lokalnim web uploadom i konačno pravim pull installom iz factory
`M3-20-g9f24b19` u `ota_0`. Pull je preuzeo 2.361.984 B imagea, verificirao ga,
rebootao sa SW razlogom i startup health gate označio ga je valjanim. NVS,
191-track library, FLX4 MIDI In/Out/UAC i SoftAP ostali su očuvani; output-late,
PCM underrun, UAC drop/overflow i service-log dropped ostali su 0. Kontrolirani
same-version check nakon stabilizacije vratio je `already running this build`
bez promjene boot ID-a 175. Zasebni `/ota/ota-test/` kanal ponudio je `M4`,
ali je relativna bundle putanja namjerno vraćala HTTP 404. Install je došao do
`downloading 0%`, zatim ispravno objavio `bundle not on the server`, obnovio AP
i ostao na `ota_0 / M3-22-gd7466ea` bez reboota; boot ID ostao je 175. FLX4,
191-track library, NVS i svi audio/service brojači ostali su uredni, a
produkcijski `/ota/` URL vraćen je u NVS.
Namjerni health-failure image `M3-24-g3b2dc41` zatim je lokalnim potpisanim
uploadom podignut iz `ota_1` kao `pending_verify`. Test-only startup gate
restartao ga je prije potvrde, nakon čega je bootloader automatski vratio
`ota_0 / M3-22-gd7466ea` u stanju `valid`. SoftAP, produkcijski OTA URL,
spremljena zaporka i 191-track library ostali su očuvani, a service-log dropped
ostao je 0. FLX4 je u završnom snapshotu bio namjerno odspojen.
Jedan neposredni same-version check nakon ranijeg lokalnog upload OTA boota
uzrokovao je izolirani PANIC reset bez dostupnog coredumpa; ponovljene odgođene
provjere nisu ga reproducirale. Točno verzionirani test-only flash-coredump
image zatim je tri puta svježe podignut preko oba OTA slota. Neposredni checkovi
svaki su put završili s `already running this build`, bez PANIC-a ili dodatnog
reseta; coredump particija na `0xc20000` ostala je prazna (`0xFFFF` zaglavlje).
Nakon testa vraćen je potpisani produkcijski `M3-22-gd7466ea` u `ota_1`, health
gate ga je označio valjanim, a SoftAP, NVS i 191-track library ostali su uredni.
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

Produženi hardware acceptance 2026-08-24 dodao je 600,502-s mixed-rate
dual-deck prozor uz istodobni USB3 read, FLX4 UAC/MIDI i Wi-Fi web promet.
Prošlo je 643/643 status, 64/64 library i 42/42 firmware zahtjeva; oba decka
napredovala su po 600.528 ms. PCM D1/D2, UAC dropped/overflow/aktivni underflow
i service-log dropped delte ostale su 0. Četiri rijetka output-late događaja
imala su maksimum 11.659 µs uz prag 10.668 µs, bez audio ili USB posljedice.

Deset novih load → paused seek → dual start → stop ciklusa imalo je 10/10
čistih aktivnih prozora. Tri dodatna fazna ciklusa pokazala su da marginalni
late događaji nastaju samo pri gotovo istodobnom dual startu, ne pri loadu,
seeku, stabilnom playbacku ili stopu; PCM/UAC su ostali 0. Povijesni D1=202
događaj time se nije ponovio u ukupno 25 kontroliranih startova, od kojih je 18
uključivalo load i paused seek. Završni FLX4 unplug/replug pod playbackom vratio
je puni MIDI In/Out i UAC za 5,597 s, bez output-latea ili PCM/UAC greške, a oba
decka nastavila su napredovati. Detalji su u
`docs/validation/P4_DUAL_USB_AUDIO_SOAK_20260824.md`.

Završni storage gate fizički je izvadio i vratio USB3 Rekordbox medij sa
zaustavljenim deckovima. Library je ispravno prošao 191 → 0 → 191 track,
generation `1→2→3`, a puna obnova trajala je 7,405 s. FLX4, Wi-Fi i firmware
ostali su aktivni; output-late, PCM, UAC i service-log dropped delte bile su 0.
Service journal potvrdio je `USB_UNMOUNTED`, `USB_MOUNTED` i `LIBRARY_LOADED`
unutar istog boot ID-a 192. Time je dual-USB/storage blok za testirani profil
zatvoren.

Kasniji clean build `M3-28-g809c203` razotkrio je da je per-controller DWC FIFO
raspodjela ranije živjela samo kao ignored izmjena u `managed_components`:
FLX4 USB2 MIDI/UAC radio je nakon OTA-a, ali USB3 disk nije proizveo ni attach
događaj. `M3-29-g2b0ad21` preselio je istu HS/FS raspodjelu u verzionirani
CMake overlay koji iz netaknute `espressif/usb 1.5.0` datoteke generira patched
source pod `build/`. Puni `idf.py fullclean` + 1865-step build prošao je,
originalni HCD zadržao je očekivani SHA-256
`de0471a749547c7d295af0fe2e3e5b61d1eedf46d88c5b57cf20cec202d6c749`, a
post-commit OTA odmah je montirao USB3 i učitao svih 191 track.

Na istom imageu hardverski je zatvoren shifted Beat Jump blok. D1 zadani `+1`
slijedio je stvarni grid (`0→731→1201 ms`), velika stranica dala je D1 `+16`
skok `1201→8724 ms`, a D2 ju je bez lokalnog odabira naslijedio i skočio na
7931 ms. Frakcijski D1 `+1/16` pomak bio je 30 ms pri 128 BPM. Operator je
potvrdio shifted mirror 1-6 te helper granice: default oba helpera ON, large
pad 8 OFF i fractional pad 7 OFF. Globalna stranica na kraju je vraćena na
default; output-late, oba PCM underruna te UAC dropped/overflow ostali su 0.

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

- [x] istodobno streamati dva decka s USB3 dok FLX4 MIDI/UAC radi na USB2;
- [x] mjeriti output deadline, cache miss, USB recovery i headphone drop brojače;
- [x] ponoviti FLX4 connect/disconnect tijekom playbacka;
- [x] potvrditi USB3 zamjenu medija i obnovu libraryja tijekom sigurnog
  transport stanja.

Acceptance: nema audio artefakata, deadlocka ni reset loopa u dugom soaku.

### 5. MIDI/LED feature parity

- [x] implementirati i host-testirati shifted mirror LED izlaz za Hot Cue,
  Pad FX1, Pad FX2 i Beat Loop, uz reconnect queue kapacitet za puni snapshot;
- [x] hardware-verify shifted mirror LED izlaz za sva četiri moda na oba FLX4
  decka;
- [x] uskladiti normalni Beat Jump raspored s XML/Mixxx mapom te host-testirati
  globalne frakcijsku, zadanu i veliku stranicu (`SHIFT` + pad 7/8),
  saturaciju, frakcijske seekove i granične helper LED-ice;
- [x] hardware-verify ispravljeni Beat Jump redoslijed, zajedničku stranicu na
  oba decka, shifted mirror LED-ice padova 1-6 i granično gašenje helpera 7/8;
- [x] implementirati persistentni `SHIFT + LOOP IN/OUT` adjust mod koji troši
  jog događaje, uređuje samo odabranu granicu i drži odgovarajuću LED-icu;
- [x] hardware-verify D1 Loop Adjust In/Out, D2 Loop Adjust Out adresu `0x4E`,
  neovisni D1/D2 Quantize toggle i beat-grid snap Loop In/Out granica;
- [x] host-testirati D1/D2 shifted Censor, Sync Master i Reloop Stop/Forget
  adrese te izložiti `sync_master` i `censor_active` kroz status API;
- [x] hardware-verify D1 Sync Master long press, obični D2 Sync prema masteru,
  D1 Reloop Stop/Forget te D1 Censor state, LED i čujni MVP repeat;
- [x] zamijeniti seek-based Censor bounded gapless slip-reverse čitačem nad
  kanonskim PCM timelineom, uz 10-ms release crossfade, mixed-rate interpolaciju
  i host ugovor da `deck_core` ne radi seek;
- [ ] hardware-verify novi Censor na D1/D2: čujni reverse tijekom držanja,
  nastavak s napredovale slip pozicije i nulta output-late/PCM/UAC delta;
- [x] hardware-verify shifted Beat FX beat-size dvostruke korake i saturaciju,
  FLANGER/DELAY DSP te potpuni shifted reset statea i ON/OFF LED-ice;
- [x] hardware-verify D1 ANLZ-grid ponašanje shifted CUE/LOOP CALL back/forward
  i inertni safety behavior Shift + Smart CFX/Fader kontrola;
- [ ] nakon dolaska zaslona hardware-verify Shift + Browse force-open/ubrzano
  kretanje i Shift + Load D1/D2 routing;
- proći preostale redove u `DDJ_FLX4_MIDI_MAP.md` izravno iz XML reference;
- za svaku kontrolu dodati input behavior i LED reconnect test;
- ukloniti zastarjele numeričke semantičke ID-jeve tek nakon pokrivanja.

Acceptance: svi podržani FLX4 elementi imaju jednoznačan P4 state owner.

`M3-34-gafee129` zatvorio je shifted transport/sync ulazni blok. D1 Sync
Master zahtijevao je najmanje 3 s držanja; potom je normalni D2 Sync pratio D1
master. D1 Reloop Stop/Forget ugasio je petlju i obični Reloop/Exit je nije
vratio. Censor state, LED i čujno kratko ponavljanje prošli su, ali postojeći
seek-based MVP pri pressu i releaseu dodaje po jedan output-late događaj i 256
PCM-underrun frameova. Kontrolni start/stop bez Censora imao je nultu deltu.
Ta je implementacija sada zamijenjena gapless slip-reverse DSP-om bez seeka:
postojeći četverosekundni timeline daje reverse povijest, forward renderer
istodobno pomiče autoritativni playhead, a release ih linearno ukršta tijekom
10 ms. Novi modul pokriva unity/mixed-rate reverse, interpolaciju, bounded rub i
release; deck-core regresija potvrđuje nula seekova. Puni host suite i ESP-IDF
6.0.2 P4 build prošli su 2026-08-24, dok hardware acceptance ostaje sljedeći
screen-independent gate.

Shifted Beat FX hardverski slice zatim je na istom imageu prošao `1→4`, gornju
saturaciju, `4→1→1/4`, donju saturaciju i povratak na `1 beat`, bez promjene
audio/USB brojača. FLANGER je imao očekivani sweep i ON LED; live prijelaz na
DELAY dao je čujan 470-ms one-shot tap. Shifted ON/OFF reset vratio je
`FILTER / 1 beat / BOTH / depth 64 / OFF`, ugasio DSP i fizičku LED. Jedan
izolirani 14.714-us output-late tijekom live FX prozora nije imao PCM/UAC
posljedicu. Sljedeći parity rez su globalni shifted Browse/Load i Smart
CFX/Fader helperi.

Sljedeći screen-independent smoke zatvorio je D1 shifted CUE/LOOP CALL:
`30000→29574→30058 ms`, odnosno jedan forward beat od 484 ms pri 124 BPM,
bez release duplikata ili audio/USB counter delte. Shifted Smart CFX/Fader
ostali su programski i fizički OFF, u skladu s namjernim no-op dizajnom.
Browse/Load helperi ne mogu dobiti smislen eyes-on acceptance bez zaslona i
odgođeni su do display bring-upa.

Screen-independent release hardening zatim je dobio čistu, host-testiranu UAC
health politiku. Clock-correction deadband ostaje 3/8–5/8 ringa, a upozorenje se
otvara tek izvan šireg 1/4–3/4 omotača tijekom stvarnog playbacka. Nove
drop/overflow/underflow delte agregiraju se i zapisuju kao `UAC_DATA_LOSS`, a
trajni niski ili visoki tlak kao `UAC_RING_PRESSURE`; oba zapisa su ograničena
na najviše jedan sažetak u minuti nakon prvog događaja. Idle underflow se samo
upija u baseline, pa zaustavljeni deckovi ne proizvode lažne alarme. Status API
izlaže pragove, `ring_state` i kumulativni drop/overflow `data_loss` indikator.
Output-late prag ostaje precizno testiran na dva 256-frame bloka: 10.668 us pri
48 kHz i 11.610 us pri 44,1 kHz. Novi modul, servisni događaji, API ugovor, puni
host suite i ESP-IDF 6.0.2 P4 build prošli su 2026-08-24; hardware status smoke
zatvoren je na `M3-39-g3bc04fd`.

Prvi `M3-38-gf944ee2` hardware pokušaj potvrdio je pragove 512/1536 i stabilan
`nominal→idle` status, ali je servisni log otkrio lažni `UAC_DATA_LOSS` od
13.582 framea: interval između zadnjeg idle uzorka i prvog aktivnog uzorka
obuhvatio je UAC zero-fill prije ring priminga. `M3-39-g3bc04fd` zato prvi
aktivni uzorak koristi samo kao novi baseline, dok sve kasnije aktivne delte i
dalje alarmira. Ponovljeni utišani single-deck test držao je ring 12 s u
`nominal` rasponu 985–1338, a 48/44,1-kHz dual-deck test 15 s u rasponu
948–1301. Oba STOP-a vratila su `idle`; UAC drop/overflow, output-late, oba PCM
underruna, service-log dropped i novi UAC incidenti imali su nultu deltu.

### 6. Audio acceptance

- [x] hardware-verify PFL prije channel fadera, cue/master routing te
  `HEADPHONES LEVEL`/`HEADPHONES MIX` bez prekida ili pucketanja;
- [x] hardware-verify da simultani dual-deck start i seek/start prijelazi ne
  povećavaju PCM underrun brojače nakon prebuffera;
- [ ] hardware-verify PCM5102A headroom i završnu limiter marginu nakon što
  korisnik fizički spoji DAC modul;
- [x] hardware-verify active-loop scratch i pitch-fader handoff;
- [x] implementirati i host-testirati gapless slip-reverse Censor bez transport
  seeka ili drugog PCM buffera;
- [ ] hardware-verify Censor reverse/release na oba decka uz nulte output-late,
  PCM-underrun i UAC drop/overflow delte;
- [ ] nakon dolaska zaslona uključiti Master Tempo kroz UI i izmjeriti stvarni
  dual-deck keylock/PSRAM deadline uz suprotne pitch vrijednosti;
- [x] postaviti pragove za UAC ring i output timing alarme, uz idle suppression,
  rate-limitirani servisni log i status API observability;

### 7. Release hardening

- P4-only reproducibilni clean build i OTA package gate;
- UI screenshot baseline nakon vizualne provjere Settings promjene i novog
  800×480 zaslona;
- ažurirani startup smoke, risk register i release validation zapis.
