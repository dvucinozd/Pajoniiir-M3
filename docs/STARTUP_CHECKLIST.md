# Startup Checklist

Status: važeći bench postupak, 2026-09-01.

Aktualni bench image je app-only `M3-48-g435bcfe-dirty / factory`, SHA-256
`2CDAB5D7C859F28F26E2BB02CDC5B711DA4A25480EA081B18A2C3EF963DF3455`;
`M3-41-g133f399 / ota_0` ostaje potpisani rollback baseline. FLX4, USB3,
PCM5102A, Wi-Fi, DSI slika, fokusirani touch, screensaver wake te corner/edge i
two-finger safety gate rade. Desetominutni zajednički i produženi
cold-power/reconnect integration gateovi prihvaćeni su.

- [x] MT 0% -> +5% -> -5% -> 0% potvrđen je na oba decka, a kratki simultani
  48/48-kHz gate s D1 +5 % i D2 -5 % prošao je uz čist zvuk, fluidan waveform,
  stabilan zaslon i nulte PCM/UAC drop/overflow delte. Novi build ispravlja
  akumulaciju grain offseta i reproduciranu PCM producer/consumer cursor utrku
  te ograničava correlation rad hijerarhijskom pretragom.
- [x] Isti dual-deck MT gate s 44,1/48-kHz izvorima prošao je bez novih
  output-late događaja, PCM underruna ili UAC drop/overflowa; operator je
  potvrdio zvuk, waveform i zaslon. Brojilo položaja nije dokaz audio-tempa.
  Vidi
  [validation zapis](validation/2026-08-31-master-tempo-response.md).

## Priprema

- [ ] P4 je napajan preko USB1/CH340C.
- [ ] DDJ-FLX4 je na USB2 FS Host portu.
- [ ] Rekordbox medij je na USB3 HS Host portu.
- [ ] PCM5102A ima `BCK=GPIO1`, `LCK=GPIO2`, `DIN=GPIO3`, `SCK=GND`, zajednički
  GND i `VIN=5V`; mostovi su `H1=L`, `H2=L`, `H3=H`, `H4=L`.
- [ ] EYOYO `DSI506 / DYL0023` FFC je potpuno umetnut na J2 u potvrđenoj
  orijentaciji; kabel se ne premješta pod napajanjem.
- [ ] Display koristi tvornički backlight put preko `0x45`; vanjski `PWM/GND`
  nije spojen, 0-ohm selektor nije premješten i `FAN 3V3` nije ulaz napajanja.
- [x] FT5426 touch na `0x38` radi bez read grešaka na 100 kHz; prihvaćene su
  obje mirror osi bez `swap_xy`.
- [ ] ESP-IDF profil javlja točno `ESP-IDF v6.0.2`.

## Build i flash

```powershell
. C:\Espressif\tools\Microsoft.v6.0.2.PowerShell_profile.ps1
idf.py --version
Set-Location firmware\main-deck-p4
idf.py build
idf.py -p COM6 flash monitor
```

- [ ] Build završi exit kodom 0.
- [ ] Boot nema reset loop, abort ni watchdog.
- [ ] Boot izravno prikazuje 800×480 GUI bez testnih traka, bijelog kadra,
  horizontalnog wrapa, zrcaljenja ili zamjene crvene/plave.

## Display i touch

- [x] EYOYO `DSI506 / DYL0023` prikazuje 800×480 RGB888 u nativnom landscapeu.
- [x] Aktivni profil je 1 lane / 800 Mbps, 27,777 MHz,
  HFP/HSW/HBP `59/2/45`, VFP/VSW/VBP `109/2/22` (50,0146 Hz), burst sync pulses i bez frame
  ACK-a.
- [x] Solo D1 i istodobni D1+D2 waveform ostaju oštri i fluidni; dual-deck PPA
  blit slijedi fizički scanout odozgo prema dolje, bez bljeskanja ili audio
  posljedice. Vidi
  [validation zapis](validation/2026-09-01-dsi506-waveform-sync.md).
