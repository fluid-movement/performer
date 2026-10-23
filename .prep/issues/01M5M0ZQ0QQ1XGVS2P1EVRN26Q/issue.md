---
title: 'Config pages: long-press to reset a parameter to default'
kind: code
tags:
  - ui
  - config
priority: low
---

Metropolix-style long-press-to-reset-default is not possible: there is no long-press primitive. Only SystemPage hand-rolls one by polling ticks in draw(). Add a reusable long-press primitive (key event layer or Page helper) and use it on ListPage to reset the selected row to its default.

Source: CONTEXT.md Active TODOs.

## Open questions
