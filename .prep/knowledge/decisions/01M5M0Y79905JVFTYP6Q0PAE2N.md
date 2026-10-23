---
type: decision
title: Config sections are opt-in per row and no section scrolls
description: Sections are three defaulted ListModel virtuals (sections, sectionName, rowSection), keyed per row so enum Item never needs reordering. Defined on the 6 track models plus Project and Clock (the pages with free F-keys). Largest section is 6 rows and Font::Tiny 7px rows show 6, so nothing scrolls, which is what makes row positions learnable. Keep new sections at 6 rows or fewer.
date: "2026-10-23"
scope:
  - src/apps/sequencer/ui/pages/ListPage.*
  - src/apps/sequencer/ui/model/*ListModel.h
---


