# Startup Checklist

Status: važeći bench postupak, 2026-08-24.

## Priprema

- [ ] P4 je napajan preko USB1/CH340C.
- [ ] DDJ-FLX4 je na USB2 FS Host portu.
- [ ] Rekordbox medij je na USB3 HS Host portu.
- [ ] PCM5102A je spojen na GPIO1/2/3 i zajednički GND.
- [ ] ESP-IDF profil javlja točno `ESP-IDF v6.0.2`.

## Build i flash

```powershell
. C:\Espressif\tools\Microsoft.v6.0.2.PowerShell_profile.ps1
idf.py --version
Set-Location firmware\main-deck-p4
idf.py build
idf.py -p COM17 flash monitor
```

- [ ] Build završi exit kodom 0.
- [ ] Boot nema reset loop, abort ni watchdog.
- [ ] DSI UI i FT5426 touch rade u 800×480 landscapeu.

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
- [ ] `SHIFT + PLAY/PAUSE` drži Censor state i LED samo do otpuštanja. Trenutačni
  MVP čujno ponavlja dio oko 1 s unatrag i zatim se vraća na napredovalu
  vremensku liniju; nije pravi reverse.

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

- [ ] Rekordbox medij se mounta, library se učita i track se može loadati na oba
  decka.
- [ ] USB3 remove/reinsert sa zaustavljenim deckovima invalidira stari library,
  povećava media generation i ponovno učita kolekciju bez P4 ili FLX4 reseta.

## Audio

- [ ] Master L/R izlazi preko PCM5102A bez speaker-amp šuma.
- [ ] Kanalni CUE/PFL ostaje čujan sa spuštenim channel faderom kada je
  `HEADPHONES MIX` na CUE; MASTER strana prati post-fader master.
- [ ] Višestruko okretanje `HEADPHONES LEVEL` ne pucketa, ne prekida playback i
  ne povećava output-late, PCM underrun ni UAC drop/overflow brojače.
- [ ] Nema kontinuiranih UAC dropova, underruna ili clippinga.
- [ ] UAC je bez aktivnog dropa/overflowa i s 44,1-kHz i s 48-kHz izvornim
  trakama; promjena source ratea ne mijenja brzinu ni visinu tona.
- [ ] Prirodni EOF i STOP/reload na mixed-rate deckovima ne povećavaju PCM
  underrun brojače.
- [ ] Simultani start oba spremna decka i seek/start nakon prebuffera ne
  povećavaju PCM underrun brojače.
- [ ] Fizički USB2 unplug/replug tijekom utišanog dual-deck playbacka vraća
  MIDI In/Out i UAC, playback nastavlja, a output-late i PCM underrun ostaju 0.
- [ ] Dual-deck playback, pitch/Master Tempo i scratch ostaju stabilni.

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
