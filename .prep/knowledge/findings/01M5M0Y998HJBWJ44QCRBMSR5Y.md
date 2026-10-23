---
type: finding
title: Hardware CV output ceiling is +-5V, not 10V
description: Calibration::CvOutput spans MinVoltage -5 to MaxVoltage 5 and voltsToValue() hard-clamps there (the ideal DAC/opamp stage gives about +5.17V / -5.25V). Do not offer ranges beyond 5V.
date: "2026-10-23"
scope:
  - src/apps/sequencer/model/Calibration.*
  - src/apps/sequencer/engine/CvOutput.*
---


