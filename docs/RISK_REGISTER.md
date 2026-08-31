# Risk Register

Status: aktivni rizici single-chip P4 izvedbe, 2026-08-31.

| ID | Rizik | Posljedica | Mitigacija / gate |
|---|---|---|---|
| R1 | P4 USB host istodobno nosi FLX4 FS i MSC HS | disconnect, starvation ili audio drop | odvojeni portovi i fail-closed per-controller FIFO patch nad pinanim `espressif/usb 1.5.0`, bez izmjene `managed_components`; clean-build OTA, 600-s mixed-rate USB2/USB3/Wi-Fi soak, FLX4 hot-plug i USB3 media remove/reinsert prošli bez PCM/UAC gubitka |
| R2 | FLX4 UAC1 descriptor/alternate-setting ili ring tlak odstupa od pretpostavke | nema slušalica, krivi format ili drop | descriptor testovi, hardware enumeration smoke, fail-closed endpoint izbor te active-playback 1/4–3/4 ring alarm; `M3-39` 48/44,1-kHz smoke potvrdio je priming-baseline suppression bez idle/start lažnih incidenata |
| R3 | Audio output task probije blok deadline | čujni klik/underrun | single-precision DSP, bounded I/O, bez real-time success logiranja, rano spuštanje USB prioriteta, phase telemetry i alarm na dva output bloka; 600-s soak imao je četiri izolirana 10,732–11,659-ms događaja bez PCM/UAC posljedice, a hot-plug delta bila je 0 |
| R4 | USB storage read blokira decode/output | dropout pri library/load aktivnostima | compressed cache i SD/USB I/O gate; 600-s soak s 64 library čitanja, 13 reload/seek ciklusa i fizički 191→0→191 media replacement prošli su bez PCM/UAC gubitka |
| R5 | MIDI mapping ili shift state nije potpun | pogrešna kontrola ili LED | autoritativni Mixxx XML, službeni MIDI popis i map acceptance ledger; shifted Beat Jump/Call, Loop Adjust `0x4C/0x4E`, D1/D2 Quantize, Censor, Sync Master, Reloop Stop/Forget, Beat FX i inertni Smart helperi hardverski potvrđeni 2026-08-24; zaslon je sada dostupan pa su Shift + Browse/Load sljedeći eyes-on gate |
| R6 | Reconnect ostavi stale FLX4 state/LED | UI i fizički kontroler se ne slažu | generation gate, connection event i puni LED snapshot nakon reconnecta |
| R7 | C6 SDIO i microSD dijele resurse na neočekivan način | mreža ili SD ne rade | slot-aware BSP, IDF6 hardware smoke, APSTA servisni posjet koji čuva Hosted i montirani microSD te zabrana Hosted teardowna dok kartica koristi drugi slot istog SDMMC kontrolera |
| R8 | PCM5102A pinovi kolidiraju s aktivnom periferijom ili fizički modul odstupa od pretpostavke | nema master zvuka, šum ili krivi L/R | zatvoreno 2026-08-26: GPIO1/2/3, SCK=GND i H1=L/H2=L/H3=H/H4=L dali su čisti stereo, tihi idle, 44,1/48-kHz switching i prihvatljivu limiter marginu; otvoreni H1-H4 reproducirali su glasni modulirani šum pa konfiguracija ostaje obvezni startup check |
| R9 | OTA paket nije vezan uz P4 target | nebootabilna slika | signed manifest, chip/project/version provjera prije `esp_ota_begin`; lokalni upload i produkcijski HTTPS pull `M3-22` prošli |
| R10 | Dokumentacija ponovno pomiješa povijesnu i važeću topologiju | pogrešan wiring ili razvojni smjer | aktivni docs index; validation/spec mape označene kao povijesne |
| R11 | Settings gašenje se sudari sa servisnim APSTA/OTA prijelazom | srušen netif, prekinut download ili AP koji se ne vrati | transition lease, host-testirani START/STOP/WAIT policy, APSTA bez gašenja AP/DHCP/HTTP puta te fizički connectivity, HTTPS channel i missing-bundle 404 smoke; fault je obnovio AP bez reboota |
| R12 | I2S master output radi na 48 kHz dok je FLX4 UAC fiksiran na 44,1 kHz | kriva brzina/visina tona ili headphone ring overflow na 48-kHz trakama | implementiran stateful 48→44,1-kHz headphone resampler; host omjer/kontinuitet, 60-s 48-kHz gate, 600-s mixed-rate soak i FLX4 slušni acceptance prošli; PCM5102A 44,1/48-kHz i mixed-rate hardware gate zatvoren 2026-08-26 bez krive brzine/visine tona ili UAC gubitka |
| R13 | Same-version channel check neposredno nakon OTA boota jednom je izazvao PANIC reset | neočekivani restart u maintenance toku | tri svježa post-OTA equality ciklusa uz test-only 64-KiB flash coredump vratila su `already running this build` bez PANIC-a; dump particija ostala je prazna, produkcijski image vraćen je i rizik ostaje samo pod monitoringom |
| R14 | DSI bridge identitet nije poznat i prihvaćeni parametri potječu iz kontroliranog hardware bring-upa, ne vendor datasheeta | buduća revizija modula može ostati bijela, imati krive boje ili wrap | za isporučeni EYOYO `DSI506 / DYL0023` zatvoreni su FFC, backlight, RGB888, 1-lane/800-Mbps, timing, burst packetizacija, boje i poravnanje; ne slati nagađane vendor upise, zadržati točan model/reviziju u wiring dokumentu i ponovno otvoriti gate za drugi modul |
| R15 | FT5426 na `0x38` je osjetljiv na I2C brzinu i tvornička orijentacija zrcali obje osi | nema dodira, zamijenjene osi ili nestabilan LVGL input | read greške uklonjene vraćanjem na 100 kHz; prihvaćeno mapiranje je `swap_xy=0`, `mirror_x=1`, `mirror_y=1`; kartice, Backlight drag i obje strane prošli su 2026-08-31, a corner/multitouch, screensaver i dugi integration soak ostaju pod monitoringom |

