# PCM5102A Master Output Acceptance, 2026-08-26

Status: PASS na instaliranom `M3-41-g133f399 / ota_0` imageu.

## Wiring i konfiguracija

| P4 / napajanje | PCM5102A |
|---|---|
| GPIO1 | BCK |
| GPIO2 | LCK |
| GPIO3 | DIN |
| GND | SCK |
| GND | GND |
| 5 V | VIN |

Na korištenom modulu zalemljeno je `H1=L`, `H2=L`, `H3=H`, `H4=L`, odnosno
FLT low, DEMP low, XSMT high/unmute i FMT low/standard I2S. Svaki most spaja
srednji pad samo prema navedenoj H/L strani. Otvoreni H1-H4 mostovi proizveli su
glasni šum moduliran glazbom; nakon konfiguriranja izlaz je čist.

## Acceptance

| Gate | Rezultat |
|---|---|
| 48-kHz single-deck | PASS; normalna brzina/visina tona i oba kanala |
| 44,1-kHz single-deck | PASS; runtime I2S rate switch, normalna brzina/visina tona |
| Idle noise | PASS; bez čujnog zujanja ili šuma nakon STOP-a |
| 44,1/48-kHz mixed-rate dual-deck | PASS; čist miks, limiter 0 na srednjem masteru |
| Full-master dual-deck, 15 s | PASS; peak 48.584, limiter 4.090 / približno 1.323.000 stereo sampleova (oko 0,31 %) |
| Full-master single-deck, 15 s | PASS; limiter delta 212 / približno 1.323.000 stereo sampleova (oko 0,016 %) |
| PCM underrun | PASS; kontrolirane delte 0/0 |
| FLX4 UAC drop/overflow | PASS; kontrolirane delte 0/0 |
| Slušni rezultat | PASS; bez clippinga, pucketanja, prekida ili očitog pumpinga |

Tijekom cijelog PCM bloka nastala su dva izolirana `output_late` događaja. Jedan
se pojavio u duljem mixed-rate testu, a jedan u full-master dual-deck startnom
prozoru. Nisu pratili PCM underrun, UAC drop/overflow ni čujna posljedica i
ostaju rezidualna monitoring stavka, ne PCM5102A acceptance blocker.

Oba decka zaustavljena su nakon testova. Wi-Fi SoftAP ostaje uključen radi rada
bez zaslona.