- [x] Desetominutni kombinirani display/touch/master/headphones/dual-deck/
  USB3/Wi-Fi soak završava bez fizičkog artefakta i bez PCM/UAC/service-log
  loss delte. Pet output-late događaja do 12522 us ostaje monitoring nalaz,
  ne zero-late tvrdnja. Vidi
  [validation zapis](validation/2026-09-01-integration-soak.md).
- [x] `OVERVIEW` je prvi, `SETTINGS` zadnji tab; nema cikličkog omatanja ruba.
- [x] Boje GUI-ja su fizički potvrđene, a PPA `rgb_swap` i horizontalni mirror
  ostaju ugašeni.
- [x] Backlight se uključuje preko `0x45`, Settings/postotak koristi isti put,
  a lokalni gumb korak-po-korak mijenja svjetlinu.
- [x] Touch read radi bez `panel_io_i2c_rx_buffer` / FT5x06 I2C grešaka.
- [x] Sve četiri kartice, Backlight drag te lijeva/desna Overview kontrola
  reagiraju na odgovarajućem mjestu bez swap/mirror greške.
- [x] Gornji lijevi/desni i donji lijevi/desni aktivni rub reagiraju ispravno;
  dva istodobna dodira ne uzrokuju ghost akciju, stuck press ni nestabilnost.
  Vidi [validation zapis](validation/2026-09-01-touch-edge-multitouch.md).
- [x] Library, Master Tempo i Shift + Browse/Load prolaze eyes-on/touch
  acceptance; oba shifted load puta mijenjaju samo ciljani zaustavljeni deck.
- [x] Screensaver se budi bez propuštanja prvog lokalnog PLAY događaja;
  sljedeći PLAY radi, a dismissing touch ne aktivira kontrolu ispod. Vidi
  [validation zapis](validation/2026-09-01-screensaver-wake.md).
- [x] D1/D2 target selektori vidljivi su prije prvog dodira; D1 Hot Cue A
  odmah prikazuje fizički set/shifted-clear, recall skače na spremljeno vrijeme,
  a cue se vraća nakon reboota i ponovnog učitavanja iste trake. Vidi
  [validation zapis](validation/2026-09-01-hot-cue-ui.md).
- [x] Settings sadržaj i Backlight slider prošli su eyes-on/touch gate; svih
  sedam 800×480 screenshot hashova prolazi, a `settings` i
  `settings_restored` identični su. Vidi
  [validation zapis](validation/2026-09-01-settings-ui.md).
- [x] Dugi display/touch/PSRAM i cold-power/reconnect gate nema tearing,
  artefakte, I2C poplavu loga, neželjeni reset ni audio/USB posljedicu. Vidi
  [validation zapis](validation/2026-09-01-cold-power-reconnect.md).

Display/touch acceptance 2026-08-31 koristio je COM6 i završni app-only image
od 2.369.840 B. Puni host suite i ESP-IDF 6.0.2 build prošli su; boot je učitao
191 traku za približno 4 s i odmah prešao u pravilno poravnati GUI. Fokusirani
touch gate je zatvoren. Screensaver wake i corner/two-finger safety prihvaćeni
su na kasnijem app-only imageu iz zaglavlja. Desetominutni zajednički i
produženi cold-power/reconnect gateovi su zatvoreni.

## USB i kontrola

- [ ] FLX4 se enumerira na P4 kao MIDI/UAC uređaj.
- [ ] Nakon clean dependency builda istodobno se enumeriraju USB2 FLX4 i USB3
  MSC; build koristi generirani `build/pajoniiir_usb/hcd_dwc.c`, a
  `managed_components/espressif__usb` prolazi Component Manager hash provjeru.
- [ ] Play, Cue, jog, tempo, faderi, EQ i browse daju očekivani P4 state.
- [ ] LED feedback prati state i potpuno se obnovi nakon reconnecta.
- [ ] Hot Cue, Pad FX1, Pad FX2 i Beat Loop pad state jednako se prikazuje na
  normalnim i SHIFT LED adresama na oba decka.
