# Instructions
- Work is tracked with prep (`.prep/`): `prep next` for what to do, `prep list` for all open issues.
- Read `agent-docs/` before searching code: start at `agent-docs/index.md`, which routes by kind of
  change to the feature doc (where code lives, how it works, a Change Guide). `agent-docs/features-log.md`
  is history only, not current behaviour.
- Before changing code, run `prep knowledge find <area>` (e.g. `curve`, `slide`, `ui`, `serialization`)
  for the decisions and findings that apply to it.
- The UI design sandbox is `tools/ui-sandbox/` (Svelte; the user runs the dev server). Its
  iteration process lives in `tools/ui-sandbox/iteration/` and is started with `/iteration`.
- Everyday commands are in the `justfile` (`just` lists them): `just test [name]`, `just sim`,
  `just firmware`, `just sandbox`.
- Docs stay current with the code: a change that alters behaviour, file locations or a stored format
  updates the matching doc in `agent-docs/` in the same change, and `just docs-check` must pass. Feature
  and code documentation lives in `agent-docs/`, never in prep knowledge (which holds only decisions
  and findings).

# Simulator Test Harness

Engine and UI behaviour is tested headlessly through the Python bindings (`just test [name]` runs
`src/apps/sequencer/tests/ui/*_test.py`). Setup, the API, the test list and the pitfalls are in
`agent-docs/testing.md`. Step notes are scale degrees, not semitones, and the default scale is Ionian
(`prep knowledge find scale`).
