# ESP32-P4 active pinout

Status: 2026-08-24. Autoritativni wiring je `docs/HARDWARE_WIRING.md`.

USB1/2/3 i C6/Wi-Fi trenutno su aktivni na benchu. PCM5102A još nije fizički
spojen, a DSI/FT5426 zaslon još nije stigao; njihovi redovi ispod rezerviraju
ciljni put i ne znače da je pripadajući hardware acceptance završen.

| Funkcija | Pin / port |
|---|---|
| PCM5102A BCLK | GPIO1 |
| PCM5102A LRCK/WS | GPIO2 |
| PCM5102A DIN | GPIO3 |
| DDJ-FLX4 MIDI/UAC1 | USB2 FS Host |
| Rekordbox MSC | USB3 HS Host |
| power/flash/monitor | USB1 CH340C |
| display | board MIPI-DSI connector |
| touch | board FT5426 connection |
| Wi-Fi | integrated ESP32-C6 over SDIO |

Mikrofon, NS4150 speaker amp i Ethernet su softverski ugašeni. GPIO pinovi koji
nisu navedeni ovdje nisu automatski slobodni; provjeri BSP i shemu ploče.