- [ ] Beat Jump zadana stranica slijedi redoslijed `−1/+1`, `−2/+2`, `−4/+4`,
  `−8/+8`; `SHIFT` + pad 7/8 globalno bira frakcijsku/zadanu/veliku stranicu
  na oba decka, padovi 1-6 imaju SHIFT LED mirror, a helper 7/8 se ugasi na
  donjoj/gornjoj granici.
- [ ] `SHIFT + LOOP IN/OUT` uključuje persistentni deck-local adjust mod;
  odgovarajuća LED ostaje upaljena, jog mijenja samo odabranu granicu petlje,
  a position/scratch/bend se tijekom uređivanja ne pomiču.
- [ ] `SHIFT + channel CUE` neovisno uključuje Quantize na D1/D2 bez promjene
  PFL-a; normalni Loop In/Out izvan grida poravnava obje granice na ANLZ beatove.
- [ ] BEAT SYNC hold od najmanje 3 s postavlja deck kao Sync Master; obični
  kratki Sync na drugom decku prati master, dok kraći hold ne glumi long press.
- [ ] `SHIFT + RELOOP/EXIT` zaustavlja i zaboravlja aktivnu petlju; naknadni
  obični Reloop/Exit ne obnavlja zaboravljenu petlju.
- [ ] `SHIFT + PLAY/PAUSE` tijekom aktivnog playbacka drži Censor state i LED,
  čujno reproducira zadržani PCM unatrag te se na otpuštanje 10-ms crossfadeom
  vraća na napredovalu slip vremensku liniju bez seeka. Na pauziranom decku
  pritisak je siguran no-op.
- [ ] `SHIFT + BEAT < / >` mijenja Beat FX veličinu za dva enum koraka i
  saturira na `1/4` odnosno `4 beats`; FLANGER ima čujan sweep, DELAY jedan
  full-band tap, a `SHIFT + BEAT FX ON/OFF` vraća `FILTER / 1 beat / 1&2 /
  depth 64 / OFF` i gasi LED.
- [ ] `SHIFT + CUE/LOOP CALL < / >` pomiče za jednu stvarnu ANLZ beat-grid
  oznaku, a release ne radi drugi skok.
- [ ] `SHIFT + SMART CFX/FADER` ostaje namjerni no-op: ne mijenja normalni
  Smart state, DSP ni LED.

Posljednji Beat Jump hardware acceptance na `M3-29-g2b0ad21` potvrdio je
zadani `+1` grid skok, `+16` veliku stranicu podijeljenu između D1/D2,
frakcijski `+1/16` skok od 30 ms pri 128 BPM te očekivane helper LED granice.
Test je završio vraćanjem globalne zadane stranice.

Loop acceptance na `M3-31-g5565151` potvrdio je D1 In/Out i D2 Out adjust,
persistentne `0x4C/0x4E` LED-ice te nepomičan playhead tijekom jog uređivanja.
Quantize je na oba decka uključen preko `SHIFT + channel CUE`; namjerne sirove
D1 granice `1850/4230 ms` poravnate su na `1671/4022 ms`, bez promjene PFL-a,
output-latea, PCM underruna ili UAC drop/overflowa.

Shifted transport/sync acceptance na `M3-34-gafee129` potvrdio je D1 Sync
Master nakon najmanje 3 s držanja, obični D2 Sync prema D1 masteru i D1
Reloop Stop/Forget. D1 Censor state, LED i čujno kratko ponavljanje također su
prošli, uz poznatu MVP dijagnostičku cijenu: press i release seek dodali su po
jedan output-late događaj i 256 PCM-underrun frameova. Kontrolni osamsekundni
start/stop bez Censora nije dodao nijedan brojač, a UAC dropped/overflow ostali
su 0.

Seek-based izvedba više nije aktualni source. Potpisani `M3-41-g133f399` s
gapless Censorom instaliran je 2026-08-24 u `ota_0`. D1 48-kHz i D2 44,1-kHz
smoke potvrdili su čujni reverse, napredovanje forward playheada i gladak 10-ms
release bez seeka; status je na oba decka zabilježio
`censor_active false→true→false`. U oba kontrolirana Censor prozora output-late,
PCM-underrun, UAC drop/overflow i service-log drop delte bile su nula. Jedan
raniji output-late iz dugog običnog D1 playbacka ostao je nepromijenjen tijekom
oba testa.

