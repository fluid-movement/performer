---
type: decision
title: Curve track has no play mode, fill mode, min/max or probability biases
description: 'Removed as dead code: curveMin/curveMax and shape/gate probability biases with routing targets CurveMin (39), CurveMax (40), ShapeProbabilityBias (22) in Version52; playMode and fillMode in Version53 (engine always uses advanceFree; _fillSequence was never read). GateProbabilityBias (9) stays for Note/Arp/Stochastic.'
date: "2026-10-23"
scope:
  - src/apps/sequencer/model/CurveTrack.*
  - src/apps/sequencer/model/Routing.*
---


