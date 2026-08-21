# Risk Register

Status: aktivni rizici single-chip P4 izvedbe, 2026-08-22.

| ID | Rizik | Posljedica | Mitigacija / gate |
|---|---|---|---|
| R1 | P4 USB host istodobno nosi FLX4 FS i MSC HS | disconnect, starvation ili audio drop | odvojeni portovi, bounded queue/ring, reconnect soak i brojači dropova |
| R2 | FLX4 UAC1 descriptor/alternate-setting odstupa od pretpostavke | nema slušalica ili krivi format | descriptor testovi, hardware enumeration smoke, fail-closed endpoint izbor |
| R3 | Audio output task probije blok deadline | čujni klik/underrun | single-precision DSP, bounded I/O, phase timing telemetry i soak |
| R4 | USB storage read blokira decode/output | dropout pri library/load aktivnostima | compressed cache, SD/USB I/O gate i dual-deck load soak |
| R5 | MIDI mapping ili shift state nije potpun | pogrešna kontrola ili LED | autoritativni Mixxx XML, službeni MIDI popis i map acceptance ledger |
| R6 | Reconnect ostavi stale FLX4 state/LED | UI i fizički kontroler se ne slažu | generation gate, connection event i puni LED snapshot nakon reconnecta |
| R7 | C6 SDIO i microSD dijele resurse na neočekivan način | mreža ili SD ne rade | slot-aware BSP, IDF6 hardware smoke i zabrana ad-hoc pin promjena |
| R8 | PCM5102A pinovi kolidiraju s aktivnom periferijom | nema master zvuka | GPIO1/2/3 kao jedini dokumentirani master put; EMAC ugašen |
| R9 | OTA paket nije vezan uz P4 target | nebootabilna slika | signed manifest, chip/project/version provjera prije `esp_ota_begin` |
| R10 | Dokumentacija ponovno pomiješa povijesnu i važeću topologiju | pogrešan wiring ili razvojni smjer | aktivni docs index; validation/spec mape označene kao povijesne |
| R11 | Settings gašenje se sudari s probe/OTA AP→STA→AP prijelazom | srušen netif, prekinut download ili AP koji se ne vrati | transition lease, host-testirani START/STOP/WAIT policy i hardware fault-injection smoke |

Najveći otvoreni acceptance rizik je cjeloviti hardware soak izravnog FLX4
MIDI/UAC puta uz istodobni USB3 streaming i aktivan UI.