Potpisani noviji signed-bundle install prošao je i lokalnim web uploadom i
produkcijskim HTTPS pull tokom, a missing-bundle download fault završio je bez
reboota ili promjene slota. Namjerni nepotvrđeni `pending_verify` image također
je automatski vraćen na prethodni valjani slot. OTA acceptance blok je zatvoren;
izolirani post-OTA PANIC ostaje rezidualna monitoring stavka nakon tri neuspjela
pokušaja reprodukcije s coredumpom. Desetominutni dual-USB/audio/Wi-Fi soak,
FLX4 reconnect pod playbackom i USB3 media remove/reinsert su zatvoreni.
PCM5102A headroom/limiter/noise gate zatvoren je 2026-08-26. DSI image i
fokusirani touch gate za EYOYO `DSI506 / DYL0023` zatvoreni su 2026-08-31;
otvoreni su corner/multitouch, screensaver, eyes-on UI funkcije i zajednički
display/master/headphones/dual-deck/Wi-Fi soak. Jednokratni simultani
start/seek PCM D1=202 događaj nije se
ponovio u ukupno 25 kontroliranih startova i ostaje telemetrijska stavka, a ne
potvrđeni reproducibilni kvar. Povijesni Censor MVP radio je dva playing seeka;
svaki je u prihvatnom testu dodao jedan output-late događaj i 256 PCM-underrun
frameova. Source ga sada zamjenjuje bounded gapless slip-reverse DSP-om bez
seeka ili dodatnog PCM buffera. Puni host suite i P4 build prolaze, a D1 48-kHz
i D2 44,1-kHz hardware smoke na `M3-41-g133f399` potvrdio je čujni reverse,
gladak release i nulte kontrolirane output-late/PCM/UAC/service-log delte; taj je
Censor rizik zatvoren.
