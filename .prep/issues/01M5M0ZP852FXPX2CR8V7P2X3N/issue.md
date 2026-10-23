---
title: 'MidiCv track: redesigned Track Edit page'
kind: code
tags:
  - ui
  - midicv
priority: high
---

Last track type in the G7 UI redesign (Note, Curve, Stochastic, Arp, Quantizer are done). MidiCv has no step grid, so its Track Edit page is a live MIDI activity monitor: up to 8 voice rows (note name + velocity bar), last incoming pitch, pitch bend bar, mod wheel bar, port/channel. Proposed F-key tabs: PORT (source port + channel), VOIC (voices, voice config, note priority), ARP, TUNE (transpose, pitch bend range, mod range), F4 action TBD. A V1 draft exists in the UI sandbox (tools/ui-sandbox, src/routes/+page.svelte) but needs iteration before porting to C++. Voice slots appearing/disappearing is purposeful animation. Follow the UI design-language decisions (prep knowledge find ui).

Source: CONTEXT.md Active TODOs (G7), agent-docs/ui-redesign/06-track-type-inventory.md section 6.

## Open questions
