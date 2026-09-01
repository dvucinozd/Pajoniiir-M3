# DSI506 / DYL0023 Display Bring-up and Acceptance

Status: display-image i fokusirani touch gate prihvaćeni su 2026-08-31, a
screensaver wake, corner/edge, two-finger safety, waveform sync, desetominutni
zajednički integration soak i produženi cold-power/reconnect gate 2026-09-01.
Aktualni zapis ima prednost nad pripremom od
2026-08-26 koja je
sačuvana ispod njega kao povijesna polazna točka.

## Aktualni dijagnosticki zapis (2026-08-31)

- Ploča je na COM6. Radni build koristi `bsp_p4_m3`; legacy `bsp_jc4880` je
  isključen iz linkanja zbog kompatibilnih imena simbola. Slika je hardverski
  prihvaćena, ali ovaj app-only build nije potpisani release.
- Dobiveni list specifikacija (`1788187609507.jpg`) navodi 800x480, LCD
  **RGB666**, ulaz modula **MIPI DSI**, touch **FT5426**, 3,3 V i najvise
  340 mA. LCD izlazni RGB format nije dokaz DSI ulaznog pixel formata.
- Fotografija `1788187695163.jpg` prikazuje neoznacene IC-e. TC358762 nije
  potvrden. ICN6211 je kandidat za identifikaciju, ne odabrani driver.