Beat FX acceptance na istom `M3-34-gafee129` imageu potvrdio je shifted
`1→4`, gornju saturaciju, `4→1→1/4`, donju saturaciju i završni povratak na
`1 beat`, bez ijedne dijagnostičke delte. FLANGER state, ON LED i čujan sweep
prošli su; live prijelaz dao je čujan DELAY s 470-ms one-shot tapom. Shifted
reset vratio je sva zadana polja, ugasio DSP i fizičku LED. Tijekom live FX
prozora zabilježen je jedan izolirani output-late od 14.714 us bez PCM ili UAC
gubitka; odvojeni paused-seek/start prije FX-a dodao je jedan late i 264 D1 PCM
framea te se ne pripisuje samom efektu.

Na istom imageu D1 `SHIFT + CUE/LOOP CALL < / >` prošao je slijed
`30000→29574→30058 ms`; forward korak bio je 484 ms pri 124 BPM, oba releasea
ostavila su položaj stabilnim i svi audio/USB brojači ostali su nepromijenjeni.
Shifted Smart CFX/Fader zatim su potvrđeni kao inertni safety placeholderi:
oba normalna statea i obje fizičke LED-ice ostali su OFF.

- [ ] Rekordbox medij se mounta, library se učita i track se može loadati na oba
  decka.
- [ ] USB3 remove/reinsert sa zaustavljenim deckovima invalidira stari library,
  povećava media generation i ponovno učita kolekciju bez P4 ili FLX4 reseta.

## Audio

- [ ] Kada je DAC spojen, Master L/R izlazi preko PCM5102A bez speaker-amp šuma.
- [ ] Kanalni CUE/PFL ostaje čujan sa spuštenim channel faderom kada je
  `HEADPHONES MIX` na CUE; MASTER strana prati post-fader master.
- [ ] Višestruko okretanje `HEADPHONES LEVEL` ne pucketa, ne prekida playback i
  ne povećava output-late, PCM underrun ni UAC drop/overflow brojače.
- [ ] Nema kontinuiranih UAC dropova, underruna ili clippinga.
- [ ] `/api/status` za ring kapaciteta 2048 prikazuje pragove 512/1536;
  zaustavljeni deckovi daju `ring_state=idle` bez novog `UAC_DATA_LOSS` ili
  `UAC_RING_PRESSURE` zapisa, a stabilni playback daje `ring_state=nominal`.
- [ ] UAC je bez aktivnog dropa/overflowa i s 44,1-kHz i s 48-kHz izvornim
  trakama; promjena source ratea ne mijenja brzinu ni visinu tona.
- [ ] Prirodni EOF i STOP/reload na mixed-rate deckovima ne povećavaju PCM
  underrun brojače.
- [ ] Simultani start oba spremna decka i seek/start nakon prebuffera ne
  povećavaju PCM underrun brojače.
- [ ] Fizički USB2 unplug/replug tijekom utišanog dual-deck playbacka vraća
  MIDI In/Out i UAC, playback nastavlja, a output-late i PCM underrun ostaju 0.
- [ ] Dual-deck playback, pitch/Master Tempo i scratch ostaju stabilni.

Zaslon i stabilizirani touch sada su dostupni za Master Tempo hardverski gate.
MT treba uključiti kroz UI jer FLX4 nema zasebnu Master Tempo kontrolu.

PCM5102A acceptance 2026-08-26 potvrdio je oba kanala i tihi idle, 48-kHz
single-deck, 44,1-kHz single-deck, mixed-rate dual-deck te puni master bez
čujnog clippinga ili pumping efekta. Dual-deck full-master test imao je 4.090
limitiranih sampleova od približno 1.323.000 (oko 0,31 %, peak 48.584), a
single-deck 212 (oko 0,016 %); obje kontrolirane probe završile su s nultim PCM
underrun i UAC drop/overflow deltama. Dva izolirana output-late događaja tijekom
cijelog PCM bloka nisu imala audio posljedicu i ostaju za monitoring.

