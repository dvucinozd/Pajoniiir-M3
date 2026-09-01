# Master Tempo acoustic timing correction

Date: 2026-08-31 through 2026-09-01. Focused 48/48-kHz and mixed-rate hardware
acceptance are complete; the longer integration soak remains open.

## Defect and correction

During the D2 UI Master Tempo test the operator reported a long delay after
moving the pitch fader. The same evening D1 initially appeared not to accelerate.
Reported source-position advancement was consistent with +5%, but that counter
was not a measurement of actual acoustic tempo.

`audio_keylock_next` advanced its logical source clock correctly but derived
each nominal analysis grain from the previous correlation-selected grain.
Search offsets accumulated separately from that clock. For small tempo changes,
the correlation search could keep selecting approximately normal-speed audio.

Nominal analysis positions now follow the integrated logical source clock,
including its fractional remainder. Correlation adjusts only the current grain;
it cannot move the next nominal position. Rebasing still keeps float coordinates
small; no allocation, new queue, DSP reset on fader movement, or double-precision
operation was introduced in the audio path.

## Reproduction and regression

An independent 440 Hz carrier with 40 ms percussion envelopes every 500 ms
exposed the defect. After a change from 0% to +5%, the old renderer produced
500.391 ms beat intervals instead of 476.190 ms; at -5%, 499.609 ms instead of
526.316 ms. Logical and analysis-grain positions diverged by about 406 ms after
eight seconds at the changed tempo. This is not a measured MIDI-input latency.

After correction the same probe produced 476.193 ms / 526.243 ms respectively.
The permanent `tests/audio_keylock/test_audio_keylock_tempo.c` regression uses:

- production bounded PCM timeline with evictable history, no fallback allowed;
- source/output rates 48/48, 44.1/48 and 48/44.1 kHz;
- factors 1.0, 1.05, 0.95, 1.005, 0.8 and 1.2;
- mid-hop tempo changes after two seconds and return to 0% after six seconds;
- output PCM onset timing against an independently integrated tempo schedule,
  not against the renderer's consumed count or internal grain coordinates.

All 18 cases pass with the exact expected onset counts and worst onset error
2.625 ms (test bound: 15 ms). The test built against the original Git HEAD
fails at +5% because the read head leaves the retained timeline. Existing
constant-tone/consumption tests alone did not catch this defect.

The existing 300-second virtual dual-deck soak passes: zero consumption drift,
detected clicks or clipping, mixed peak 18749. Tonal frequency errors are 1.348%
and 0.301%, within its existing 3% gate; this is not a claim of perfect pitch
preservation or hardware real-time deadline acceptance.

## Hardware retest / stop gates

Full `tests/run_p4_host_tests.ps1` and ESP-IDF v6.0.2 build exited 0.
App-only `M3-47-g3f23bd2-dirty` (2369712 bytes) was written on COM6 to
`factory` / `0x20000`; esptool verified the written hash. Binary SHA-256:
`ED3DAB491476CAB7E6B963EDED156AC0906269FD3295EAFE003BD2D80CA803BC`.
Bootloader, partition table, NVS, otadata and ota_0 were not written.

After reconnecting Windows to the existing `Pajoniiir-M3` Wi-Fi profile,
`/api/firmware` confirmed the new version and factory slot. API confirmed
FLX4 MIDI In/Out/UAC, 191 library tracks, both decks stopped and initial
output-late/PCM underrun counters at zero. This is boot/reconnect evidence,
not an active playback or audible response pass. Master volume reset to
16383; operator was warned to move MASTER low before PLAY. D2 channel volume
is limited to 3140 for preparation, with no automatic playback.
Final preparation: D2 `The Traveller.mp3` (key 106, 48000 Hz) is READY and
paused at 45000 ms, runway 94464 frames, pitch 0%. Master subsequently reads
2099 after the operator adjustment; D1 remains unloaded/stopped. MT must be
visually enabled through the touchscreen before the listening retest.

- Keep Wi-Fi on; preserve signed M3-41 / ota_0 rollback baseline.
- Check actual new running version, restored FLX4 MIDI/UAC and USB3 library.
- On one playing deck with MT visibly enabled, move 0% -> +5% -> -5% -> 0%.
  Check GUI response separately from audible rhythm response and vocal quality.
- Repeat on the other deck, then combine both decks and mixed source rates.
- Compare active-window output-late, PCM underrun and UAC loss deltas; idle
  UAC zero-fill must not be interpreted as playback loss.
- Do not close MT acceptance from an API BPM/position counter alone.

An earlier FLX4 all-LED-off disconnect recovered without restarting P4; its
cause remains unknown. One pre-fix output-late event (11997 us vs 10668 us)
was recorded after recovery. Neither incident is closed by this DSP fix.

