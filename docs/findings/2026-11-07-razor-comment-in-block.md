# Razor Comment Inside A Control Block

Recorded on 2026-11-07 against parser-content commit `941da4d` (grammar fix on top of `9fdcfd7`).

## Defect

A `@* … *@` comment inside an `@if`, `else`, or `@foreach` block left the parser in a state where
everything after the block became one `ERROR` node from line 1 to the end of the file. The Razor
compiler accepts a comment anywhere, so a Blazor component that parsed before lost every `@code`
member when a developer added such a comment. Found through code-kb on three ceres components
(julie-extractors #29).

`_razor_comment_text` was `repeat1(/.|\n|\r/)`, one token per character. `razor_comment` is both an
extra and a `_node`, and inside a block the GLR parser took a path where the `@` of `@if` became a
`_razor_marker` and the rest never recovered.

## Fix

`razor_comment` is one token: `token(seq("@*", /([^*]|\*+[^*@])*/, /\*+@/))`. It ends at the first
`*@`, so `@* a * b ** c @ d **@` is one comment. A Razor comment has no child nodes any more; the
corpus case `Commented Code Block` expected C# `comment` children and now expects a plain
`razor_comment`. `html_comment` keeps its own `_html_comment_text` rule, unchanged in behavior.

## Julie Extractors handoff

- Pin the Razor parser to the commit of this fix on `anortham/tree-sitter-razor` `main`.
- Parser ABI: 15, unchanged.
- Generation CLI: `tree-sitter-cli` 0.26.11 (`tree-sitter generate`, then `git diff --stat` of
  `src/grammar.json`, `src/node-types.json`, `src/parser.c`). Generation emits the two existing
  unnecessary-conflict warnings, as before.
- Node dependencies were installed with `npm install --ignore-scripts` on Node 24.18.1; the native
  Node binding does not build on Node 24 (the Node 22 constraint of the previous certification), so
  `npm test` did not run here.

## Verification ledger

| Invariant | Command | Result |
|---|---|---|
| Corpus | `tree-sitter test` | 154/154, with four new cases: a comment first in an `@if` block, after an element in an `else` block, first in a `@foreach` block, and a comment that holds stars and at signs |
| Minimal regression | `tree-sitter parse` on a 10-line component with `@* c *@` inside `@if` followed by `@code { … }` | `compilation_unit` with no `ERROR`; before the fix the root was `ERROR [0, 0] - [10, 0]` |
| Real components | `tree-sitter parse` on ceres `ThemeDesigner.razor` (7b9bad2), `GallerySettingsPanel.razor` and `Pages.razor` (fa389b6) | `compilation_unit` roots; 3, 6, and 0 local `ERROR` nodes, the pre-existing recoveries around `@<span>` templates; before the fix each file was one `ERROR` |
| Rust binding | `cargo test --locked` | see the julie-extractors pin commit |
