# Shift + Browse/Load hardware acceptance

Date: 2026-09-01.

## Scope

Focused eyes-on and API verification of the FLX4 browser shifted namespace on
the accepted DSI/touch bench image `M3-47-g3f23bd2-dirty`, factory SHA-256
`AEDF6D680BB207F57E31147024F5261D364A2BB8AE0BF467E71F233F51E5CF26`.
Both decks were stopped and Wi-Fi remained enabled.

## Results

1. Holding Shift and pressing Browse opened the Library tab without loading or
   starting a track.
2. One unshifted Browse detent moved the visible selection by one row.
3. One shifted Browse detent produced the expected accelerated visible jump;
   production code scales Library movement by ten rows per input detent.
4. Shift + Load 1 loaded the selected 44.1-kHz `Latin Lover - Casanova Action`
   track only into Deck 1. Deck 2 remained on `The Traveller.mp3`.
5. After one further Browse detent, Shift + Load 2 loaded the selected 44.1-kHz
   `Kim Wilde - You Keep Me Hangin' On` track only into Deck 2. Deck 1 remained
   on `Latin Lover - Casanova Action`.

After both loads, both decks reported `READY` and `playing=false`. The cumulative
output-late count remained 1, PCM underrun counts remained 0/0, UAC dropped
blocks and overflow frames remained 0/0, and `data_loss=false`.

## Decision

Accept Shift + Browse Library force-open and accelerated navigation, plus
shifted Load routing for both decks. This closes the focused Browse/Load UI
gate. It does not replace the remaining screensaver, corner/multitouch,
repeated-edge or long combined integration gates.
