# Risk Register

Status: aktivni rizici single-chip P4 izvedbe, 2026-08-24.

| ID | Rizik | Posljedica | Mitigacija / gate |
|---|---|---|---|
| R1 | P4 USB host istodobno nosi FLX4 FS i MSC HS | disconnect, starvation ili audio drop | odvojeni portovi i fail-closed per-controller FIFO patch nad pinanim `espressif/usb 1.5.0`, bez izmjene `managed_components`; clean-build OTA, 600-s mixed-rate USB2/USB3/Wi-Fi soak, FLX4 hot-plug i USB3 media remove/reinsert prošli bez PCM/UAC gubitka |
| R2 | FLX4 UAC1 descriptor/alternate-setting odstupa od pretpostavke | nema slušalica ili krivi format | descriptor testovi, hardware enumeration smoke, fail-closed endpoint izbor |
| R3 | Audio output task probije blok deadline | čujni klik/underrun | single-precision DSP, bounded I/O, bez real-time success logiranja, rano spuštanje USB prioriteta i phase telemetry; 600-s soak imao je četiri izolirana 10,732–11,659-ms događaja bez PCM/UAC posljedice, a hot-plug delta bila je 0 |
| R4 | USB storage read blokira decode/output | dropout pri library/load aktivnostima | compressed cache i SD/USB I/O gate; 600-s soak s 64 library čitanja, 13 reload/seek ciklusa i fizički 191→0→191 media replacement prošli su bez PCM/UAC gubitka |
| R5 | MIDI mapping ili shift state nije potpun | pogrešna kontrola ili LED | autoritativni Mixxx XML, službeni MIDI popis i map acceptance ledger |
| R6 | Reconnect ostavi stale FLX4 state/LED | UI i fizički kontroler se ne slažu | generation gate, connection event i puni LED snapshot nakon reconnecta |
| R7 | C6 SDIO i microSD dijele resurse na neočekivan način | mreža ili SD ne rade | slot-aware BSP, IDF6 hardware smoke, APSTA servisni posjet koji čuva Hosted i montirani microSD te zabrana Hosted teardowna dok kartica koristi drugi slot istog SDMMC kontrolera |
| R8 | PCM5102A pinovi kolidiraju s aktivnom periferijom | nema master zvuka | GPIO1/2/3 kao jedini dokumentirani master put; EMAC ugašen |
| R9 | OTA paket nije vezan uz P4 target | nebootabilna slika | signed manifest, chip/project/version provjera prije `esp_ota_begin`; lokalni upload i produkcijski HTTPS pull `M3-22` prošli |
| R10 | Dokumentacija ponovno pomiješa povijesnu i važeću topologiju | pogrešan wiring ili razvojni smjer | aktivni docs index; validation/spec mape označene kao povijesne |
| R11 | Settings gašenje se sudari sa servisnim APSTA/OTA prijelazom | srušen netif, prekinut download ili AP koji se ne vrati | transition lease, host-testirani START/STOP/WAIT policy, APSTA bez gašenja AP/DHCP/HTTP puta te fizički connectivity, HTTPS channel i missing-bundle 404 smoke; fault je obnovio AP bez reboota |
| R12 | PCM5102A output radi na 48 kHz dok je FLX4 UAC fiksiran na 44,1 kHz | kriva brzina/visina tona ili headphone ring overflow na 48-kHz trakama | implementiran stateful 48→44,1-kHz resampler; host omjer/kontinuitet, 60-s 48-kHz gate, novi 600-s mixed-rate soak i FLX4 slušni acceptance prošli |
| R13 | Same-version channel check neposredno nakon OTA boota jednom je izazvao PANIC reset | neočekivani restart u maintenance toku | tri svježa post-OTA equality ciklusa uz test-only 64-KiB flash coredump vratila su `already running this build` bez PANIC-a; dump particija ostala je prazna, produkcijski image vraćen je i rizik ostaje samo pod monitoringom |

Potpisani noviji signed-bundle install prošao je i lokalnim web uploadom i
produkcijskim HTTPS pull tokom, a missing-bundle download fault završio je bez
reboota ili promjene slota. Namjerni nepotvrđeni `pending_verify` image također
je automatski vraćen na prethodni valjani slot. OTA acceptance blok je zatvoren;
izolirani post-OTA PANIC ostaje rezidualna monitoring stavka nakon tri neuspjela
pokušaja reprodukcije s coredumpom. Desetominutni dual-USB/audio/Wi-Fi soak,
FLX4 reconnect pod playbackom i USB3 media remove/reinsert su zatvoreni.
Najveći otvoreni hardverski gateovi sada su PCM5102A headroom/limiter rubovi i
bring-up još nedostupnog 800×480 DSI/FT5426 sklopa. Jednokratni simultani
start/seek PCM D1=202 događaj nije se
ponovio u ukupno 25 kontroliranih startova i ostaje telemetrijska stavka, a ne
potvrđeni reproducibilni kvar.
