# Razor Comment Inside A Control Block

Recorded on 2026-11-07 against parser-content commits `941da4d` (one-token comment) and the extra-only commit that follows `bd30dc3` (grammar fixes on top of `9fdcfd7`).

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

`razor_comment` is an extra only, no longer a `_node` choice. As a single token in both roles, a
comment above the directives (`@* … *@` then `@page`) counted as the first node, which ended the
directive section: julie's `cross_file` fixture then read `@page`, `@inherits`, and `@inject` as
implicit expressions and reported one parse error. As an extra, the comment is accepted in every
state and still appears in the tree (`(razor_comment)` inside `razor_if`, `element`, or
`compilation_unit`), so the corpus expectations do not change.

## Known gap, unchanged

A block that holds only a comment, or nothing (`@if (x) { }`, `@foreach (…) { @* c *@ }`), is one
`ERROR`, as it was before this fix: `_blended_content` is `repeat1`, and 13 block forms use it.
Tree-sitter refuses a rule that matches the empty string, so the fix is `optional($._blended_content)`
at each use; it is out of this change's scope. `@code { @* c *@ }` and `<div>@* c *@</div>` parse.

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
| Corpus | `tree-sitter test` | 156/156, with six new cases: a comment first in an `@if` block, after an element in an `else` block, first in a `@foreach` block, a comment that holds stars and at signs, a comment before the directives, and a comment first in an element |
| Minimal regression | `tree-sitter parse` on a 10-line component with `@* c *@` inside `@if` followed by `@code { … }` | `compilation_unit` with no `ERROR`; before the fix the root was `ERROR [0, 0] - [10, 0]` |
| Real components | `tree-sitter parse` on ceres `ThemeDesigner.razor` (7b9bad2), `GallerySettingsPanel.razor` and `Pages.razor` (fa389b6) | `compilation_unit` roots; 3, 6, and 0 local `ERROR` nodes, the pre-existing recoveries around `@<span>` templates; before the fix each file was one `ERROR`. After the extra-only change `ThemeDesigner.razor` has 2 `ERROR` nodes (lines 1286-1288, the `@<span>` template) |
| julie fixtures | `tree-sitter parse` on julie's `fixtures/extraction/razor/cross_file/source.razor` and `error_recovery/Panel.razor` | no `ERROR` node; with the comment as a `_node`, `cross_file` had one at its `@using` line, and `Panel.razor` was one `ERROR` on the pinned grammar |
| Rust binding | `cargo test --locked` | 1 passed, 0 failed (the `bindings/rust/lib.rs` doctest loads the language) |
