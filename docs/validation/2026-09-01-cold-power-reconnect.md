# Cold-power and reconnect integration gate — 2026-09-01

## Scope

Extended physical recovery gate for the app-only Pajoniiir-M3 bench candidate
after the accepted ten-minute combined-load soak. The test covered three full
P4 cold-power cycles with the complete peripheral set attached, FLX4 USB2
disconnect/reconnect during playback, USB3 media removal/reinsert in the safe
stopped transport state, a five-minute post-reconnect dual-deck load and a
final single-deck PFL smoke.

The tested image was `M3-48-g435bcfe-dirty` in `factory`, SHA-256
`2CDAB5D7C859F28F26E2BB02CDC5B711DA4A25480EA081B18A2C3EF963DF3455`.
Wi-Fi remained enabled. The signed `M3-41-g133f399` rollback baseline in
`ota_0` was not modified.

## Cold-power cycles

All three cycles removed only USB1/CH340C power; FLX4, USB3 media, PCM5102A
and the DSI display remained attached. Network-offline intervals include the
operator's manual delay and are therefore not reported as firmware boot times;
the service-journal millisecond timestamps are the authoritative boot evidence.

- Cycle 1 restored the SoftAP/API, FLX4 MIDI In/Out/UAC and all 191 tracks.
  A 45-second post-boot window completed 80 status polls without error or any
  PCM, UAC, output-late or service-log delta. The operator confirmed all four
  UI tabs and the Backlight slider.
- Cycle 2 created service-journal boot `283` with reset reason `POWERON`.
  USB3 mounted at 2447 ms, all 191 tracks were loaded at 2537 ms and Wi-Fi
  started at 3369 ms. FLX4 fully enumerated and all initial loss counters were
  zero. The operator confirmed the GUI and touch.
- Cycle 3 created service-journal boot `284`, again with `POWERON`. USB3 mounted
  at 2445 ms, all 191 tracks were loaded at 2537 ms and Wi-Fi started at
  3369 ms. FLX4 fully enumerated and all initial loss counters were zero.

No cold-power cycle produced a panic, watchdog, reset loop, white frame,
horizontal wrap, touch failure, peripheral loss or stale library.

## FLX4 reconnect during playback

D1 continued playing while FLX4 was physically disconnected. Full MIDI In,
MIDI Out and UAC returned in approximately 7.5 seconds. The following
30-second window completed 53 status polls and advanced D1 by 29.866 seconds.

| Metric | Delta/result |
| --- | ---: |
| PCM underrun D1/D2 | 0 / 0 |
| UAC dropped/overflow/active-underflow | 0 / 0 / 0 |
| output-late | 0 |
| service-log dropped | 0 |
| headphone ring | `nominal`, 985–1397 / 2048 frames |

The operator confirmed clean uninterrupted PCM5102A master audio and restored
headphones/controller operation.

## USB3 media reconnect

The acceptance run removed and reinserted USB3 with both decks stopped. The
library transitioned `191 -> 0 -> 191`; restoration took 7.21 seconds. FLX4
MIDI In/Out/UAC remained present. The 20-second post-restore window completed
36 status polls without error, and PCM, UAC, output-late and service-log deltas
were all zero.

An additional above-contract stress attempt removed USB3 during D1 playback.
The firmware followed its documented fail-closed policy: it stopped/unloaded
the deck and rebuilt the library after reinsertion. That forced stop recorded
384 D1 PCM underrun frames and two output-late events, so it is not presented
as a loss-free acceptance run. Continued playback after physical removal of
the source medium is not a product guarantee.

## Post-reconnect combined load

Both looped decks played for 300 seconds with PCM5102A master, FLX4 UAC,
both PFL paths exercised, moving waveforms, touch and sustained Wi-Fi/API
traffic.

| Metric | Result |
| --- | ---: |
| status polls / errors | 525 / 0 |
| playback or loop state violations | 0 |
| library checks / errors | 13 / 0 |
| firmware checks / errors | 4 / 0 |
| PCM underrun delta D1/D2 | 0 / 0 |
| UAC dropped/overflow/active-underflow delta | 0 / 0 / 0 |
| service-log dropped delta | 0 |
| output-late delta / maximum | 3 / 13069 us |
| headphone ring | `nominal`, 897–1380 / 2048 frames |
| minimum total/internal heap free | 24,880,408 / 101,319 bytes |

The three isolated deadline misses had no PCM/UAC consequence. The operator
confirmed clean master and headphone audio, fluid waveforms, stable display
and responsive touch.

After cold boot 3, a final 30-second D1 playback window and a separate
20-second D1 PFL window both completed with zero PCM, UAC, output-late and
service-log deltas. The final PFL ring remained `nominal` at 959–1246 frames,
and the operator confirmed clean headphone audio.

## Verdict

**PASS — prolonged cold-power/reconnect integration gate closed.**

This is not a zero-late claim. The three post-reconnect load deadline misses
remain monitoring evidence without measured or audible data-loss consequence.
The next block is release hardening: reproducible clean build, final artifact
identity and release/startup documentation review.
