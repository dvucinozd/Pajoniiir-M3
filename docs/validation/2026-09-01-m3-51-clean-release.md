# M3-51 clean release acceptance — 2026-09-01

## Artifact identity

- source commit: `afb20993381ad5e9a245d83de79692a04e7a9db9`;
- toolchain: ESP-IDF `v6.0.2`;
- clean build directory: `firmware/main-deck-p4/build_release`;
- firmware version: `M3-51-gafb2099`;
- app image: 2,371,008 bytes;
- app SHA-256:
  `7FB9C78E4918117E60848B1BF4D34277412B722341DACD41DEAEAF2E94C815B7`;
- smallest app partition: 4 MiB, 43 % free;
- `dependencies.lock`: unchanged.

The full P4 host suite had already passed for the source commit before the
release build. The clean ESP-IDF build completed successfully from an empty
ignored directory; tracked worktree state remained clean.

## Signed bundle

`tools/package_ota_release.ps1 -BuildName build_release` produced and locally
verified an ECDSA-P256-SHA256 bundle using key ID `rel-001`:

- bundle: `main-deck-p4.ddjota`;
- bundle size: 2,371,196 bytes;
- bundle SHA-256:
  `C2658E3C55647745DB8142D691E8A9EBF5A27FA584DED9469B6AA36335BEEBC7`;
- manifest SHA-256:
  `50E4B3BDEDB0E1DD85F6DFA18C14A56AC2E752E0C648F559EDA566E68E7B6262`;
- signature SHA-256:
  `CDC980081D531A12139063DC2D10DBA20C803D0F083D41E64C5AAEBBF7E5F6AD`.

## Installation and boot evidence

The clean image was first fully flashed through COM6. Flash write verification
passed and the board booted `factory / M3-51-gafb2099`. The product SoftAP
remained enabled and the same signed bundle was uploaded to
`POST /api/ota/p4`. The endpoint returned HTTP 200 with
`{"ok":true,"rebooting":true}`.

Serial evidence showed manifest verification, a 2,371,008-byte write to
`ota_0`, image verification and the next-boot selection. The board then booted
`ota_0 / M3-51-gafb2099` as `pending_verify`; startup health passed and marked
the image valid. `/api/firmware` subsequently reported `idle` with no error.

USB3 restored the 191-track library. The known initial simultaneous FLX4/MSC
enumeration collision occurred and recovered automatically by reclaiming the
FLX4 at address 3. MIDI In, MIDI Out and UAC audio were all active afterward.
Three stable API snapshots reported zero PCM underruns, zero output-late events
and zero service-log drops.

## Physical release smoke

The operator confirmed on the installed `ota_0` release:

- normal GUI rendering;
- responsive touch and Backlight control;
- both decks playing;
- clean PCM5102A master and FLX4 headphone output;
- fluid main waveforms at 4, 8, 12, 16 and 24 visible beats.

Result: **M3-51 clean build, signed local OTA and physical release smoke PASS**.

The product Wi-Fi remains enabled for continued track loading and development.
The automatically recovered initial USB enumeration collision and previously
isolated output-late events remain monitoring items, not failures of this gate.
