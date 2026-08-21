# Development Plan

Status: plan nakon single-chip čišćenja, 2026-08-22.

## Trenutna baza

- jedan ESP32-P4 firmware target;
- izravni FLX4 USB MIDI In/Out i UAC1 headphone put;
- zaseban USB3 MSC put za Rekordbox medij;
- dual-deck audio, DSP, UI, Wi-Fi remote i potpisani P4 OTA;
- uklonjeni pomoćni kontrolni firmware, UART/bulk protocol, `.s3bin` profili,
  peer debug/OTA i međupanački PCM link.

## Sljedeće faze

### 1. Učvrstiti izravni FLX4 host

- dodati host testove za `p4_flx4_map`, MIDI generation gate, LED encoder, UAC
  packetizer i audio ring;
- potvrditi sve interface/endpoint/alternate-setting izbore na stvarnom FLX4;
- provjeriti reconnect tijekom playbacka i puni LED resync.

Acceptance: bez stale događaja, LED mismatcha ili kontinuiranih UAC dropova.

### 2. Dual-USB stress

- istodobno streamati dva decka s USB3 dok FLX4 MIDI/UAC radi na USB2;
- mjeriti output deadline, cache miss, USB recovery i headphone drop brojače;
- ponoviti connect/disconnect i zamjenu medija tijekom sigurnih transport stateova.

Acceptance: nema audio artefakata, deadlocka ni reset loopa u dugom soaku.

### 3. MIDI/LED feature parity

- proći preostale redove u `DDJ_FLX4_MIDI_MAP.md` izravno iz XML reference;
- za svaku kontrolu dodati input behavior i LED reconnect test;
- ukloniti zastarjele numeričke semantičke ID-jeve tek nakon pokrivanja.

Acceptance: svi podržani FLX4 elementi imaju jednoznačan P4 state owner.

### 4. Audio acceptance

- hardware-verify cue/master routing, headphone level/mix i PCM5102A headroom;
- dovršiti scratch/Master Tempo rubne slučajeve uz loop i pitch promjene;
- postaviti pragove za UAC ring i output timing alarme.

### 5. Release hardening

- P4-only reproducibilni clean build i OTA package gate;
- UI screenshot baseline nakon vizualne provjere Settings promjene;
- ažurirani startup smoke, risk register i release validation zapis.
