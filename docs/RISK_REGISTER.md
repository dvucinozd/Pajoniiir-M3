# Risk Register

Status: aktivni rizici single-chip P4 izvedbe, 2026-08-23.

| ID | Rizik | Posljedica | Mitigacija / gate |
|---|---|---|---|
| R1 | P4 USB host istodobno nosi FLX4 FS i MSC HS | disconnect, starvation ili audio drop | odvojeni portovi, bounded queue/ring, reconnect soak i brojači dropova |
| R2 | FLX4 UAC1 descriptor/alternate-setting odstupa od pretpostavke | nema slušalica ili krivi format | descriptor testovi, hardware enumeration smoke, fail-closed endpoint izbor |
| R3 | Audio output task probije blok deadline | čujni klik/underrun | single-precision DSP, bounded I/O, zabrana packet-level i periodičnog success logiranja u FLX4 real-time callbacku, rano spuštanje prioriteta na canceled/no-device transferu, phase timing telemetry i soak; headphone-level, USB2 disconnect i fizički multi-client gateovi prošli bez latea |
| R4 | USB storage read blokira decode/output | dropout pri library/load aktivnostima | compressed cache, SD/USB I/O gate i dual-deck load soak |
| R5 | MIDI mapping ili shift state nije potpun | pogrešna kontrola ili LED | autoritativni Mixxx XML, službeni MIDI popis i map acceptance ledger |
| R6 | Reconnect ostavi stale FLX4 state/LED | UI i fizički kontroler se ne slažu | generation gate, connection event i puni LED snapshot nakon reconnecta |
| R7 | C6 SDIO i microSD dijele resurse na neočekivan način | mreža ili SD ne rade | slot-aware BSP, IDF6 hardware smoke, APSTA servisni posjet koji čuva Hosted i montirani microSD te zabrana Hosted teardowna dok kartica koristi drugi slot istog SDMMC kontrolera |
| R8 | PCM5102A pinovi kolidiraju s aktivnom periferijom | nema master zvuka | GPIO1/2/3 kao jedini dokumentirani master put; EMAC ugašen |
| R9 | OTA paket nije vezan uz P4 target | nebootabilna slika | signed manifest, chip/project/version provjera prije `esp_ota_begin`; lokalni upload i produkcijski HTTPS pull `M3-22` prošli |
| R10 | Dokumentacija ponovno pomiješa povijesnu i važeću topologiju | pogrešan wiring ili razvojni smjer | aktivni docs index; validation/spec mape označene kao povijesne |
| R11 | Settings gašenje se sudari sa servisnim APSTA/OTA prijelazom | srušen netif, prekinut download ili AP koji se ne vrati | transition lease, host-testirani START/STOP/WAIT policy, APSTA bez gašenja AP/DHCP/HTTP puta i fizički connectivity + HTTPS channel smoke |
| R12 | PCM5102A output radi na 48 kHz dok je FLX4 UAC fiksiran na 44,1 kHz | kriva brzina/visina tona ili headphone ring overflow na 48-kHz trakama | implementiran stateful 48→44,1-kHz resampler; host omjer/kontinuitet, 60-s 48-kHz gate, 6-min mixed-rate soak i FLX4 slušni acceptance prošli |
| R13 | Same-version channel check neposredno nakon OTA boota jednom je izazvao PANIC reset | neočekivani restart u maintenance toku | događaj je dokumentiran; odgođene kontrolirane provjere prolaze bez promjene boot ID-a, a reprodukcija s uključenim coredumpom ostaje otvorena prije zatvaranja rizika |

Potpisani noviji signed-bundle install prošao je i lokalnim web uploadom i
produkcijskim HTTPS pull tokom. Najveći otvoreni acceptance rizici sada su OTA
download fault, firmware-health rollback i izolirani post-OTA equality-check
PANIC, duži dual-USB stress s aktivnim UI-jem te bring-up još nedostupnog
800×480 DSI/FT5426 sklopa. Jednokratni simultani start/seek PCM D1=202 događaj
nije se ponovio u 12 kontroliranih ciklusa i ostaje telemetrijska soak stavka,
a ne potvrđeni reproducibilni kvar.
