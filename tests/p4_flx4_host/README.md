# P4 FLX4 host tests

PC regression coverage for the pure parts of the direct ESP32-P4 to DDJ-FLX4
path:

- USB-MIDI packet parsing and semantic mapping;
- MIDI output generation admission across connect/disconnect generations;
- LED USB-MIDI packet encoding;
- UAC1 44.1 kHz packet pacing, stateful 48→44.1 kHz block conversion and the
  four-channel audio ring.

The suite is part of `tests/run_p4_host_tests.ps1`. USB descriptor selection,
interface claiming and real transfer completion remain hardware acceptance work.