## First post-fix listening result

Operator confirmed `reagira. odlicno!` after the D2 MT-on positive-pitch
instruction. Subsequent API sample showed `The Traveller.mp3` playing at
83691 ms, pitch +5.51% (raw 3675), FLX4 MIDI In/Out/UAC connected. Output-late
and both PCM underruns were 0; UAC dropped blocks/overflow were 0 and ring
state nominal (1301/2048). UAC underflow total 19866344 includes idle history
and is not itself evidence of active audio loss.

This accepts the operator-observed positive-pitch response on D2, not a
quantified end-to-end latency or the whole MT block. Negative pitch, return
to zero, D1, tonal quality and combined-load/mixed-rate gates remain open.

## Negative-pitch listening failure: still open

Operator subsequently reported crackling when pitch goes negative. Do not
extend the positive-pitch response observation to negative-pitch audio quality.
First API inspection after that report already showed pitch back at 0% and D2
playing at 53701 ms. Totals were output-late=18 (max 12887 us, budget 10668 us),
D2 PCM underrun=23249, UAC drop/overflow=0 and underflow=22364798. The track had
also restarted since the earlier sample; these cumulative values cannot all be
assigned to the fader movement or to steady negative tempo.

Journal boot 258 records output-late, PCM underrun, active UAC_DATA_LOSS and a
low-ring event. A subsequent 24-sample active window at 22:48:23..22:48:36,
positions 86010..99552 ms, pitch 0%, kept late=18, PCM=23249 and
UAC underflow=22364798 unchanged. Forward PCM runway remained about 94k frames.
Asked operator to hold approximately -5% and distinguish continuous crackling
from crackling only during fader movement. No further firmware change or flash
was made during this diagnostic step; Wi-Fi remains on.

## Steady negative tempo: CPU starvation and search-cache candidate

Operator confirmed continuous crackling with a stationary negative fader,
waveform stutters and occasional display flashes. At -4.98%, the initial
sample near EOF had late=87 (max 13524 us), D2 PCM underrun=157512. After
pause/seek to 45 seconds/resume, HTTP became starved; the attempted HTTP sample
run was aborted on timeouts and is not used as a valid active-window delta.

COM6 serial diagnostics confirmed IDLE0 watchdog warnings. Captured PCs resolve
to `L3_imdct36` in minimp3 (ae_decode) and `audio_pcm_timeline_read` (ae_output).
The forward runway had collapsed to roughly 0.6k-4.4k frames. Output blocks
repeatedly exceeded 10.668 ms; watchdog logging coincided with still larger
stalls (recorded max 147706 us). After playback stopped, API recovered and
reported late=378 and D2 PCM underrun=1118471. No array/heap corruption or USB
audio overflow has been established. The existing LVGL full-redraw fallback
when invalidation areas fill is another possible source of display flashes;
it was not changed or accepted in this step.

The search now memoizes raw source frames within one correlation search only.
Candidate order, interpolation, score and tie-breaking remain unchanged. Each
deck owns a 640-frame cache plus validity bytes, covering the supported maximum
rate ratio of 4. Scratch storage adds 3200 bytes per instance in internal BSS,
not on the 8192-byte output stack, and introduces no render-time allocation.
ELF symbol `s_keylocks`: internal address 0x4ff21340, total size 0x1960.

Fifteen deterministic before/after comparisons (including missing source frames,
mid-hop tempo changes, resets, long source offsets and ratios 0.25..4) produced
identical PCM/consumption/position hashes and failure counts. At ratio 1/-5%,
the maximum callbacks per render call fell from 2355 to 194; overall source
calls fell from 363934 to 207504. These are callback counts, not a hardware
speedup measurement. `test_audio_keylock_search.c` enforces rate-scaled read
budgets and checks sentinel words around the state on every render call.

Full host suite, final targeted sentinel test, 300-second dual-deck PC soak and
ESP-IDF v6.0.2 build passed. UBSan/bounds instrumentation could not link because
this Windows GCC installation lacks libubsan; do not claim sanitizer coverage.
Hardware negative-tempo, waveform and display-flash retest remains open.

New cache candidate keeps the same uncommitted version string
`M3-47-g3f23bd2-dirty`; distinguish it by 2369968-byte size and SHA-256
`5DC4AFF4489476A9AA25C8BDB802AEE47382BE9F42BD6D1ED4B5A596E9023409`, not
the earlier timing-only candidate's ED3DAB49... hash.

Cache candidate was installed app-only on COM6 at 0x20000 with esptool hash
verification; bootloader/NVS/otadata/ota_0 were not written. After reconnect,
API confirmed factory/newly written version, FLX4 MIDI/UAC and 191 tracks,
with initial late/PCM counters zero. D2 was prepared paused at 45 seconds,
channel level 2000, with no automatic playback. The operator must set MASTER
low after the reset and visibly re-enable MT before the next -5% test.

