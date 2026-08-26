# ESP32-P4 active pinout

Status: 2026-08-26. Autoritativni wiring je `docs/HARDWARE_WIRING.md`.

USB1/2/3, C6/Wi-Fi i PCM5102A trenutno su aktivni na benchu. DSI/FT5426 zaslon
još nije stigao; njegovi redovi ispod rezerviraju ciljni put i ne znače da je
display hardware acceptance završen.

| Funkcija | Pin / port |
|---|---|
| PCM5102A BCK/BCLK | GPIO1 |
| PCM5102A LCK/LRCK/WS | GPIO2 |
| PCM5102A DIN | GPIO3 |
| PCM5102A SCK | GND |
| PCM5102A GND / VIN | GND / 5 V |
| DDJ-FLX4 MIDI/UAC1 | USB2 FS Host |
| Rekordbox MSC | USB3 HS Host |
| power/flash/monitor | USB1 CH340C |
| display | board MIPI-DSI connector |
| touch | board FT5426 connection |
| Wi-Fi | integrated ESP32-C6 over SDIO |

Mikrofon, NS4150 speaker amp i Ethernet su softverski ugašeni. GPIO pinovi koji
nisu navedeni ovdje nisu automatski slobodni; provjeri BSP i shemu ploče.

Korišteni PCM5102A modul mora imati `H1=L`, `H2=L`, `H3=H`, `H4=L`. Detalji i
objašnjenje konfiguracijskih mostova nalaze se u `docs/HARDWARE_WIRING.md`.
