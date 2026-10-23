# UI Sandbox

A SvelteKit 2 / Svelte 5 app for drafting PER|FORMER screens in the browser before any C++. Its
`Canvas.ts` mirrors `src/core/gfx/Canvas.h` (colours, fonts, Set/Add/Sub blend, the 256x64 mono
framebuffer), so a mockup translates one-to-one into painter code. Decision and reasons:
`prep knowledge find sandbox`.

Run it with `just sandbox` (or `npm run dev` here); type-check with `just sandbox-check`. The user
runs the dev server; agents do not start it.

## Layout

```
src/lib/canvas/Canvas.ts       drawing API (mirrors Canvas.h)
src/lib/canvas/FrameBuffer.ts  256x64 buffer with Set/Add/Sub blending
src/lib/canvas/Display.svelte  renders a draw function at N x scale (pixelated)
src/lib/canvas/LedPanel.svelte, LedState.ts   the 32 bi-colour LEDs beside the buttons
src/lib/canvas/fonts/          tiny5x5 (6 px per character) and ati8x8 (called Font.Normal here,
                               Font::Small in the firmware)
src/routes/+page.svelte        catalog of page variations plus a click dummy with hardware buttons
iteration/                     the iteration process: prompt, design log, tastemakers, feedback ledger
```

## Variations

Every page has a list of variations in `VARIATIONS[pageId]` in `src/routes/+page.svelte`:

```ts
interface Variation {
  label: string;
  draw: (c: Canvas, tab?: number) => void;
  tabs?: string[];          // F1-F5 tab labels, length <= 5
  leds?: () => LedState;    // LED state; defaults to all off
}
```

The first variation is the trunk (labelled TRUNK). A page that has shipped keeps its chosen design
there and in the firmware; the sandbox copy of `CurveSequence::evalSegment` must stay identical to
the C++ (`prep knowledge find evalSegment`).

## Where the docs are

- Design rules: `agent-docs/ui-redesign/01-design-language.md` and the UI decisions in prep knowledge.
- Target page map: `agent-docs/ui-redesign/00-ia.md`; what exists today: `agent-docs/ui-state.md`.
- Iteration process (started with `/iteration`): `iteration/prompt-iteration.md`,
  `iteration/04-design-log.md`, `iteration/07-iteration-feedback.md`.
- The one unbuilt page: `agent-docs/ui-redesign/midicv-page-brief.md`.
