# Hardware Wiring

Status: važeća single-chip topologija i plan spajanja, 2026-08-26.

Trenutni bench ima spojene USB1, USB2/FLX4, USB3/Rekordbox medij i PCM5102A.
DAC wiring i konfiguracija ispod hardverski su potvrđeni 2026-08-26.
5,0-inčni DSI/FT5426 zaslon još nije stigao, pa display upute ostaju ciljni
wiring za zasebni bring-up.

## USB

| Priključak | Uloga | Uređaj |
|---|---|---|
| USB1 | 5 V, flashing, serial monitor | CH340C / razvojno računalo |
| USB2 | FS USB Host, 12 Mbps | Pioneer DDJ-FLX4: MIDI In/Out + UAC1 OUT |
| USB3 | HS USB Host, 480 Mbps | Rekordbox USB Flash / MSC |

USB2 i USB3 su neovisni host portovi. Ne umeći vanjski hub i ne spajaj FLX4 na
debug/power port.

## PCM5102A master DAC

| P4 / napajanje | PCM5102A signal |
|---|---|
| GPIO1 | BCK / BCLK |
| GPIO2 | LCK / LRCK / WS |
| GPIO3 | DIN |
| GND | SCK |
| GND | GND |
| 5 V | VIN |

Na korištenom ljubičastom PCM5102A modulu zalemljeni su konfiguracijski mostovi:

| Most | Položaj | Funkcija |
|---|---|---|
| H1 | L | FLT low |
| H2 | L | DEMP low |
| H3 | H | XSMT high / unmute |
| H4 | L | FMT low / standard I2S |

Spaja se srednji pad samo prema navedenoj H/L strani, ne sva tri pada. `SCK`
ostaje na GND radi internog BCK PLL načina. Otvoreni konfiguracijski mostovi
uzrokovali su početni glasni šum moduliran glazbom; gornja konfiguracija dala je
čist stereo izlaz i tihi idle.
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

Prije uključivanja provjeri GND, 5 V polaritet, svih šest DAC vodova i H1-H4
mostove te da su USB2 i USB3 spojeni na odgovarajuće uređaje. DSI
konektor/panel identitet provjeri prije display brancha.
