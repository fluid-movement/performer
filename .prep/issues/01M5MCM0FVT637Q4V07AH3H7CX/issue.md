---
title: 'Bug: ArpSequence::read leaves old-format bytes unread for pre-Version46 projects'
kind: code
tags:
  - bug
  - arp
  - serialization
---

ArpSequence::read (src/apps/sequencer/model/ArpSequence.cpp:90) returns early for dataVersion < Version46, calling clear() without consuming the old per-step ArpSequence bytes. Every read after it in the project stream is misaligned, so loading a pre-Version46 project with an Arp track corrupts the data that follows it (further sequences, tracks, song, routing). Fix: consume the old layout (reconstruct its byte sizes from the pre-Version46 ArpSequence::write in git history) with reader.skip, or otherwise keep the stream aligned, and add a harness test that loads a pre-Version46 project with an Arp track followed by other data. Note the Version46 break was a deliberate clean break (defaults instead of migration); this is about not corrupting the rest of the file.

Source: documentation pass for 01M5M1Z71R13P7S9TY1SW2QM69 (finding 01M5M4SW25RBH4Z67M2QW16EX1).

## Open questions