UAC health hardware acceptance na `M3-39-g3bc04fd` potvrdio je pragove
512/1536, `nominal` tijekom 12-s single-deck i 15-s 48/44,1-kHz dual-deck
playbacka te `idle` nakon STOP-a. Nije bilo novih UAC incidenata, drop/overflow,
output-late, PCM underrun ili service-log drop brojača. Prvi `M3-38` pokušaj
otkrio je i zatim regresijskim testom zatvorio lažni idle→PLAY baseline alarm.

## Mreža i servis

- [ ] C6/ESP-Hosted se inicijalizira.
- [ ] Settings razlikuje spremljeni Wi-Fi zahtjev od stvarnog
  OFF/STARTING/AP/STA/ERROR stanja i prikazuje aktualnu IP adresu.
- [ ] SoftAP `Pajoniiir-M3` prihvaća najmanje dva istodobna klijenta i broj
  klijenata na Settings ekranu prati connect/disconnect.
- [ ] Wi-Fi remote može se uključiti i vratiti u AP način nakon OTA probea.
- [ ] Servisni SSID, zaporka i HTTPS update URL čitaju se iz NVS-a; API i logovi
  izlažu samo `has_password`, nikada samu zaporku.
- [ ] Connectivity probe koristi APSTA, dobiva servisnu IPv4 adresu i završava s
  `round trip complete`; lokalni klijent cijelo vrijeme zadržava valjanu
  `192.168.4.x` adresu bez link-local fallbacka.
- [ ] HTTPS update check prihvaća samo usporediv noviji M3 release, a jednaku ili
  stariju objavu završava bez downloada i bez pisanja u flash.
- [ ] Probe/check/install zahtjev dok ijedan deck svira vraća HTTP 400
  `a deck is playing`, ne pokreće APSTA i ne prekida playback.
- [ ] P4 reset sa spremljenim Wi-Fi OFF stanjem ne ostavlja C6 ni SoftAP
  aktivnim.
- [ ] Zahtjev za gašenje tijekom probea/OTA-a izvrši se tek nakon obnove AP-a;
  nema racea, srušenog netifa ni izgubljenog ESP-Hosted transporta.
- [ ] `/api/status` prikazuje FLX4 i USB-headphone dijagnostiku.
- [ ] Prvi web control POST nakon lokalnog screensavera istodobno probudi UI i
  izvrši naredbu; ne zahtijeva drugi klik. Fizički FLX4 wake pritisak i dalje se
  samo potroši na buđenje kako ne bi nenamjerno pokrenuo deck.
- [ ] P4-only potpisani OTA odbija pogrešan chip, projekt ili potpis.
- [ ] Pull OTA sa zaustavljenim deckovima preuzima ponuđeni noviji release,
  podiže novi slot, prolazi startup health gate, čuva NVS/library/FLX4 i vraća
  SoftAP bez output-late, PCM underrun, UAC drop/overflow ili service-log dropa.
- [ ] Same-version channel check završava s `already running this build` bez
  reseta ili panica; boot ID ostaje nepromijenjen.
- [ ] Nepostojeći ili prekinuti bundle završava jasnom download greškom, ne
  mijenja boot slot i vraća SoftAP bez reboota.
- [ ] Namjerno neuspjeli startup health gate vraća prethodni valjani slot.
- [ ] Test-only coredump build koristi zaseban overlay; produkcijski
  `sdkconfig.defaults` ne uključuje flash coredump ni prisilni rollback.
- [ ] Ako se PANIC dogodi, dump iz particije `coredump` pročita se uz ELF istog
  builda prije novog testa ili brisanja particije.
- [ ] APSTA/OTA posjet ne deinitializira ESP-Hosted dok je microSD montiran na
  drugom slotu zajedničkog SDMMC kontrolera.

Svaki hardware smoke zapisuje verziju firmwarea, wiring, COM port, medij,
rezultat i relevantne dijagnostičke brojače.
