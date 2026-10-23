---
type: decision
title: 'Config pages are modeless: the encoder always edits the selected row'
description: 'On every ListPage the encoder changes the selected row''s value and never navigates; there is no edit mode (the old _edit toggle was a mode error waiting to happen). Left/Right move the cursor and their LEDs (Key::Left/Right, indices 28/29) light green when they will move it. F1-F5 jump between sections; pressing the already-active tab walks the cursor down that section and wraps. This also applies to pages with their own footer tabs (Layout, System, User Scale) but not where F-keys are actions (PREV/NEXT/INIT, CANCEL/OK). Exception: single-column models (FileSelect, GeneratorSelect, Utilities) move the cursor because the row is the value; ListModel::columns() is the discriminant. Shipped 2026-08-29, UI only, no version bump.'
date: "2026-10-23"
scope:
  - src/apps/sequencer/ui/pages/ListPage.*
  - src/apps/sequencer/ui/model/*ListModel.h
  - src/apps/sequencer/ui/pages/LayoutPage.*
  - src/apps/sequencer/ui/pages/SystemPage.*
  - src/apps/sequencer/ui/pages/UserScalePage.*
---


