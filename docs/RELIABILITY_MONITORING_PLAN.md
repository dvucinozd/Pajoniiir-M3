# Reliability Monitoring Plan

Status: **deferred** after the accepted `M3-51-gafb2099` release.

## Purpose

This plan preserves the exact procedure for investigating two residual
monitoring findings without treating them as confirmed release failures:

1. the initial FLX4 claim can collide with simultaneous USB mass-storage
   enumeration and then recover automatically on a new address;
2. isolated audio `output-late` events have occurred without audible or visual
   consequences, PCM underruns, UAC data loss, or resets.

No firmware change is justified until the campaign produces a repeatable
failure or a measurable regression. The accepted M3-51 release remains the
baseline.

## Required monitor utility

Before starting the physical campaign, add a host-side
`tools/monitor_m3_reliability.ps1` utility. It must not modify device state.
It should:

- poll `/api/firmware` and `/api/status` at a configurable interval;
- periodically fetch `/api/library` and count tracks;
- save timestamped raw JSON and a compact CSV summary;
- capture the initial and final `/api/diagnostic-log` snapshots;
- calculate counter deltas instead of judging cumulative boot history;
- record HTTP errors, response latency, controller recovery time, library
  recovery time, firmware version, running slot, and unexpected reboot signs;
- stop with a non-zero exit code on a hard failure;
- preserve all output under a timestamped ignored artifact directory.

The summary must include at least:

- `controller.present`, `midi_in`, `midi_out`, and `usb_audio`;
- library track count;
- deck state, source sample rate, pitch, and playback position;
- `output_late_count` and `output_late_max_us`;
- both PCM underrun counters;
- UAC dropped blocks, overflow frames, active underflow delta, ring state, and
  `data_loss`;
- service-log queue depth and dropped-record count;
- free internal heap and PSRAM;
- OTA state, running version, and running slot.

## Campaign A: USB enumeration and recovery

Run the following 30 controlled cycles with the Rekordbox medium containing the
accepted 191-track library:

| Scenario | Cycles | Initial connection state |
| --- | ---: | --- |
| Cold boot, both devices attached | 10 | FLX4 on USB2 and media on USB3 |
| FLX4 unplug/replug | 10 | USB3 remains attached; decks stopped |
| USB3 first, then FLX4 | 5 | connect in the stated order |
| FLX4 first, then USB3 | 5 | connect in the stated order |

For every cycle, record:

- time until the web API responds;
- time until the controller is present with MIDI In, MIDI Out, and UAC active;
- time until the library reports 191 tracks;
- USB address transitions visible in the diagnostic log;
- whether recovery needed another replug, reset, or power cycle;
- LED snapshot correctness after reconnect.

### Campaign A PASS/FAIL

PASS requires automatic recovery in every cycle, with no manual reset or extra
replug. A cycle is flagged for investigation if the FLX4 is not fully ready
within 15 seconds or the accepted library is not restored within 20 seconds.

Hard failure conditions:

- controller state never becomes fully ready;
- stale or incorrect LED state survives recovery;
- library does not return to 191 tracks;
- panic, watchdog, reset loop, deadlock, or required manual intervention;
- new PCM underrun, UAC drop/overflow, or service-log drop caused by recovery.

## Campaign B: worst-case output timing soak

Run for 60 minutes initially. Extend to 120 minutes only if the first hour is
clean and a longer confidence run is useful.

Required workload:

- both decks playing continuously;
- one 44.1-kHz source and one 48-kHz source;
- Master Tempo enabled on both decks;
- opposing pitch values, nominally D1 `+5%` and D2 `-5%`;
- active loops on both decks;
- waveform zoom at 4 or 8 visible beats;
- PCM5102A master and FLX4 headphones active;
- display, touch, USB3 media, and Wi-Fi enabled;
- `/api/status` polled every 250 ms;
- `/api/library` checked every 40 status polls;
- `/api/firmware` checked every 120 status polls.

The operator should periodically confirm that master audio, headphones,
waveforms, touch, and controller response remain normal. Do not perform USB
removal during this timing-only phase.

### Campaign B PASS/FAIL

Hard failure conditions:

- audible click, crackle, interruption, or incorrect playback speed;
- waveform deformation, display flash, or UI stall;
- panic, watchdog, restart, or controller disconnect that does not recover;
- any new PCM underrun;
- any new UAC dropped block or overflow frame;
- active-playback underflow or `data_loss=true` caused by the run;
- any new service-log drop.

An isolated `output-late` increment is recorded but is not automatically a
failure when it has no physical or data-loss consequence. Open an engineering
investigation when either condition occurs:

- `output_late_max_us` exceeds 15,000 us; or
- three or more new output-late events occur within one minute.

These are investigation triggers, not retroactive release-failure thresholds.

## Evidence and completion

Store the monitor command, firmware identity, raw samples, CSV summary,
diagnostic-log snapshots, operator observations, and final counter deltas in a
dated validation record. Do not claim a zero-late result unless the measured
delta is actually zero.

When both campaigns are eventually completed:

1. update `docs/RISK_REGISTER.md` with measured recovery rates and timing;
2. update `docs/DEVELOPMENT_PLAN.md` and `docs/DOCUMENTATION_STATUS.md`;
3. link the dated validation record from this plan;
4. only change firmware if evidence identifies a repeatable defect.
