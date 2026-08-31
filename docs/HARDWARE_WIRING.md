# Hardware Wiring

Status: važeća single-chip topologija i potvrđeno bench spajanje, 2026-08-31.

Trenutni bench ima spojene USB1, USB2/FLX4, USB3/Rekordbox medij, PCM5102A i
EYOYO `DSI506 / DYL0023`. DAC wiring potvrđen je 2026-08-26, a DSI slika,
backlight, boje, nativni landscape, poravnanje i fokusirani touch gate
2026-08-31. Touch koristi 100-kHz I2C te `swap_xy=0`, `mirror_x=1`,
`mirror_y=1`.

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

- Spojeni modul je EYOYO `DSI506 / DYL0023`: 5,0", 800×480 IPS, 15-pin
  Raspberry Pi-style MIPI-DSI i kapacitivni touch. FFC orijentacija i J2
  spajanje fizički su potvrđeni; kabel se i dalje umeće samo bez napajanja.
- Aktualni scanout koristi DSI lane 0 i clock; lane 1 fizički postoji na J2, ali
  ovaj prihvaćeni profil ga ne koristi. Video je 1 lane / 800 Mbps, RGB888,
  27,777 MHz, HFP/HSW/HBP `59/2/45`, VFP/VSW/VBP `7/2/22`, burst sync pulses i
  bez frame ACK-a.
- J2 nosi 3,3 V/GND i zajednički I2C. Scan nalazi `0x18`, touch-kandidat
  `0x38` i panel power/backlight kontroler `0x45` (`ID=C3`, `ID2=8B`). Bridge
  identitet ostaje nepoznat; firmware ne šalje nagađane vendor init upise.
- Backlight radi preko tvorničkog `0x45` kontrolera i lokalnog gumba koji
  stupnjevito mijenja svjetlinu. Vanjski `PWM/GND` nije spojen i 0-ohm selektor
  nije premješten. `FAN 3V3/GND` je samo mjerno mjesto (izmjereno 3,3 V), ne
  ulaz dodatnog napajanja zaslona.
- `0x38` je FT5426/FT5x06 touch put. Stabilan rad traži 100-kHz I2C; raniji
  400-kHz pre-arrival override davao je runtime read greške i uklonjen je.
  Nativno landscape mapiranje je `swap_xy=0`, `mirror_x=1`, `mirror_y=1`.
  Kartice, Backlight drag i obje strane Overviewa fizički su potvrđeni; točan
  corner/multitouch, screensaver i dugi integration soak ostaju zasebni gateovi.
- Detalji i odbijeni display kandidati su u
  `docs/DISPLAY_DSI506_BRINGUP.md`.
- ESP32-C6 koristi ESP-Hosted preko SDIO; ne mijenjati SDIO pinove bez revizije
  BSP-a i microSD dijeljenja kontrolera.

## Namjerno neaktivno

- ugrađeni mikrofon;
- NS4150 mono speaker amp;
- RJ45 Ethernet / EMAC;
- bilo kakav UART prema pomoćnom kontrolnom MCU-u;
- međupanački I2S/PCM transport.

Prije uključivanja provjeri GND, 5 V polaritet, svih šest DAC vodova i H1-H4
mostove te da su USB2 i USB3 spojeni na odgovarajuće uređaje. DSI FFC se
premješta samo bez USB napajanja i bez umetnute SD kartice.