- Korisnik je dostavio tocni kupljeni artikl:
  [AliExpress 1005009787482043](https://www.aliexpress.com/item/1005009787482043.html).
  Naslov njegove otvorene kartice navodi EYOYO 5 Inch / 800x480 / IPS / DSI.
  Sadrzaj artikla nije procitan: tekstualni dohvat nije uspio, a browser
  site-safety pravilo blokira pristup. Zatrazena je snimka Description /
  Specifications i eventualnih manual/driver poveznica; naslov nije IC ID.
- Naknadno je dostavljen dvostranicni `PRODUCT MANUAL`, datoteka
  `E:/Downloads/S4c90c179a29742e8a6c2defe278b9d13n.pdf`, SHA-256
  `ED60667178478708AE9DA3390458ADB96BDA693665ED31A2AE25ABD319510702`.
  Tekst i obje renderirane stranice pregledani su. Stranica 1 navodi
  `Model: DSI506`, `Batch/Serial Number: DYL0023`, proizvodjaca
  `ShenZhen Sky Blue Ocean Technology Co.,Limited`, kontakt
  `cs@vvavvstore.com`. Dokument ne navodi DSI bridge IC, panel FPC model,
  lane count/rate, video timing niti init/register sekvencu.
  Stranica 2 sadrzi genericke camera/battery/USB-charging upute i napomenu da
  prodavac treba prilagoditi korake konkretnom proizvodu. Njenu uputu o 5-V
  USB punjenju **ne primjenjivati na DSI/J2 3,3-V napajanje**. Ovo je trag za
  kontakt dobavljaca, ne elektricna specifikacija niti potvrda bridgea.
- Fotografija kutije `1788190037036.jpg` potvrduje brend **EYOYO**, oznaku
  **DYL0023**, 800x480 IPS DSI i istog proizvodjaca kao PDF. Korisnik je
  dodatno prepisao slabije vidljivu oznaku `08-10 3A1-10`; njeno znacenje nije
  potvrdeno (ne proglasavati je PCB ili bridge revizijom). Naljepnica povezuje
  isporuceni artikl s PDF-om, ali ne navodi bridge IC ni video parametre.
- I2C prije power-sekvence ranije je odgovarao na `0x18`, `0x38`, `0x45`.
  Kontroler `0x45` vratio je `ID=0xC3`, `ID2=0x8B`, `PORTB=0x85` nakon
  POWERON. Te vrijednosti nisu identifikacija DSI bridge silicija.
- Pozadinsko osvjetljenje proradilo je nakon power/PWM sekvence na `0x45`.
  Vanjski GPIO23 PWM nije spojen i nije potreban za ovu dijagnostiku. Prilozeni
  manual trazi premjestanje 0-ohm selektora za vanjski PWM; zasad ne lemiti.
- Pokusaji TC358762 DSI/proxy inicijalizacije nisu dali testne trake ni UI;
  korisnik i Logitech kamera potvrdili su bijelu sliku. Proxy rezultat
  `0x8B8B` nije valjani dokaz citanja bridge ID-a.
- Aktualna dijagnostika uklanja nepotvrdene TC358762 proxy naredbe. Radi
  ogranicene address-only scanove `before-power-sequence`, `power-off`,
  `power-on-early` i `power-on-settled` (dodatnih 250 ms). Ako `0x2C` odgovara,
  cita samo ID registre `0x00..0x03`; ICN6211 match trazi tocno `C1 62 11`.
  Ostale adrese samo se prijavljuju, bez generickog register dumpa ili upisa.
  Neodziv `0x2C` ne iskljucuje bridge na privatnoj I2C sabirnici modula.
- Prihvaćeni DSI stream koristi 1 lane / 800 Mbps, 27,777 MHz,
  H front/sync/back `59/2/45`, V `109/2/22` (50,0146 Hz), RGB888 framebuffer i izlaz te
  burst sync pulses / bez frame ACK-a. To je empirijski prihvaćen profil ovog
  fizičkog DYL0023 primjerka, ne vendor specifikacija za sve revizije.
- Wi-Fi postavke i audio put ostaju netaknuti. Nije pokrenut audio/integration
  soak; oba decka trebaju ostati zaustavljena tijekom display dijagnostike.

### Završni image i prihvaćeni fokusirani touch gate

Prethodni display-only kandidat nakon uklanjanja privremenih testnih traka
(touch-fix image je dokumentiran odmah ispod):

- version string: `M3-45-g5bb55bc-dirty`;
- slot: `factory`, app-only flash na `0x20000`;
- veličina: 2.369.840 B (`0x242930`), 43% app particije slobodno;
- SHA-256:
  `52A324421F59BA6AA6E48B409FDA286E8BB6AA7086315C7EEF01813DC8DE437E`;
- ESP-IDF 6.0.2 build, `git diff --check` i puni
  `tests/run_p4_host_tests.ps1` završili su s exit 0;
- flash hash je verificiran, boot nema panic/reset loop, GUI se pojavljuje bez
  15-s/12-s/3-s dijagnostičkih odgoda, a USB3 učitava 191 traku oko 4 s;
- korisnik je potvrdio ispravne boje, nativnu orijentaciju, redoslijed tabova i
  uklonjeno horizontalno omatanje.

Fokusirani touch gate prihvaćen je na završnom čistom imageu
`M3-46-gee004d6-dirty`, app-only u `factory` na `0x20000`, veličine
2.369.840 B (`0x242930`) i SHA-256
`00A131B3CE5A1DB9B009007316A3940DA9EBD6E58864E2F25EC4CB2676742988`.
Flash hash je verificiran, a Wi-Fi je ostao uključen.

Uzrok ranijih FT5x06 runtime grešaka bio je pre-arrival BSP override od
400 kHz. Modul na `0x38` stabilno radi na 100 kHz; nakon promjene nema
`panel_io_i2c_rx_buffer()` ni FT5x06 read grešaka. Dijagnostičko čitanje dalo
je sirove ID bajtove `A3/A6/A8 = FF/0B/79`, koji se ne koriste kao dokaz točnog
IC modela. Raw sampler potvrdio je valjane koordinate i horizontalni swipe bez
read grešaka.

Prvi LVGL testovi zabilježili su fizičke dodire gornjih tabova kao
`Library x=297 y=458` i `Settings x=703 y=457`: X je već bio ispravno zrcaljen,
a Y je bio obrnut. Prihvaćena landscape transformacija zato je
`swap_xy=0`, `mirror_x=1`, `mirror_y=1`. Korisnik je nakon uklanjanja svih
privremenih dijagnostičkih logova potvrdio slijed Overview -> Library ->
Hot Cues -> Settings, Settings backlight slider dolje/gore te velike kontrole
na lijevoj i desnoj strani Overviewa. Time su I2C komunikacija, press/release,
osnovna koordinatna transformacija i fokusirana interakcija prihvaćeni.

Ovaj fokusirani PASS nije uključivao eksplicitni corner/multitouch test,
screensaver wake ni zajednički display/master/headphones/dual-deck/Wi-Fi
integration soak. Screensaver wake naknadno je prihvaćen 2026-09-01; vidi
[validation zapis](validation/2026-09-01-screensaver-wake.md).
Corner/edge i two-finger safety također su naknadno prihvaćeni; vidi
[validation zapis](validation/2026-09-01-touch-edge-multitouch.md). LVGL input
ostaje namjerno single-pointer, pa rezultat ne tvrdi dva neovisna kursora.

### Refresh-sinkronizirani waveform gate (2026-09-01)

Nakon što je korisnik na pokretnim glavnim waveformima prijavio efekt kao da
su linije u vodi, rigidnost cachea provjerena je kroz više rubnih dopuna i
obilazaka ring buffera. Problem je ostao ograničen na fizički scanout. Panel
refresh je spušten na 50,0146 Hz povećanjem samo VFP-a s `7` na `109`;
horizontalni timing, RGB888, burst packetizacija, lane/rate, orijentacija i
touch nisu mijenjani. Solo D1 tada je fizički potvrđen kao oštar i fluidan.

Pri istodobnom D1+D2 opterećenju gornji waveform još je blago pokazivao isti
efekt. Scheduler je na svakom frameu izmjenjivao redoslijed dvaju direct-PPA
blitova, zbog čega je gornji overlay svaki drugi frame bio kasni upis. Za
dual-redraw tick redoslijed je zaključan na gornji pa donji, u skladu s
panel scanoutom; single-redraw fairness i dalje se izmjenjuje. Korisnik je na
završnom kandidatu potvrdio da su oba waveforma oštra i fluidna te da nema
bljeskanja ni audio posljedice. Puni host suite i ESP-IDF 6.0.2 build prošli su.
Detalji, image identitet i ograničenja nalaze se u
[validation zapisu](validation/2026-09-01-dsi506-waveform-sync.md).

### Desetominutni zajednički integration soak (2026-09-01)

Oba 44,1-kHz decka s aktivnim loopovima ostala su u playbacku svih 600 s dok
su istodobno radili PCM5102A master, FLX4 UAC slušalice, Overview waveformi,
FT5426 touch/backlight, USB3 knjižnica od 191 trake i kontinuirani Wi-Fi web
promet. Operator je potvrdio čist zvuk, fluidan prikaz, responzivan touch,
stabilan zaslon i FLX4. Nije bilo PCM underruna, UAC drop/overflow/underflowa,
service-log dropa ni API/state greške. Pet output-late događaja, maksimalno
12522 us, nije imalo fizičku posljedicu i ostaje monitoring nalaz. Vidi
[integration soak zapis](validation/2026-09-01-integration-soak.md).

Identifikacijska referenca:
[Linux ICN6211 driver](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/bridge/chipone-icn6211.c)
odvaja provjeru ID-a od konfiguracijskih upisa. Ne prenijeti init sekvencu dok
se ne potvrde konkretni bridge i panel parametri.

### Potvrden post-power scan na COM6

Prvi identifikacijski build `M3-45-g5bb55bc-dirty` instaliran je app-only u
`factory` na `0x20000`; SHA-256 binarnog imagea:
`B5F19BEC66D064ECADD5EBA48743B57EBDFFE8F2189798CDB019074765C7FD92`.

- Sve cetiri faze nalaze tocno `0x18`, `0x38`, `0x45`, po tri uredaja i nula
  probe gresaka. `0x2C` vraca `ESP_ERR_NOT_FOUND` i nakon power-on + 250 ms.
- Kontroler ponavlja `ID=C3`, `ID2=8B`, `PORTB=85`. Bridge ostaje nepoznat.
- Boot nastavlja do microSD mounta i USB knjiznice sa 191 trakom. Korisnik
  prijavljuje povratak bijele slike; testne trake/UI nisu prihvaceni.
- ESP-IDF 6.0.2 build i puni P4 host suite prosli su (exit 0). To nije dokaz
  ispravnog DSI transporta niti zajednicki audio/display acceptance.

### Prethodni cetverobajtni DSI ID pokus

Prije DPI klijenta salju se samo standardni Maximum Return Packet Size = 4
i LP Generic Read s parametrima `{0x00, 4}` prema Linux ICN6211 read protokolu.
Nema bridge unlocka, vendor upisa ni promjene video timinga. Svako citanje ima
jedan zajednicki softverski deadline od 200 ms (uz rasporedivanje RTOS taska).
IDF 6.0.2 HAL read helper ima neogranicena FIFO cekanja, pa ga ne koristimo.
Samo tocno `C1 62 11` pokrece ponovljeno citanje; identitet vrijedi tek ako se
oba cetverobajtna rezultata poklope bez prijavljenih transportnih gresaka.

Nakon probe DSI host uvijek se ugasi/izbrise/ponovno kreira prije DPI starta,
ukljucujuci timeout put, tako da zaglavljeni BTA ili RX podaci ne ostanu u
video putu. Neodziv nije dokaz ni vrste ni neispravnosti bridgea.

Prva COM6 proba (image SHA-256
`464757C7B3D0C9F7E128EC8F8AA8CE70DD0EEDE332E5A245D4A853DA27436BEF`)
vratila je `ESP_ERR_INVALID_RESPONSE` za 1032 us, ne softverski timeout:

```text
stage=generic-read-response
cmd_pkt_status=00050015 phy_status=0000153D
int_st0=00000042 int_st1=00000000
```

U P4 IDF register mapu to su `ACK_WITH_ERR_1` i `ACK_WITH_ERR_6`, a ne valjani
ID payload. Prema usporednom opisu istih DSI host registara to odgovara
SoT Sync i False Control greskama u peripheral ACK/error reportu
([Microchip DSI_INT_ST0 reference](https://onlinedocs.microchip.com/oxy/GUID-82119957-1E11-4B69-84AC-EF0EA08F5595-en-US-5/GUID-158FA27F-33B6-4747-8EE7-2B16772DC3BF.html)).
To tumacenje nije identifikacija bridgea i ne razlikuje problem protokola,
power/reset sekvence ili integriteta signala. Nije dokaz kvara samog panela.

Error put uredno je ponovno kreirao host i pokrenuo video, zatim microSD i
USB knjiznicu (191 traka), bez opaženog panic/reset ciklusa. Puni 200-ms
timeout put nije aktiviran ovim hardverskim rezultatom. Build i host suite
prosli su; host DSI provjere su staticki ugovori, ne simulacija periferije.
Sljedeci preduvjet za vendor init je tocni proizvod/revizija i odgovarajuca
driver dokumentacija ili potvrdeno ID ocitanje. Ne premjestati 0R selektor,
ne lemiti i ne mijenjati timing na temelju same bijele slike.

Zavrsni dijagnosticki image ovog koraka (isti version string
`M3-45-g5bb55bc-dirty`, `factory`, app-only na COM6) ima SHA-256:
`CD3F48B91ADFAB6213CC3A301D8C470535F637577EF37E525F8C4A562A498F44`.
Velicina je `0x2428B0` / 2369712 bajtova; 44% app particije je slobodno.
Flash hash je verificiran. Ponovljeni boot vraca `ESP_ERR_INVALID_RESPONSE`
za 1033 us, ovaj put `int_st0=00000040`, `int_st1=00000000`, uz isti
`cmd_pkt_status=00050015` i `phy_status=0000153D`. Dakle failure je ponovljen,
ali ACK/error bitmaska nije bila identicna (`0x42` pa `0x40`). Host je ponovno
kreiran i boot se nastavio bez opaženog panica. Oba puna build/host runs
prosla su (exit 0), kao i `git diff --check`.

USB knjiznica ponovno je ucitana sa 191 trakom nakon zavrsne instalacije.
Wi-Fi postavke nisu mijenjane; `Pajoniiir-M3` je potvrden u Windows listi
vidljivih mreza nakon obje probe. Web/audio/touch/visual acceptance nije
ponovno izveden ovim korakom. Nije bilo commita ni pusha.

Dobavljacu treba zatraziti tehnicki datasheet za `DSI506 / DYL0023`, tocni
DSI-to-RGB bridge i LCD panel model, register/init i power/reset sekvencu,
DSI lane count/bitrate/pixel format/video mode, pixel clock i H/V porch
parametre. Alternativni koristan dokaz je tocni Raspberry Pi driver/overlay
s izvornim kodom i potvrdom da pripada ovoj reviziji. Poruka nije poslana
dobavljacu; nema ovlastenja za vanjsku komunikaciju.

### Kontrolni pokus s pojedinacnim ID registrima

Linuxov `chipone_atomic_enable()` ne koristi cetverobajtni ID burst nego
`chipone_readb()` / `regmap_read()` zasebno za registre `0x00..0x03`.
Kontrolni pokus stoga koristi MRPS=1 i LP Generic Read `{reg, 1}`. Time se
usporeduje stvarni identifikacijski obrazac primarnog drivera; to samo po
sebi ne dokazuje da je prethodni burst bio nevaljan za ovaj hardver.

Produkcijski helper `bsp_dsi_id_probe.h` prekida na prvom transportnom
neuspjehu ili pogresnom prefiksu `C1 62 11`. Samo potpuni prvi ID pokrece
drugi prolaz, koji mora ponoviti i revision byte. Najvise osam pojedinacnih
citanja, po 200 ms softverskog deadlinea; nikakav vendor init ne slijedi
automatski ni nakon podudaranja. DSI host i dalje se resetira prije videa,
a lane count/rate, video mode, timing i postojece power/PWM naredbe nisu
mijenjani.

Dodani su raw readbackovi MCU registara PORTA/B/C (`0x81/0x82/0x83`) prije
POWERON=0, nakon njega i nakon POWERON=1 + settlinga. Nema novih upisa u te
registre. Adrese su usporedene s
[Raspberry Pi ATTiny regulator driverom](https://github.com/raspberrypi/linux/blob/rpi-6.6.y/drivers/regulator/rpi-panel-attiny-regulator.c);
znacenje pinova na ovom klonu nije potvrdeno. PORTB bit0 nije mjerenje
napona ni dokaz otpustenog bridge reseta.

ESP-IDF 6.0.2 build i puni P4 host suite prosli su (exit 0). Novi behavioral
test izvodi 274 slucaja za redoslijed registara, rano zaustavljanje na svim
pozicijama greske/nepodudaranja, ponovljeni ID i sve 256 revision vrijednosti.
To testira stvarni zajednicki helper, ne simulira DSI FIFO, PHY ili timeout.
Postojeci optional MP3-file decode primjer u host suiteu nije pokrenut jer
nije zadan ulazni MP3. Zasebni dugi audio soak i UI screenshot suite nisu
ponavljani za ovu identifikacijsku promjenu.

Image `M3-45-g5bb55bc-dirty`, SHA-256
`F9B8D7247D024FEA34E2A039E62AAF12B5E965B515507A7BCF1973D4C059C049`,
2370208 bajtova (`0x242AA0`, 43% particije slobodno), instaliran je app-only
na COM6 u `factory` na `0x20000`; NVS i OTA particije nisu prepisane.
Serijski boot potvrduje novi pojedinacni upit i sljedece rezultate:

| MCU faza | PORTA `0x81` | PORTB `0x82` | PORTC `0x83` |
| --- | --- | --- | --- |
| prije POWERON=0 | `04` | `85` | `00` |
| nakon POWERON=0 | `04` | `85` | `00` |
| nakon POWERON=1 + settling | `04` | `85` | `00` |

Ovo je warm/serial-reset pokus, ne dokaz pocetnog stanja nakon fizickog
uklanjanja svih napajanja. Readbackovi se ne mijenjaju, ali bez sheme i
potvrdenog MCU protokola to ne dokazuje da se stvarni power/reset pinovi ne
mijenjaju. Ne upisivati druge PORTC vrijednosti prema pretpostavci o klonu.

Sve cetiri I2C faze opet nalaze `0x18/0x38/0x45`, nula probe gresaka;
`0x2C` je nedostupan. Vec prvi pojedinacni upit za registar `0x00` vraca:

```text
ESP_ERR_INVALID_RESPONSE stage=generic-read-response elapsed=1020 us
cmd_pkt_status=00050015 phy_status=0000153D
int_st0=00000040 int_st1=00000000
```

Daljnji ID upiti su preskoceni, identitet ostaje UNKNOWN, host je ponovno
kreiran i pokrenut je isti 15-sekundni testni stream. Korisnik potvrduje:
**i dalje bijeli ekran**. Promjena velicine upita nije uklonila gresku;
200-ms timeout opet nije dosegnut. MicroSD se montira, USB knjiznica se
ucitava sa 191 trakom na 19,199 s. Pocetni "PDB not found" prije USB mounta
i MSC sense `06/28/00` prethode uspjesnom ucitavanju, bez opazenog panica.
`Pajoniiir-M3` je ponovno vidljiv u Windows Wi-Fi scanu; FLX4/audio/touch
prihvacanje nije izvedeno. Nema commita ni pusha.

Sljedeci izolirani pokus je fizicki cold power-on sa svim izvorima napajanja
odspojenima najmanje 10 s, bez pomicanja FFC-a i bez novih firmware upisa.
Usporediti MCU readbackove i ID odgovor s ovim zapisom. Ako se rezultat
ponovi, i dalje treba tocni driver/power-reset protokol dobavljaca ili
referentni pokus na podrzanom Raspberry Piju; bijela slika sama nije dokaz
neispravnog panela ili dovoljan razlog za nasumicni vendor init.

### Cold power-on: USB napajanje s racunala

Korisnik je potvrdio ponovno spajanje nakon trazenog odspajanja i izricito
naveo da plocu napaja USB kabel s racunala. Koristen je isti prethodni
image, bez novog builda/flasha ili promjene FFC-a. COM6 monitor zabiljezio je
odspajanje porta, automatski reconnect i novi boot (`reset reason: 1`).

- ID MCU-a je opet `C3/8B`; PORTA/B/C ostaju `04/85/00` u sve tri faze.
- I2C scanovi opet nalaze `0x18/0x38/0x45`, bez probe gresaka; `0x2C` ne
  odgovara. Prvi DSI ID byte opet nije procitan: `ESP_ERR_INVALID_RESPONSE`,
  `elapsed=1022 us`, `cmd_pkt_status=00050015`, `phy_status=0000153D`,
  `int_st0=00000F43`, `int_st1=00000000`. Bitmaska se razlikuje od prethodnog
  `0x40`; ovo nije valjani ID ni dokaz konkretnog uzroka/kvara.
- Host je ponovno kreiran i testni stream pokrenut. Nema novog korisnickog
  vizualnog prihvacanja za ovaj boot. MicroSD je montiran; USB knjiznica
  na prvom pokusaju prijavljuje `Bad page_size=1341294004`, a retry uspijeva
  sa 191 trakom na 19,755 s. Ta nova anomalija ostaje otvorena, ne pripisuje
  se napajanju niti se smatra dokazom trajno ostecene baze.
- Nema opazenog panic/reset ciklusa nakon boota; `Pajoniiir-M3` je vidljiv
  u Wi-Fi scanu. RF vidljivost nije potpuni web/audio acceptance.

Sljedeca provjera: izmjeriti DC napon pod trenutnim opterecenjem na
postojecem PCM5102A `VIN/GND` (5-V vod) i display `FAN 3V3/GND` headeru.
Ranijih 3,4 V na FAN-u nije novo mjerenje ovog boota. Prije premjestanja
sondi osigurati da ne mogu kratko spojiti susjedne kontakte; mjerni instrument
koristiti u DC voltage, ne current modu. FAN je mjerno mjesto, ne ulaz za
dodatno napajanje. Ne mijenjati firmware, FFC ni dodavati paralelni izvor dok
se ovaj izolirani test ne zavrsi. DC mjerenje ne iskljucuje kratke tranzijente.

Korisnik je zatim prijavio **PCM5102A VIN/GND = 4,85 V** i **display FAN
3V3/GND = 3,3 V**. To su korisnikova multimetarska ocitanja, ne telemetrija
firmwarea. Na tim mjernim mjestima nema ocitog velikog stalnog odstupanja od
nominalnih 5 V i 3,3 V; 4,85 V je 3% ispod nominalnih 5 V. To ne potvrduje
sve interne napone modula, raspolozivu struju USB porta ili odsutnost kratkih
padova/ripplea, niti objasnjava prethodnu USB read anomaliju.

Korisnik dodatno potvrdjuje da fizicki **Backlight gumb radi**. To je
potvrda funkcionalnosti lokalne kontrole pozadinskog osvjetljenja, ne dokaz
DSI video prijenosa, identiteta bridgea ili ispravne LCD init sekvence.
Naknadno je precizirao da uzastopni pritisci gumba **korak po korak povecavaju
svjetlinu**; opaženo je stupnjevito upravljanje, ne samo on/off. Ne izvodi se
zakljucak o broju stupnjeva, PWM polaritetu ili modelu MCU-a.

Naknadno citanje COM6 nije dalo novi output, panic ni reset. Nema novog
flasha niti promjene konfiguracije. Dosadasnji nalazi ne opravdavaju
proglasavanje USB napajanja uzrokom niti nasumicnu promjenu panel timinga.
Sljedeci najkorisniji neovisni test je isti display/FFC na Raspberry Pi 3/4
navedenom na kutiji, uz odgovarajuce proizvodjacke upute. Dostupnost takve
ploce tek treba potvrditi. Ako tamo proradi, to potvrduje funkcionalnost
zaslona u toj konfiguraciji i daje referencu za MCU/bridge init; ne prihvaca
automatski P4 implementaciju. Ako ne proradi, provjeriti reference setup,
FFC i panel prije zakljucka da je panel neispravan.

Korisnik je potvrdio da **nema Raspberry Pi**, pa taj referentni test trenutno
nije dostupan. Lokalna shema
`JC-ESP32P4-M3-DEV/5-Schematic/6_MIPI_DSI&MIPI_CSI.png` vizualno je ponovno
provjerena: J2 nosi DSI DATA1 na 2/3, CLK na 5/6, DATA0 na 8/9, I2C na 11/12
i 3,3 V na 14/15. To se slaze s ovdje dokumentiranim pinoutom; ne potvrduje
stvarno stanje svakog kontakta korisnikova FFC-a. I2C/backlight funkcionalnost
zato ne iskljucuje los kontakt DSI clock/data vodova.

Sljedeci korisnicki pokus: nakon odspajanja svih napajanja pricekati gasenje,
pazljivo otvoriti FFC zasune na oba kraja, pregledati kontakte i kabel ravno
umetnuti do kraja u **istoj dosadasnjoj orijentaciji**, bez okretanja pinova.
Zatvoriti zasune bez sile, ponovno ukljuciti i usporediti COM6 ID odgovor i
sliku. Firmware i video parametri ostaju isti. Rezultat jos nije izveden niti
prihvacen; ako nema promjene, traziti provjeren zamjenski FFC ili tocni
dobavljacev driver/MCU-bridge protokol umjesto nasumicnih vendor naredbi.

### Ponovno spajanje bez USB medija i microSD kartice

Na zahtjev za ponovno umetanje FFC-a korisnik javlja "spojeno" i dodatno
navodi odspojene USB medij i SD karticu. Stoga ovaj pokus mijenja i prisutnost
medija/opterećenje, nije izolirana usporedba samo FFC kontakta. COM6 biljezi
odspajanje/reconnect te boot istog `M3-45-g5bb55bc-dirty` imagea, bez flasha.

- Sve cetiri I2C faze ostaju `0x18/0x38/0x45`, nula probe gresaka;
  MCU ID `C3/8B`, PORTA/B/C `04/85/00` u sve tri faze; `0x2C` nije dostupan.
- DSI registar `0x00`: `ESP_ERR_INVALID_RESPONSE`, `elapsed=1023 us`,
  `cmd_pkt_status=00050015`, `phy_status=0000153D`, `int_st0=00000F43`,
  `int_st1=00000000`. Identitet je UNKNOWN; bitmaska je ista kao u prethodnom
  cold-power pokusu s medijima. Greska se dakle reproducira i bez njih.
- Host je ponovno kreiran i pokrenut je testni stream. Korisnik naknadno
  potvrdjuje da je i nakon ovog spajanja ekran ostao potpuno bijel.
- Bez microSD-a tri mount pokusaja zavrsavaju timeoutom, nakon cega se /sd
  preskace. Cleanup dodatno ispisuje `host controller with slot registered`;
  to biljeziti kao zaseban shared-SDMMC teardown trag, ne DSI uzrok.
- Bez USB medija nema PDB knjiznice; USB storage nakon osam brzih ciklusa
  prelazi na 30-s recovery interval. Ne proglasavati odsutni medij novim
  kvarom niti tvrditi da je u ovom bootu ucitano 191 traka.
- Nakon boota nema novog serijskog outputa/panica/resetiranja u procitanom
  intervalu, a `Pajoniiir-M3` ponovno je vidljiv u Wi-Fi scanu. Web/audio
  i SD reinsertion acceptance nisu izvedeni.

Simptom je ostao isti; sljedeci hardverski kontrolni pokus zahtijeva drugi
provjereno kompatibilan 15-pinski DSI FFC. Njegova dostupnost nije potvrdena.
Ako nije dostupan, potreban je tocni dobavljacev driver/protokol za daljnju
inicijalizaciju. Dosadasnji neuspjeli kandidat-ID upit nije dokaz pokvarenog
zaslona i ne iskljucuje drugaciji, jos nepotvrdeni bridge protokol.

### Drugi, kraci FFC: simptom nepromijenjen

Korisnik je prijavio drugi isti, ali kraci kabel i nakon zamjene uz ugaseno
napajanje potvrdio: "spojeno. isto je bijeli ekran". Isti firmware ostaje u
factory particiji; USB medij i microSD ostaju odspojeni. COM6 reconnect je
uhvacen bez novog flasha. Sve I2C faze i MCU readbackovi ostaju kao prije;
`0x2C` i dalje ne odgovara. Prvi DSI ID registar `0x00` vraca
`ESP_ERR_INVALID_RESPONSE`, `elapsed=1021 us`, status `00050015`, PHY
`0000153D`, errors `00000F43/00000000`. Host se ponovno kreira i pokrece
testni stream, ali korisnik ne vidi sliku nego bijeli panel.

Zamjena kabela nije uklonila simptom. To nije certificiranje ispravnosti
oba kabela niti dokaz pokvarenog panela; stvarni bridge i njegov read/init
protokol i dalje nisu identificirani. Daljnje ponavljanje istog upita ili
nasumicni vendor init nisu opravdani. Za sljedeci konfiguracijski korak
zatraziti od prodavaca tocni DSI-to-RGB bridge IC, power/reset i register-init
sekvencu te Raspberry Pi driver/overlay (po mogucnosti source) za
**EYOYO DSI506 / DYL0023**, ne jos jedan genericki PRODUCT MANUAL.
Poruka dobavljacu nije poslana. Kraci kabel moze ostati kao trenutni bench
uvjet; audio/touch/display acceptance i dalje nisu dovrseni.

### Provjera naknadno dostavljenog teksta o TC358762

Procitan je cijeli korisnicki `pasted-text.txt` iz attachmenta
`56ffb124-2576-4b00-83ca-ed7d22f73f2f`, SHA-256
`7D7618C6AD2A76884751ED78B468243F9D4F913C1AEEAB1629A952763798F5CF`.
Tekst otvoreno nagadja register vrijednosti i zatim iznosi nepotvrdjene
tvrdnje o EYOYO DYL0023. Nije dokaz BOM-a niti ovlast za slijepo izvrsavanje
navedenih upisa, GPIO reseta, promjenu lane counta ili dodatnih napona.

Usporedjen je s primarnim Raspberry Pi Linux `rpi-6.1.y` izvorima:

- [panel-raspberrypi-touchscreen.c](https://github.com/raspberrypi/linux/blob/rpi-6.1.y/drivers/gpu/drm/panel/panel-raspberrypi-touchscreen.c):
  odvojeni 8-bitni I2C MCU registri i 6-byte generic DSI write
  (16-bitna adresa + 32-bitni podatak, little-endian). `rpi_touchscreen_prepare`
  sadrzi izravne DSI upise; prikazana `tc358762_init[]` tablica s nizom nula
  ne postoji. `PPI_STARTPPI` je `0x0104`, ne `0x0004`. Power sekvenca ide
  preko MCU `REG_POWERON`, ne preko prikazanog GPIO27 pseudokoda. Referentni
  driver prihvaca MCU ID `C3` kao firmware rev2 i koristi jednu DSI lane.
- [vc4-kms-dsi-7inch-overlay.dts](https://github.com/raspberrypi/linux/blob/rpi-6.1.y/arch/arm/boot/dts/overlays/vc4-kms-dsi-7inch-overlay.dts):
  `0x45` je panel regulator/GPIO controller na I2C-u, dok je TC358762 child
  DSI hosta. Bridge enable vezan je uz taj kontroler, ne uz Raspberry Pi
  GPIO27. Stvarni KMS overlay koristi odvojeni bridge driver i nije jednak
  ilustrativnom overlayu iz teksta.
- [bridge/tc358762.c](https://github.com/raspberrypi/linux/blob/rpi-6.1.y/drivers/gpu/drm/bridge/tc358762.c):
  stvarna funkcija `tc358762_init()` salje konfiguraciju kroz
  `mipi_dsi_generic_write()`, ne neposredne bridge I2C upise na `0x45`.
  Koristi jednu lane, RGB888 i video sync-pulse/LP. Podrska silicija za dvije
  lane nije isto sto i konfiguracija tog referentnog drivera.

U tekstu predlozeni I2C pinovi FPC 15/16 nisu primjenjivi na nas 15-pinski
J2: prema lokalnoj shemi SCL/SDA su 11/12, pin15 je 3,3 V, a pin16 ne
postoji. Ne spajati analizator ili dodatno napajanje po tom tekstu.
Ni referentni TC358762 driver ni podudaranje MCU ID-a `C3` ne dokazuju da je
na ovom EYOYO modulu TC358762. Dosadasnji bounded ID upit je **ICN6211
kandidat-protokol**, ne univerzalni DSI ID test: njegov neuspjeh na drugom
bridgeu ne bi dokazao kvar DSI linka. Izvorni driver je koristan tehnicki
trag, ali tekst ne zatvara identifikacijski preduvjet. Nema novih firmware
upisa/flasha, ožicenja ili promjena napajanja na temelju ovog priloga.

### Drugi AI prilog: ICN6211 / GT911 / navodni 100-ms clock window

Drugi cijeli `pasted-text.txt`, attachment
`199ad2b7-0bec-49fc-bf0e-859a1d48dfe7`, SHA-256
`2F9E7E4CF21890DE2AC8007119864DE556700367782B5E6DC2471543DB2BFA75`,
izricito je prijepis AI razgovora. Najprije proglasava modul Waveshareom,
zatim bez izvora za konkretni BOM tvrdi ICN6211, GT911 i MCU auto-init.
To nije neovisna potvrda prethodnih kandidata niti odgovor proizvodjaca.

- GT911 / `0x5D` ili `0x14` nije sukladan dosadasnjem fizickom scanu koji
  nalazi `0x38`, a ne te adrese, niti dostavljenom listu koji navodi FT5426.
  I dalje razlikovati specifikaciju i detekciju adrese od citanja touch IC ID-a.
- Tvrdnja o automatskoj ICN6211 inicijalizaciji i obveznom kontinuiranom
  DSI clocku unutar 100 ms nema prilozen MCU source/datasheet ili mjerenje.
  Ostaje hipoteza, ne dokazana osobina DYL0023. Ne izvoditi prikazane GPIO
  power/IOVCC/reset promjene: ti pinovi nisu potvrdeni na korisnikovu modulu.
- Navodni "panel-raspberrypi defaults" ne odgovaraju prethodno provjerenom
  `rpi-6.1.y` driveru: njegov HFP/HSW/HBP je `1/2/46`, VFP/VSW/VBP `7/2/21`,
  clock priblizno 25,9794 MHz i jedna lane, a ne vrijednosti `40/20/46`,
  `7/2/23`, 27 MHz i dvije lane iz priloga. To jos ne propisuje timing za
  ovaj nepotvrdjeni 5-inch modul.
- Primarni [ESP-IDF 6.0.2 DSI API](https://docs.espressif.com/projects/esp-idf/en/v6.0.2/esp32p4/api-reference/peripherals/lcd/dsi_lcd.html)
  potvrduje da je DSI host dio postojeceg `esp_lcd` komponenta. Za host ne
  treba izvrsavati navedeni `add-dependency "espressif/esp_lcd_mipi_dsi"`.
  Eventualni konkretni panel driver zasebna je stvar.
- Pseudokod se ne moze prenijeti kao ESP-IDF 6.0.2 kod: stvarni
  `esp_lcd_video_timing_t` ima `h_size`, `v_size`, `hsync_pulse_width` i
  ostale `hsync_*`/`vsync_*` clanove, ne `h_active`, `h_front_porch` itd.
  Pixel clock je `esp_lcd_dpi_panel_config_t.dpi_clock_freq_mhz`, ne
  `video_timing.pixel_clock_hz`. Usporedjeno i s lokalnim IDF headerima.
- Tvrdnja da TC358762 pretvara RGB/SPI u DSI takodjer je obrnut smjer od
  citiranog Linux DSI-to-DPI bridge drivera.

Ova dva AI odgovora ne uspostavljaju identitet bridgea. Ocuvati postojece
bench stanje; nijedna naredba iz priloga nije izvrsena, driver dependency
nije dodan i firmware nije mijenjan. Eventualni test ranog kontinuiranog
host clocka mora biti zasebno oznacen kao provjera hipoteze, bez mijesanja
s drugim lane/timing/reset pretpostavkama i bez tvrdnje o potvrdenom BOM-u.

Naknadni prosireni AI prijepis (attachment
`4c408415-4eef-46c1-a487-3d2e0a7be4dd/pasted-text.txt`) procitan je cijeli.
Novi zavrsetak povlaci GT911 tvrdnju i navodi FT5426/`0x38`; registri prvog
dodira `0x02..0x06` podudaraju se s vec ukljucenim Espressif FT5x06 driverom.
Lokalno su provjereni njegov source/header i `bsp_touch_init()`; novi driver
ne treba dodavati, ali to jos nije hardverska potvrda tocnih koordinata.

Zakljucak "LCD RGB666 stoga obvezno bits_per_pixel=18 na DSI-u" nije
potkrijepljen: dostavljeni list odvojeno navodi LCD RGB666 i module MIPI DSI.
Treba razlikovati framebuffer format, DSI ulaz bridgea i njegov paralelni
LCD izlaz. Njihova jednakost nije dokazana. Trenutni P4 put eksplicitno
koristi RGB565 framebuffer i RGB888 DSI output; ne mijenjati ih na temelju
same oznake LCD izlaza. U nasem DPI putu format bira
`esp_lcd_dpi_panel_config_t.in_color_format/out_color_format`; nepovezani
`esp_lcd_panel_dev_config_t.bits_per_pixel` iz pseudokoda to ne mijenja.
Ponovljeni timing primjer ima iste nepostojece clanove IDF 6.0.2 strukture,
a 27 MHz nije specifikacija izvediva samo iz rezolucije 800x480. Nema novih
firmware upisa niti promjene boja/timinga; primarni ESP-IDF API prethodno
citiran u ovom zapisu ponovno je provjeren.

### Izolirani pokus ranog kontinuiranog DSI clocka

Na korisnikovo odobrenje promijenjena je samo postavka DSI hosta
`esp_lcd_dsi_bus_config_t.flags.clock_lane_force_hs = true`. Primarni
[ESP-IDF v6.0.2 bus source](https://github.com/espressif/esp-idf/blob/v6.0.2/components/esp_lcd/dsi/esp_lcd_mipi_dsi_bus.c)
primjenjuje je vec u `esp_lcd_new_dsi_bus()`, prije postojeceg softverskog
`POWERON=1` na MCU-u. Nisu promijenjeni lane count/rate, video mode, timing,
RGB565/RGB888 formati, power/PWM upisi niti ID-protokol. Nema novih vendor
upisa, GPIO reseta, dodatnog napajanja ili promjene ozicenja.

Logovi `before-power-on`, `after-power-on`, `after-host-recreate` i
`video-started` biljeze vrijeme, HS request, auto-clock bit, LPCLK_CTRL i PHY
status. To je readback hosta, ne osciloskopsko mjerenje clocka na modulu.
Postojeci ID-probe recovery i dalje kratko prekida clock brisanjem/reinitom
hosta; forced-HS postavka se ponovno primjenjuje. Pokus zato **ne dokazuje
clock bez prekida kroz cijeli boot**, niti clock unutar 100 ms od fizickog
napajanja modula. Raniji clock je samo u odnosu na softverski POWERON.

ESP-IDF 6.0.2 build i puni P4 host suite prosli su (exit 0), ukljucujuci
dvije nove staticke provjere redoslijeda ranog clocka i nepromijenjenih video
parametara. Host provjere nisu simulacija DSI PHY-a. Zasebni UI screenshot
suite i dugi audio soak nisu ponavljani za ovaj pokus.
Image `M3-45-g5bb55bc-dirty` ima 2370496 bajtova (`0x242BC0`, 43% slobodno),
SHA-256 `AE8C121286C9CACBEE21E25391D4619D4E7CA5EBEC9619373E48FAE46F4B9C6A`.
App-only flash na COM6 verificiran je hashom; boot potvrduje `factory` na
`0x20000`. NVS/OTA particije nisu prepisane. Kraci FFC i USB napajanje s PC-a
ostaju; USB medij i microSD nisu spojeni.

| Clock faza | Log vrijeme (ms) | esp_timer (us) | HS request / auto | LPCLK_CTRL | PHY |
| --- | --- | --- | --- | --- | --- |
| before-power-on | 1910 | 153345 | 1 / 0 | `00000001` | `00001539` |
| after-power-on | 2043 | 287075 | 1 / 0 | `00000001` | `00001539` |
| after-host-recreate | 2407 | 650340 | 1 / 0 | `00000001` | `00001539` |
| video-started | 2440 | 683480 | 1 / 0 | `00000001` | `00001539` |

Log timestamp i esp_timer imaju razlicite vremenske baze; ne tretirati
esp_timer vrijednost kao vrijeme od fizickog ukljucenja USB-a. PHY clock
stop-state bit sada je obrisan u odnosu na prethodni `0000153D` readback.
I2C scanovi i MCU readbackovi ostaju nepromijenjeni. ICN6211 kandidat-ID
registar `0x00` opet vraca `ESP_ERR_INVALID_RESPONSE`, ovaj put za 1031 us,
`status=00050015`, `phy=00001539`, `err=00000042/00000000`. Identitet ostaje
UNKNOWN; 200-ms timeout nije dosegnut.

**Korisnik prvi put potvrdjuje obojene testne trake** ("evo napokon color
bar!!!!!"), a zatim nakon prijelaza prijavljuje **jednobojni crveni ekran**,
bez teksta/kontrola. Ovo je prvi uspjesni prikaz host-generator uzorka nakon
promjene clocka, ne dovrsen UI/display acceptance. Neuspjesan kandidat-ID
upit koegzistira s vidljivim videom i ne smije se tretirati kao univerzalni
test neispravnosti DSI linka. Nisu potvrdeni bridge IC ni 100-ms hipoteza.

Provjera IDF `esp_lcd_dpi_panel_set_pattern()` potvrduje da host generator
zaobilazi framebuffer put: `PATTERN_NONE` gasi generator i ponovno ukljucuje
interni P4 DPI bridge. Preostalu jednobojnu sliku treba izolirati usporedbom
CPU framebuffer uzorka s LVGL/PPA prikazom, uz isti clock i video parametre;
sam simptom jos ne utvrdjuje je li uzrok DMA, prijelaz generatora ili UI.
U procitanom serijskom intervalu nije prijavljen DSI underrun/panic/reset.
Prisutan je vec poznati SD timeout/cleanup trag bez kartice, USB knjiznica
nije dostupna, a `Pajoniiir-M3` je vidljiv u Windows Wi-Fi scanu. Web/audio,
touch, cold-power ponavljanje i integration soak nisu prihvaceni. Nema
commita ni pusha.

### CPU framebuffer: izolacija jednobojne slike nakon generatora

Sljedeci dijagnosticki kandidat zadrzava forced-HS clock i sve video/power/ID
postavke prethodnog prvog uspjesnog prikaza. Nakon 15 s hardverskih okomitih
traka CPU upisuje cetiri RGB565 kvadranta u **stvarni, jedini DPI framebuffer**:
gore lijevo crveno, gore desno zeleno, dolje lijevo plavo, dolje desno bijelo,
s crnim rubom sirine 8 px. Cache se eksplicitno objavi u memoriju dok je
generator jos ukljucen, zatim se generator iskljuci. Kvadranti ostaju 12 s,
pa CPU brise isti framebuffer na crno i objavljuje cache za dodatne 3 s.
Tek potom se nastavlja normalni boot do LVGL/PPA/audio/USB/Wi-Fi zadataka.
Pokus privremeno produljuje boot za 15 s, ne mijenja spremljenu Wi-Fi postavku.

Nema novog framebuffera, resetiranja DMA-a, vendor upisa ni promjene boja na
DSI zici. `esp_mm` je dodan kao privatni BSP dependency za `esp_cache_msync`.
Read-only `DSI scanout` zapisi usporedjuju generator enable, interni P4 DPI
enable, pixel-format registar, trenutnu FIFO dubinu i host error registre.
Snapshot nije mjerac propusnosti ni kumulativni underrun brojac, a ispravan
CPU upis/cache API nije sam po sebi dokaz da je slika stigla do panela.

Referenca za driverov framebuffer i generator prijelaz:
[ESP-IDF 6.0.2 DPI driver](https://github.com/espressif/esp-idf/blob/v6.0.2/components/esp_lcd/dsi/esp_lcd_panel_dpi.c).
Nova host provjera je staticki ugovor redoslijeda, ne simulacija DMA/PHY-a.
Prvi build zaustavljen je zbog nedeklariranog `esp_mm` dependencyja;
nakon ispravka privatne CMake ovisnosti ESP-IDF 6.0.2 build prosao je
(exit 0). Puni P4 host suite takodjer je prosao, ukljucujuci novi staticki
ugovor. `git diff --check` prosao je. UI screenshot gate i zasebni dugi audio
soak nisu ponovljeni. Image `M3-45-g5bb55bc-dirty` ima 2372112 bajtova
(`0x243210`, 43% slobodno), SHA-256
`239EBE377E5B615C366A09AE8E436450087465A10ACAD49A76DECBC57059BBBB`.
App-only flash na COM6 verificiran je hashom, a boot potvrdjuje `factory`
na `0x20000`. NVS/OTA particije nisu prepisane. Korisnik nakon probe javlja:
**"nakon traka ostaje crveno"**. Cetiri CPU polja i crna faza nisu vidljivi;
problem je reproduciran prije LVGL/PPA inicijalizacije.

| Faza | Log ms | VPG / DPI | PIXEL_TYPE | FIFO dubina | INT_ST0 / INT_ST1 |
| --- | --- | --- | --- | --- | --- |
| host-bars-end | 17474 | 1 / 0 | `00000000` | 892 | `00000000 / 00080080` |
| cpu-quadrants-start | 17481 | 0 / 1 | `00000000` | 892 | `00000000 / 00080000` |
| cpu-quadrants-end | 29491 | 0 / 1 | `00000000` | 892 | `00000000 / 00000000` |
| cpu-black-end | 32499 | 0 / 1 | `00000000` | 892 | `00000000 / 00000000` |

Clock prije UI handoffa i dalje je `request_hs=1`, `auto=0`, LPCLK_CTRL=1,
PHY=`00001529`. I2C/MCU scanovi ostaju isti; ICN6211-specific reg0 upit
vratio je INVALID_RESPONSE za 1023 us s error `00000040/00000000`.
To nije glavni kriterij ovog framebuffer pokusa. Boot nastavlja do UI i
Wi-Fi zadataka uz vec poznate poruke odsutnog SD/USB medija; nema opazenog
panica/reset ciklusa. `Pajoniiir-M3` je vidljiv u Windows Wi-Fi scanu.

#### Utvrdjen interni RGB565/RGB888 nesklad na P4 rev1.3

COM6 flasher identificira stvarni chip kao **ESP32-P4 v1.3**. U primarnom
[IDF 6.0.2 bridge LL driveru](https://github.com/espressif/esp-idf/blob/v6.0.2/components/esp_hal_lcd/esp32p4/include/hal/mipi_dsi_brg_ll.h)
grana `CHIP_SUPPORT_MIN_REV < 300` koristi **isti `pixel_type.raw_type`** za
input i output setter. Input RGB565 upisuje 2, a naknadni output RGB888
prepisuje isti field na 0. Odvojeni `raw_type` i `dpi_type` postoje tek u
grani za rev >= 3.0. To nije moguce rijesiti samo ponovnim upisom ulaznog
formata bez promjene izlaznog tumacenja na ovoj reviziji.

Aktualni DPI driver racuna framebuffer velicinu i DMA transfer iz
`in_color_format` (800x480x2 = 768000 B), ali naknadnim output setterom
postavi hardver na RGB888 (800x480x3 = 1152000 B). Readback `PIXEL_TYPE=0`
potvrdjuje taj nesklad na uredaju. Ovo je konkretna firmware konfiguracijska
greska, ne pretpostavka o vanjskom bridgeu. DSI host INT_ST1 bit19
`dpi_buff_pld_under` i bit7 `dpi_pld_wr_err` takodjer su zabiljezeni; kasniji
nulti snapshot ne ponistava prethodnu gresku i nije kumulativni counter.

Sljedeci korektivni pokus: **zadrzati RGB888 na DSI zici**, a pravi scanout
framebuffer prebaciti na RGB888 i uskladiti sve njegove writere/strideove.
LVGL i waveform source mogu ostati RGB565, uz PPA konverziju u RGB888 prije
scanouta; CPU probe i direct-rectangle put moraju pisati po tri bajta.
Jedan framebuffer tada trosi dodatnih 384000 B PSRAM-a, a scanout bandwidth
raste 50%; obvezno ponovno pratiti underrun i naknadni integration soak.
Ponoviti iste trake/CPU-polja/crno/UI da se potvrdi da ispravak uklanja
simptom. Taj je ispravak u nastavku implementiran kao izolirani kandidat;
generator-handoff/DMA problem i dalje se mora potvrditi vizualnim hardware testom.

### Ispravak scanout formata: RGB888 memorija i DSI izlaz

Korektivni kandidat postavlja DPI input i output na RGB888. DSI wire format,
forced-HS clock, lane/rate, porch/video postavke, power/ID sekvenca i
15/12/3-sekundni testni slijed ostaju isti. Jedan stvarni framebuffer je
1152000 B (800x480x3), bez dodatnog drugog framebuffera.

LVGL draw buffer i waveform izvori ostaju RGB565. PPA SRM input ostaje
RGB565, output je RGB888, a output buffer size koristi zajednicki
`BSP_SCANOUT_BYTES_PER_PIXEL=3`. CPU test i izravno crtanje pravokutnika
koriste isti `bsp_scanout.h` helper: tri uzastopna bajta B/G/R, u skladu s
IDF `color_pixel_rgb888_data_t` redoslijedom komponenti. Ne koristi se
`sizeof` te IDF unije (sadrzi i 32-bitni clan). CPU 5/6-bitne komponente
prosiruju se bit-replikacijom; cache publish pravokutnika sada vraca gresku
pozivatelju umjesto da je zanemari.

Novi host behavioral test pokriva svih 65536 RGB565 vrijednosti i byte guardove,
420 valjanih pravokutnika s provjerom stridea/nepromijenjenih susjeda te 13
nevaljanih poziva: ukupno 65969 slucajeva. PPA poziv nije host-simuliran;
dodatni staticki ugovor cuva input/output i velicinu scanout buffera.

Provjera kandidata 2026-08-31:

- `idf.py build` je prosao na ESP-IDF 6.0.2; image je 2372112 B
  (`0x243210`), SHA-256
  `E3A9D25CE94DB76B33954768E8CD83259D2E1D5547F193009F983A03D7C7D25F`.
- `tests/run_p4_host_tests.ps1` je prosao; novi `bsp_scanout` test prijavio je
  `TESTS_RUN=65969`, a cijeli P4 host suite završio je s exit 0.
- Image je instaliran preko `COM6` naredbom `app-flash`; serijski boot i
  RGB888 CPU-probe (trake -> četiri polja -> crno -> LVGL handoff) prošli su
  bez panica. Vizualna potvrda korisnika za četiri polja, crni kadar i UI još
  nije upisana; do nje display gate ostaje otvoren.
- Kandidat s `mirror_x=true` i mapiranjem `physical.x = W - (logical.x +
  logical.w)` odbijen je na hardware provjeri: korisnik je potvrdio da su tekst
  i redoslijed tabova tada vodoravno zrcaljeni. Produkcijski landscape put zato
  ostaje na 1:1 mapiranju (`mirror_x=false`); pomoćni mirror helper nije aktivan.
- Nakon povratka na 1:1 mapiranje kandidat je ponovno izgrađen i puni image je
  flešan na factory (`0x20000`) preko `COM6`; SHA-256
  `87714FE170B6E0EE6C937397D4F33A75233A64A3B546B1A433CC4DBDD906377A`.
  Host suite je ponovno završio s exit 0. Čeka se korisnička potvrda da je
  GUI vizualno u očekivanoj orijentaciji.

### PPA RGB swap i timing kandidat

Korisnička CPU-proba pokazala je da se crvena/plava zamjenjuju, uz zaseban
horizontalni ciklički pomak; GUI je imao isti trag u bojama. Izolirani PPA
`rgb_swap=true` kandidat nije promijenio ni boje ni pomak na hardveru i zato je
odbačen, a produkcijski put vraćen je na `false`.

Za geometriju je pripremljen novi, odvojeni timing kandidat prema službenom
[Raspberry Pi `vc4-kms-dsi-waveshare-800x480` overlayu](https://raw.githubusercontent.com/raspberrypi/linux/rpi-6.18.y/arch/arm/boot/dts/overlays/vc4-kms-dsi-waveshare-800x480-overlay.dts):
jedna DSI lane, RGB888, 27,777 MHz, HFP/HSW/HBP `59/2/45` i VFP/VSW/VBP
`7/2/22`. Overlay koristi isti pomoćni panel-regulator na I²C `0x45`, ali
to još nije dokaz identiteta ovog DSI-506 modula. Nisu dodani vendor DSI
registri ni promjene orijentacije. Nakon instalacije ovog kandidata treba
ponoviti CPU-kocke i GUI; boje i geometrija bilježe se kao dva zasebna ishoda.

Prva hardverska provjera ovog kandidata pokazala je da su GUI boje sada
ispravne, dok horizontalni ciklički pomak i dalje postoji. Timing kandidat
zato ostaje aktivan za kanalno/poravnavajući dio problema, ali display
geometrija još nije prihvaćena. Sljedeći pokus mora prvo izmjeriti smjer i
veličinu pomaka (npr. odnos širine jedne kartice prema cijelom 800 px kadru)
prije promjene početka scanouta ili porchesa.

Korisnik je zatim precizirao da je kadar pomaknut udesno: `SETTINGS`, koji je
logički četvrti tab, pojavljuje se prvi, a dio njegova okvira omata se na desni
rub. To potvrđuje ciklički pomak cijelog scanouta, ne pogrešan LVGL redoslijed
tabova. Za sljedeći COM6 pokus CPU framebuffer umjesto četiri kvadranta crta
osam numeriranih vertikalnih polja `0..7`, svako točno 100 px široko. Proba i
dalje zaobilazi LVGL/PPA; redoslijed brojeva na fizičkom ekranu dat će smjer i
grubu veličinu faznog pomaka bez vendor register upisa.

Hardverski rezultat numerirane probe bio je `70123456`. To znači da se
posljednjih 100 px framebuffer kadra prikazuje prvo, a logički `x=0` počinje
oko fizičkog `x=100`: ciklički pomak je približno 100 px udesno. Aktivni
horizontalni blanking iznosi `59 + 2 + 45 = 106` pixel-clockova, što je gotovo
jednako izmjerenoj fazi. Zato sljedeći izolirani COM6 pokus mijenja samo DSI
video packetizaciju s prisilnog non-burst načina na IDF-ov zadani
`MIPI_DSI_LL_VIDEO_BURST_WITH_SYNC_PULSES`. Lane count/rate, RGB888, pixel
clock, svi porch/sync parametri, frame ACK, LVGL/PPA, Wi-Fi i audio ostaju
nepromijenjeni. Prihvatni rezultat numerirane probe je `01234567`; svaki drugi
redoslijed ili nestabilna slika odbacuje ovaj packetizacijski kandidat.

Korisnik je nakon instalacije burst kandidata potvrdio **"sad je ok"**:
horizontalno omatanje je nestalo, redoslijed i geometrija GUI-ja su ispravni,
a prethodno prihvaćene boje ostale su ispravne. `burst with sync pulses` zato
je prihvaćen za ovaj DSI-506/DYL0023 primjerak, dok je prisilni non-burst način
odbačen. Privremeni boot slijed od 15 s hardverskih traka, 12 s numeriranih
polja i 3 s crnog kadra uklonjen je iz normalnog firmwarea; LVGL se sada
pokreće odmah nakon inicijalizacije panela. Touch i zajednički display/audio/
USB/Wi-Fi soak ostaju zasebni acceptance koraci.

## Povijesna priprema (2026-08-26)

## Identifikacija

Korisnik je dostavio prodajnu fotografiju 5-inčnog plavog DSI modula i naveo
oznaku `DSI-506`. Fotografija se podudara s proizvodnom obitelji koja se prodaje
kao `DSI5061` odnosno `DSI5061-A`, ali puna oznaka, proizvođač i PCB revizija
moraju se pročitati sa stvarnog primjerka.

Referentna fotografija spremljena je kao
`docs/reference/DSI506_PRODUCT_REFERENCE.jpg` (SHA-256
`DDDC23FE0EC29081B3EA64A9CAB05A954CE2A86C93A8CD8A827E2B763BEFCCC3`).

Trenutno je razumno očekivati:

- 5,0-inčni IPS TFT, 800×480, nativni landscape;
- približno 60 Hz i 108,00×64,80 mm aktivne površine;
- Raspberry Pi-style 15-pin, 1,0-mm MIPI-DSI priključak;
- dvije DSI podatkovne lane;
- kapacitivni touch, vjerojatno FT5426/FT5x06 na I2C adresi `0x38`;
- tipku `Backlight` i zasebne `PWM/GND` padove za opcionalnu regulaciju;
- 3,3-V logiku/napajanje kroz DSI priključak.

Ovo još nije dovoljno za potvrdu DSI bridgea, video formata, porch timinga,
touch INT/RESET izvedbe, FFC orijentacije ili backlight polariteta.

Referentni izvori, provjereni 2026-08-26:

- Elecrow `DSI05379I` proizvod iste vidljive PCB obitelji:
  <https://www.elecrow.com/5-inch-dsi-display-ips-800-480-touch-screen-compatible-with-raspberry-pi-4b-3b-3b.html>
- Elecrow V1.4 user manual:
  <https://www.elecrow.com/download/product/DSI05379I/5inch-dsi-display_user_manual-v1.4.pdf>
- DSI5061 listing s istim specifikacijama i priborom:
  <https://eckstein-shop.de/50-Zoll-800x480-MIPI-DSI-IPS-Display-fuer-Raspberry-Pi-Kapazitiver-Touch-EN>

Dobavljačka dokumentacija navodi 3,3 V i približno 340 mA. Prodajna stranica na
jednom mjestu pogrešno ispisuje 3400 mA; manual i ostali zapisi navode 340 mA.
Desni `3V3/GND` header na fotografiji opisan je kao izlaz za mali 3,3-V
ventilator i ne smije se tretirati kao ulaz napajanja zaslona bez provjere.

## J2 kandidat pinout

Postojeći `bsp_p4_m3` dokumentira JC-ESP32P4-M3-DEV J2 ovako:

| J2 pin | Signal |
|---|---|
| 1 | GND |
| 2 | DSI data lane 1 N |
| 3 | DSI data lane 1 P |
| 4 | GND |
| 5 | DSI clock N |
| 6 | DSI clock P |
| 7 | GND |
| 8 | DSI data lane 0 N |
| 9 | DSI data lane 0 P |
| 10 | GND |
| 11 | touch I2C SCL |
| 12 | touch I2C SDA |
| 13 | GND |
| 14 | +3,3 V |
| 15 | +3,3 V |

Raspored odgovara standardnom Raspberry Pi 15-pin DSI konektoru, ali to ne
potvrđuje smjer kontakata priloženog FFC-a. Prije umetanja treba identificirati
pin 1, stranu izloženih kontakata i treba li kabel s kontaktima na istoj ili
suprotnoj strani. Kabel se umeće samo uz isključeno napajanje.

## Backlight

DSI 15-pin ne nosi zaseban PWM signal. Modul ima vlastitu `Backlight` tipku i
`PWM/GND` padove. Za prvi light-up ostaviti tvornički KEY/fixed-brightness način
i ne lemiti PWM vod. Tek nakon stabilne slike treba multimetrom i dokumentacijom
potvrditi polaritet/razinu, zatim po potrebi spojiti slobodan P4 PWM GPIO na
`PWM` i zajednički GND.

Postojeći `bsp_p4_m3` generira 5-kHz aktivno-visoki PWM na GPIO23, ali taj GPIO
nije dio J2 15-pin konektora. Settings slider zato neće upravljati ovim modulom
dok se fizički PWM put zasebno ne potvrdi i spoji.

## Stanje firmwarea prije dolaska

Pripremljena, ali još neaktivna komponenta:

- `firmware/main-deck-p4/components/bsp_p4_m3/bsp_p4_m3.c`
- `firmware/main-deck-p4/components/bsp_p4_m3/include/bsp_p4_m3.h`
- `firmware/main-deck-p4/components/bsp_p4_m3/include/bsp_jc4880.h`

Ona trenutno pretpostavlja:

- dvije DSI lane na 500 Mbps;
- DPI clock 30 MHz;
- H timing `40/40/40` za sync/back/front porch;
- V timing `9/29/13`;
- 800×480, RGB565 i jedan framebuffer;
- čisti DPI video bez vendor DBI/DCS init sekvence;
- FT5x06 driver na `0x38`, GPIO7/8, bez zasebnih INT/RESET pinova;
- nativni landscape bez swap/mirror transformacije;
- GPIO23 kao 5-kHz backlight PWM.

UI backend već ima nativni 800×480 put s `PPA_SRM_ROTATION_ANGLE_0`, a
`dependencies.lock` već sadrži `esp_lcd_touch_ft5x06`.

Aktualni build ipak još zahtijeva i kompilira stari `bsp_jc4880` za
ST7701S/GT911. `bsp_p4_m3` nije aktivan samo zato što postoji u stablu. Prvi
display commit mora kontrolirano prebaciti component dependency s
`bsp_jc4880` na `bsp_p4_m3` najmanje u:

- `firmware/main-deck-p4/main/CMakeLists.txt`;
- `firmware/main-deck-p4/components/audio_engine/CMakeLists.txt`;
- `firmware/main-deck-p4/components/ui/CMakeLists.txt`;
- statičkim BSP provjerama u `tests/run_p4_host_tests.ps1`.

Postojeći `bsp_jc4880.h` compatibility redirect unutar novog BSP-a dopušta da se
C includeovi migriraju odvojeno. Stari BSP ne uklanjati dok novi panel, touch,
PCM5102A, microSD i C6/SDIO ne prođu zajednički acceptance.

## Što fotografirati odmah po dolasku

Bez napajanja snimiti:

1. cijelu prednju i stražnju stranu ravno odozgo;
2. punu `DSI-506...` oznaku, sufiks i `REV`;
3. oznake oba veća IC-a i svih naljepnica;
4. oznaku na centralnom panel FPC-u;
5. DSI konektor izbliza, s vidljivim pinom 1 i položajem zasuna;
6. oba kraja svakog priloženog FFC kabela, jedan pokraj drugoga;
7. označena IPS/touch polja na PCB-u, ako su popunjena;
8. `Backlight`, selector/0R, `PWM/GND` i `3V3/GND` područja;
9. pakiranje, SKU i eventualni priloženi list/pinout.

Zapisati broj pinova, pitch, jesu li kontakti FFC-a same-side/opposite-side te
izmjeriti koji su pinovi GND. Ne raditi continuity mjerenje preko DSI parova uz
napajanje.

## Sigurni redoslijed bring-upa

### 0. Priprema builda

1. Sačuvati poznati `M3-41-g133f399 / ota_0` rollback baseline.
2. Prebaciti dependencyje i statičke testove na `bsp_p4_m3` u zasebnom commitu.
3. Pokrenuti puni host suite i clean ESP-IDF 6.0.2 build.
4. Prvi display kandidat instalirati preko USB1/serijskog flashing puta, ne
   isključivo OTA-om, jer pogrešan rani display init može spriječiti Wi-Fi boot.

### 1. Prvi light-up

1. Isključiti napajanje prije umetanja FFC-a.
2. Potvrditi pin 1 i orijentaciju kontakata na oba konektora.
3. Ostaviti vanjski PWM nepovezan i koristiti tvornički backlight način.
4. Stišati vanjsko audio pojačalo; oba decka moraju biti zaustavljena.
5. Prvi boot promatrati na serijskom monitoru zbog brownouta, reseta, DSI/PSRAM
   grešaka ili neuobičajenog grijanja.
6. Potvrditi puni kadar najprije statičkim bijelim/crnim/crvenim/zelenim/plavim
   uzorkom, zatim LVGL UI-em.

Ako postoji backlight bez slike, ne mijenjati nasumično timing. Prvo pročitati
IC oznake, skenirati J2 I2C sabirnicu i utvrditi treba li DSI bridgeu I2C ili
DBI/DCS inicijalizacija. Ako postoji slika s krivim bojama, provjeriti očekuje li
bridge RGB565 ili RGB888 prije promjene cijelog UI framebuffera.

### 2. Touch

1. Napraviti I2C scan GPIO7/8 i zapisati sve adrese.
2. Potvrditi očekivani `0x38` prije vezivanja FT5x06 drivera.
3. Provjeriti sva četiri kuta, centar i rubove.
4. Utvrditi `swap_xy`, `mirror_x`, `mirror_y` iz mjerenja, ne iz fotografije.
5. Provjeriti press/release, drag i najmanje dva istodobna dodira ako controller
   i UI to podržavaju.

### 3. UI i Master Tempo

1. Proći Overview D1/D2, Library, Hot Cues, Settings i screensaver.
2. Potvrditi Shift + Browse/Load tok s vidljivim Library ekranom.
3. Provjeriti backlight slider tek nakon fizičkog PWM rješenja.
4. Uključiti Master Tempo kroz UI i odraditi dual-deck keylock test sa suprotnim
   pitch vrijednostima uz nadzor PSRAM-a i output deadlinea.
5. Vizualno pregledati snimke prije promjene screenshot baselinea.

### 4. Zajednički soak

Istodobno držati aktivnima DSI/touch/UI, PCM5102A master, FLX4 MIDI/UAC,
USB3 library/playback i Wi-Fi. Acceptance traži stabilnu sliku i touch, bez DSI
underruna, brownouta, reseta, PCM underruna, UAC drop/overflowa ili servisnog
log dropa.

## Obvezne provjere

```powershell
. C:\Espressif\tools\Microsoft.v6.0.2.PowerShell_profile.ps1
idf.py --version
$repoRoot = git rev-parse --show-toplevel
Set-Location "$repoRoot\firmware\main-deck-p4"
idf.py fullclean
idf.py build
Set-Location $repoRoot
$env:Path = "$env:Path;C:\msys64\ucrt64\bin"
.\tests\run_p4_host_tests.ps1
.\tests\ui_simulator\run_ui_simulator_e2e.ps1
```

Nakon vizualnog hardware acceptancea ponoviti build bez `-dirty`, instalirati
potpisani image, odraditi startup checklist, ažurirati aktivnu dokumentaciju i
tek tada zatvoriti display gate.

## Acceptance zapis koji treba popuniti

```text
PCB/model/revision:
Panel FPC oznaka:
DSI bridge IC:
Touch IC i I2C adresa:
FFC tip i orijentacija:
Napajanje i izmjerena struja:
DSI lane count/rate:
Pixel format i timing:
Backlight način/PWM polaritet:
Touch transformacija:
Cold/warm boot rezultat:
UI/Master Tempo rezultat:
DSI/PCM/UAC/service counter delte:
Prihvaćeni firmware commit/version/slot:
```
