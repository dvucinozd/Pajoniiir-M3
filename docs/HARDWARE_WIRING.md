# Hardware Wiring

Status: važeća single-chip topologija, 2026-08-22.

## USB

| Priključak | Uloga | Uređaj |
|---|---|---|
| USB1 | 5 V, flashing, serial monitor | CH340C / razvojno računalo |
| USB2 | FS USB Host, 12 Mbps | Pioneer DDJ-FLX4: MIDI In/Out + UAC1 OUT |
| USB3 | HS USB Host, 480 Mbps | Rekordbox USB Flash / MSC |

USB2 i USB3 su neovisni host portovi. Ne umeći vanjski hub i ne spajaj FLX4 na
debug/power port.

## PCM5102A master DAC

| P4 GPIO | PCM5102A signal |
|---|---|
| GPIO1 | BCLK |
| GPIO2 | LRCK / WS |
| GPIO3 | DIN |

Spoji zajednički GND i napajanje prema specifikaciji konkretnog DAC modula.
Master izlaz se ne vodi kroz ugrađeni NS4150.

## Zaslon, touch i mreža

- 5.0" 800×480 panel koristi pločin MIPI-DSI priključak u nativnom landscapeu.
- FT5426 touch koristi tvornički spoj ploče.
- ESP32-C6 koristi ESP-Hosted preko SDIO; ne mijenjati SDIO pinove bez revizije
  BSP-a i microSD dijeljenja kontrolera.

## Namjerno neaktivno

- ugrađeni mikrofon;
- NS4150 mono speaker amp;
- RJ45 Ethernet / EMAC;
- bilo kakav UART prema pomoćnom kontrolnom MCU-u;
- međupanački I2S/PCM transport.

Prije prvog uključivanja provjeri GND, 5 V polaritet, DAC pinove i da su USB2 i
USB3 spojeni na odgovarajuće uređaje.
