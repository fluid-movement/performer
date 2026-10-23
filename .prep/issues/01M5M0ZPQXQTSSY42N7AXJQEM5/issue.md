---
title: 'Bug: OverviewPage curve strip uses the legacy Curve::function table'
kind: code
tags:
  - bug
  - curve
  - ui
---

OverviewPage.cpp:168 renders the curve overview strip via Curve::function(Curve::Type(step.shape())), reading the V1 shape percent as an old curve-type enum index, so the overview does not show the actual segment. Render with CurveSequence::evalSegment (and the track's shapeCurveExponent) like CurveSequenceEditPage.

Source: agent-docs/features-log.md (Version50 entry, known pre-existing issue).

## Open questions
