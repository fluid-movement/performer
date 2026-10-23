---
type: decision
title: CONFIG_VERSION_NAME is not the fork name
description: The bootloader and update path match against the VersionTag, so CONFIG_VERSION_NAME stays as is; the fork's display name is the display-only CONFIG_EDITION_NAME.
date: "2026-10-23"
scope:
  - src/apps/sequencer/Config.h
  - src/apps/sequencer/ui/pages/StartupPage.*
---


