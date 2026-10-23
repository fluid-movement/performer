# MidiCv Track Edit Page: Brief

The one Track Edit page that has not been built. Note, Curve, Stochastic, Arp and Quantizer have
shipped Steps pages (see their feature docs); the MidiCv track still only has a config list page.
This brief is the design input for building it. How the track works:
[../features/midicv-track.md](../features/midicv-track.md). Visual rules:
[01-design-language.md](01-design-language.md). A V1 draft exists in the sandbox
(`tools/ui-sandbox/src/routes/+page.svelte`, variation list for the MidiCv page) and needs iteration.

## What the page is

MidiCv has no sequence or step parameters. Its Track Edit page is a **real-time MIDI activity
monitor**: it shows what is coming in and how it is being converted. There is no step grid; the
voice display replaces it.

## Real-time display

| Element | Description |
|---|---|
| Active voices | up to 8 voice slots: note name plus a velocity bar, or off |
| Incoming pitch | last received note name |
| Pitch bend | bend as a bar |
| Mod wheel | CC01 value as a bar |
| Port / channel | current MIDI input configuration |

## Configuration tabs

With no editing loop, the function keys surface the most-changed config values:

| Tab | Label | Content |
|---|---|---|
| F1 | PORT | MIDI source port and channel |
| F2 | VOIC | voices, voice config, note priority |
| F3 | ARP | arp mode / hold / octaves / gate length / divisor |
| F4 | TUNE | transpose, pitch bend range, modulation range |
| F5 | (action, undecided) | |

## Design notes

- Each voice is a row: up to 8 rows of about 7 px fit the 55 px content area.
- Encoder plus a held button edits config; idle encoder scrolls voice rows or tabs.
- The monitor is live (voice slots appear and disappear as notes arrive). That is purposeful
  animation under the animation rule (`prep knowledge find animation`).
- The page uses the whole content area for status rather than step editing.
- Tab and column conventions follow the other Steps pages: see the TUNE tab pattern on the
  Quantizer page (dim label at the top, bipolar bar in the middle, numeric value below).
