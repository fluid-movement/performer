---
type: finding
title: pybind ignores defaulted routed arguments on setters
description: Binding a Python property directly to a setter with a defaulted routed argument makes assignment raise TypeError; wrap the setter in a lambda in python/project.cpp.
date: "2026-10-23"
scope:
  - src/apps/sequencer/python/**
---