## Cache candidate: steady D2 negative pitch listening pass

Operator confirmed: no crackling and fluid waveform after the steady -5%
instruction. Six API samples at 23:02:11.295..23:02:14.259 confirmed D2 playing
at -5.01%, source position 103260..106147 ms, with FLX4 present. PCM underruns
remained zero on both decks, UAC drop/overflow remained zero, UAC underflow
stayed at 3215871 and ring state was nominal. Output-late stayed at 2: no new
late events in this sample window, but this is NOT a zero-late boot/soak pass.

Accept the focused stationary negative-pitch listening/waveform result on D2.
Positive/negative transitions on the cache candidate, D1, combined-load and
mixed-rate acceptance remain open. Continue observing the earlier display
flash report and the two accumulated output-late events.
Journal boot 259 places both late events (max 11514 us vs 10668 us budget)
at ms=95879, alongside a UAC_DATA_LOSS delta of 99 underflow frames. A low
ring event was also recorded at ms=75878. These pre-window events remain for
monitoring; neither a clean full-session UAC pass nor a soak pass is claimed.

## D2 pitch transitions accepted; D1 prepared

Operator confirmed clean sound and fluid waveform through the requested
-5% -> 0% -> +5% -> 0% sequence with MT enabled. Subsequent API showed D2
back at 0%, already stopped at EOF (position 237408 ms; metadata duration
234000 ms). Late count remained 2, PCM underruns remained 0 on both decks,
and UAC drop/overflow remained 0. UAC underflow had increased during the
intervening idle interval; do not infer an active underflow delta from it.

D1 prepared with key 1 `Film, Jura Stublic - Srce Na Cesti.mp3`, paused at
45 seconds, channel volume limited to 2000. D2 stays stopped, Wi-Fi stays on.
Next: visually enable D1 MT and test 0% -> +5% -> -5% -> 0% independently,
then the combined-load/mixed-rate gate. No firmware change or flash in this step.

## D1 pitch transitions accepted; simultaneous playback prepared

Operator confirmed clean sound and fluid waveform on D1 through
0% -> +5% -> -5% -> 0% with MT enabled. API afterwards showed D1 playing
at 0%, position 91413 ms, and later 123499 ms. Late count stayed 2, both
PCM underrun counts stayed 0; UAC underflow was unchanged at 12428141 across
those two active samples, with drop/overflow 0 and nominal ring in the first.
This closes the focused solo pitch-transition listening checks on both decks,
not the combined-load gate.

Both decks were then paused and prebuffered at 45000 ms, runway 94464 each.
The existing web pitch control set D1 +5% (raw 4096), D2 -5% (raw 12288).
Both sources are 48000 Hz: this first combined test is NOT mixed-rate.
No automatic playback; user must verify both MT buttons are blue and start
both decks. Wi-Fi stays on; initial combined-test late baseline=2, PCM=0/0.

## First simultaneous-playback failure and PCM ownership fix

The operator reported severe stutter and display flashing with D1 at +5% and
D2 at -5%, both MT-enabled. Playback was stopped immediately. Relative to the
late=2, PCM=0/0 baseline, totals rose to output-late=103 (max 143949 us versus
10668 us budget), PCM underrun D1/D2=452618/358307, UAC dropped blocks=21 and
overflow=2347 frames. Journal outliers included 143389 us in the mixer plus
99902 us and 103270 us main/mix stalls. This rejects that firmware candidate
for simultaneous playback. The display flash is correlated with starvation,
but no heap corruption or display-memory overflow has been established.

A deterministic interleaving test then reproduced a separate PCM timeline
publication race. The decode producer advances `write_index` after copying a
frame and only then release-publishes `write_seq`. If preempted between those
operations, `audio_pcm_timeline_set_playhead()` formerly derived a retained
frame's physical slot from a half-published producer index and could select the
wrong PCM. Random key-lock reads and playhead moves now derive physical slots
from the output task's consumer-owned `play_seq/play_index` pair. Range checks
still use the producer's published low cursors, reject 2^32 aliases and discard
a frame that is evicted while copied. Same-epoch cursor movement now uses one
release store; the seqlock remains only for rare 32-bit epoch transitions.

`test_seek_ignores_unpublished_producer_index` fails on the previous mapping
and passes on this fix. `audio_pcm_timeline` now reports 313 assertions; the
search-budget and 18-case acoustic tempo tests pass. The full P4 host suite and
ESP-IDF v6.0.2 build pass. The standalone 300-second dual-deck PC soak consumed
the exact expected source-frame counts with zero drift, clicks or clipping and
a mixed peak of 18749. This is PC evidence only; the short, bounded hardware
simultaneous-playback retest remains open.

