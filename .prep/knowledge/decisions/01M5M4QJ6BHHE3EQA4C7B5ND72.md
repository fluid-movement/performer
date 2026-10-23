---
type: decision
title: 'UI design docs: rules stay in agent-docs/ui-redesign, sandbox docs live with the sandbox'
description: Design language, IA and design ideas stay in agent-docs/ui-redesign/ because firmware agents changing painters need them; the sandbox README and iteration process live in tools/ui-sandbox/; superseded inventories were dropped.
date: "2026-10-23"
issue: 01M5M1Z71R13P7S9TY1SW2QM69
---

00-ia, 01-design-language and 08-design-ideas govern how shipped C++ pages look, so agents editing ui/ read them from agent-docs (indexed in index.md). 02-sandbox-status was sandbox process and a stale file tree: its useful parts became tools/ui-sandbox/README.md next to iteration/. 05-current-ui-functions duplicated ui-state.md for the old UI and 06-track-type-inventory was superseded by the track feature docs, except the MidiCv section, kept as ui-redesign/midicv-page-brief.md. Rules themselves are prep decisions; the docs link to them.