The new uninstalled app-only candidate still identifies as
`M3-47-g3f23bd2-dirty`; distinguish it by 2369968-byte size and SHA-256
`DB55CB6C8EE317F56DEF4E611414DEF2583C9F4E6AC92D010C7532575B2774D9`.

The candidate was then installed app-only on COM6 at `0x20000`. A separate
esptool readback verified all 2369968 bytes by digest before hard reset;
bootloader, partition table, NVS, otadata and `ota_0` were not written. The API
confirmed `factory / M3-47-g3f23bd2-dirty`, FLX4 MIDI In/Out/UAC, 191 library
tracks, initial output-late=0 and PCM D1/D2=0/0. D1 key 1 and D2 key 106 are
paused and prebuffered at 45 seconds with 94464-frame runways, channel levels
1500, D1 +5% and D2 -5%. Both are 48 kHz. No playback was started; the operator
must visibly enable MT on both decks before the bounded retest.

## Second simultaneous failure and bounded correlation search

The prepared post-ownership-fix 48/48-kHz retest still failed on hardware.
Service journal boot 261 captured repeated mixer stalls between 104 and 144 ms,
including `AUDIO_BLOCK_OUTLIER` at 144199 us, `AUDIO_UNDERRUN a0=111569` and
UAC data loss. The operator again reported waveform stutter, crackling and a
display flash. This rejected the ownership-only candidate and showed that two
exhaustive correlation scans could still exceed one output deadline. The flash
remains correlated with CPU starvation; it is not evidence of heap or display
memory overflow.

The correlation stage now uses bounded hierarchical scans over the full search
radius followed by local refinement. Stereo SAD uses native 32-bit arithmetic
and four evenly distributed reference samples instead of a 64-bit squared-error
scan. Permanent budget tests cover source/output ratios 0.25, 0.91875, 1.0,
1.088435 and 4.0. At ratio 1.0 the maximum observed work is 22 candidates and
83 source reads; at ratio 4.0 it is 30 candidates and 133 reads. The 18-case
acoustic-tempo gate, full host suite, 313-assert PCM timeline gate and ESP-IDF
v6.0.2 build pass. The 300-second dual-deck PC soak has zero drift, clicks and
clipping, mixed peak 18749 and 1.161 s host CPU time.

The resulting app-only image still identifies as `M3-47-g3f23bd2-dirty` and
is 2370224 bytes with SHA-256
`AEDF6D680BB207F57E31147024F5261D364A2BB8AE0BF467E71F233F51E5CF26`.
It was written only to `factory` at `0x20000` on COM6. Full 2370224-byte
readback matched the digest; bootloader, partition table, NVS, otadata and
signed `ota_0` rollback baseline were not changed. Boot smoke confirmed the
factory image, FLX4 MIDI In/Out/UAC, 191 tracks, output-late=0 and PCM D1/D2=0/0.

## Focused 48/48-kHz dual-deck acceptance

On 2026-09-01 both 48-kHz decks ran simultaneously with MT enabled, D1 at +5 %
and D2 at -5 %, channel levels 1500 and Wi-Fi left enabled. The approximately
12-second sample window stayed API-responsive; both positions advanced, PCM
underrun deltas remained 0/0, UAC dropped-block and overflow deltas remained 0,
`data_loss` stayed false and the UAC ring remained nominal at approximately
1100-1337 frames. After stop, one output-late event was present with maximum
11024 us against a 10668-us warning threshold. It had no PCM/UAC consequence.
The operator confirmed clean sound, fluid waveforms and a stable display.

This closes the focused solo-transition and short 48/48-kHz simultaneous MT
gate for this image. It does not replace the longer integration soak.

## Focused 44.1/48-kHz mixed-rate acceptance

D1 loaded key 6, `Grant Miller - I'm Alive Tonight`, at 44.1 kHz and +5 %.
D2 loaded key 106, `The Traveller.mp3`, at 48 kHz and -5 %. Both were paused
and prebuffered at 45000 ms with channel levels 1500; the operator visibly
confirmed both MT buttons before playback. Wi-Fi remained enabled.

Across ten responsive status samples both decks stayed active and advanced.
The controlled run added no output-late events: the cumulative count remained
1 and the existing maximum remained 11024 us. PCM underrun counts remained
0/0, UAC dropped blocks and overflow frames remained 0/0, and `data_loss`
remained false. Both decks were then stopped automatically. The operator
confirmed that sound, waveforms and display remained correct throughout.

This closes the focused 44.1/48-kHz mixed-rate MT gate for the current app-only
image. A longer combined display/touch, master/headphones, dual-deck and Wi-Fi
integration soak remains a separate acceptance item.
