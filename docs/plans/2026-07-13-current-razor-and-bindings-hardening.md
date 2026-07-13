# Current Razor and Bindings Hardening Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use razorback:subagent-driven-development when subagent delegation is available. Fall back to razorback:executing-plans for single-task, tightly-sequential, or no-delegation runs.

**Goal:** Make the fork parse the current stable ASP.NET Core 10 Razor surface and the newest ASP.NET Core 11 preview examples without errors, while making every advertised binding and CI gate trustworthy.

**Architecture:** Deepen the existing Razor-aware markup and external-scanner seam instead of attempting to compose C# and HTML base grammars. Preserve the named syntax-tree contract consumed by Julie Extractors, especially `element`, `razor_block`, `razor_explicit_expression`, `razor_implicit_expression`, and every existing directive node. Regenerate checked-in parser sources with the pinned CLI in every grammar slice so each commit is self-consistent, then prove the final generated output deterministic before validating bindings and release metadata.

**Tech Stack:** Tree-sitter DSL, C external scanner, tree-sitter-c-sharp 0.23.5, tree-sitter-cli 0.26.11, Node/N-API, Rust, Python, Go, Swift, GitHub Actions.

**Architecture Quality:** Affected modules are `grammar.js`, `src/scanner.c`, generated parser sources, bindings, and CI. The caller-facing interface is the emitted syntax tree; tests must assert named trees and error boundaries rather than private scanner state. Risk is high because markup and transition rules sit on nearly every Razor parse path. Keep complexity local in the grammar/scanner seam, reject a second-base-grammar composition, reject downstream error-recovery workarounds, and fail the implementation if preserved node kinds disappear from `src/node-types.json`.

## Global Constraints

- Current stable baseline is ASP.NET Core 10; ASP.NET Core 11 preview examples are forward-looking regression inputs, not a stable-version claim.
- Use the official [.NET 10 Razor syntax reference](https://learn.microsoft.com/en-us/aspnet/core/mvc/views/razor?view=aspnetcore-10.0), [.NET 10 Razor component reference](https://learn.microsoft.com/en-us/aspnet/core/blazor/components/?view=aspnetcore-10.0), and [.NET 11 preview release notes](https://learn.microsoft.com/en-us/aspnet/core/release-notes/aspnetcore-11?view=aspnetcore-11.0). Record the source URL and `dotnet/AspNetCore.Docs` commit beside each copied corpus section.
- Preserve all existing named Razor and `element` nodes used by downstream consumers; additions are allowed, removals or renames require a separate downstream migration plan.
- Keep `tree-sitter-c-sharp` at exactly `0.23.5` until an explicit external-token compatibility test proves another version.
- Generate with `tree-sitter-cli 0.26.11`; do not accept generated output from `0.24.x`.
- Bind the ABI-15 parser with Python Tree-sitter `>=0.25,<0.27` and `github.com/tree-sitter/go-tree-sitter v0.25.0`; Python 0.24 and Go 0.23 expose only language version 14.
- Do not edit Julie Extractors, Miller, or Eros in this repository.
- Do not tag, publish, push, or enable registry publication without explicit approval.
- Preserve `.julieignore`, `.miller/`, `/private/tmp/razor-base`, and the `codex/blazor-review-fixes` worktree.

## Verification Strategy

**Project source of truth:** `package.json`, `Cargo.toml`, `pyproject.toml`, `go.mod`, `Package.swift`, `.github/workflows/ci.yml`, and the checked-in Tree-sitter corpus.

**Worker red/green scope:** Run `npx tree-sitter test --file-name <corpus-file>` before implementation to prove the new corpus is red; after grammar/scanner edits, run `npx tree-sitter generate` before the same focused test. Use the binding's own focused test command for binding slices.

**Worker ceiling:** Grammar workers may regenerate their owned checked-in parser sources and run focused corpus/contract tests; binding workers may run their owned binding suite. Only the lead runs final regeneration drift, all bindings together, and the full branch gate.

**Worker gate invariant:** A grammar gate proves the exact documented valid form has no `ERROR` and the exact invalid/recovery form has the expected error boundary. A binding gate proves that binding loads the language, parses a mixed Razor sample, and loads the highlight query when the binding exposes resources.

**Lead affected-change scope:** `npx tree-sitter test --stat total-only`, `npm test`, `cargo test --locked`, `go test ./...`, `swift test`, and the Python wheel test after coherent batches.

**Branch gate:** Clean `npm ci`; `npx tree-sitter generate`; no generated-source diff; all corpus/highlight tests; all five binding suites; `cargo package --locked --list`; package dry-runs; and workflow syntax/convention checks.

**Replay/metric evidence:** Hard gates are zero failed corpus cases, zero unexpected `ERROR` nodes in valid fixtures, expected bounded recovery in invalid fixtures, and successful binding smoke tests. Parse throughput is report-only unless a new corpus case is slower than the existing outlier by more than 25%, in which case profile before handoff.

**Escalation triggers:** Any C# external-token order change, scanner serialization change, named-node removal, ABI change, or binding API-floor change requires the full branch gate on macOS, Linux, and Windows CI.

**Assigned verification failure:** Workers stop and report when assigned verification fails unless the plan explicitly changes that expectation.

**Verification ledger:** Record invariant, command, scope label, commit SHA, result, and timestamp. Reuse a passing entry only when it was produced at the same HEAD.

## Parallel Execution Contract

| Task | Parallel batch | File ownership | Serialization required | Dependency reason |
|---|---|---|---|---|
| Task 1: Lock the named syntax-tree contract | None - serial | new `test/contracts/named-node-contract.test.mjs` | Yes | Establishes a green downstream compatibility gate before grammar changes. |
| Task 2: Complete the markup and scanner seam | None - serial | `package.json`, `package-lock.json`, `grammar.js`, `src/scanner.c`, generated `src/grammar.json`, `src/node-types.json`, `src/parser.c`, `src/tree_sitter/array.h`, `src/tree_sitter/parser.h`, `test/corpus/html.txt`, `test/corpus/razor_html.txt`, `test/corpus/fullexamples.txt` | Yes | Pins generation inputs, preserves Task 1, and establishes a self-consistent markup parser before transition work. |
| Task 3: Complete Razor transitions and directives | None - serial | `grammar.js`, `src/scanner.c`, generated `src/grammar.json`, `src/node-types.json`, `src/parser.c`, `src/tree_sitter/array.h`, `src/tree_sitter/parser.h`, `test/corpus/directives.txt`, `test/corpus/codeblocks.txt`, `test/corpus/expressions.txt`, `test/corpus/escapes.txt`, `test/corpus/transitions.txt` | Yes | Shares grammar/scanner/generated files with Task 2 and must build on its green parser commit. |
| Task 4: Guard upstream coupling and prove deterministic regeneration | None - serial | new `test/contracts/external-token-order.test.mjs`; verification-only access to generated `src/**` and pinned manifests | Yes | Requires the final Task 3 grammar/scanner/generated state. |
| Task 5: Repair advertised bindings | Batch A | `bindings/**`, `binding.gyp`, `Package.swift`, `go.mod`, `bindings/go/go.mod`, `pyproject.toml`, `setup.py`, binding-only manifest sections | No | Separate binding files can be assigned to distinct workers after Task 4; lead integrates under `parallel-lead-commit`. |
| Task 6: Make CI and package metadata truthful | None - serial | `.github/workflows/**`, `Cargo.toml`, `tree-sitter.json`, `README.md`, non-binding metadata in `package.json`, `pyproject.toml` | Yes | Starts after Task 5 integrates shared manifests, avoiding concurrent edits to `package.json`, `pyproject.toml`, and `Cargo.toml`. |
| Task 7: Run branch certification and record Julie handoff | None - serial | `docs/plans/2026-07-13-current-razor-and-bindings-hardening.md`, `docs/findings/2026-07-13-current-razor-certification.md` | Yes | Requires every preceding task and produces the exact commit for Julie. |

### Task 1: Lock the named syntax-tree contract

**Files:**
- Create: `test/contracts/named-node-contract.test.mjs`

**Interfaces:**
- Consumes: Current named-node output from `src/node-types.json`.
- Produces: A green compatibility gate that every later grammar task must preserve.

**Contract inputs:** Julie consumes `element`, `razor_block`, `razor_explicit_expression`, `razor_implicit_expression`, and the current named directive nodes.

**File ownership:** `test/contracts/named-node-contract.test.mjs`

**Serialization required:** Yes.

**Dependency reason:** Establishes a green downstream compatibility gate before grammar changes.

**Step 1: Write the compatibility test**

Read `src/node-types.json` and assert every downstream named node is present and named:

```javascript
const required = [
  "element",
  "razor_block",
  "razor_explicit_expression",
  "razor_implicit_expression",
  "razor_page_directive",
  "razor_using_directive",
  "razor_namespace_directive",
  "razor_rendermode_directive",
];
for (const kind of required) {
  assert.ok(nodeTypes.some((node) => node.type === kind && node.named), kind);
}
```

**Step 2: Run the test**

Run: `node --test test/contracts/named-node-contract.test.mjs`

Expected: PASS on the current parser.

**Step 3: Verify failure messaging**

Temporarily change one required node name in the local test invocation, confirm the assertion reports that exact missing name, then restore the correct list before continuing. Do not leave a failing test in the task diff.

**Step 4: Run the existing corpus and contract gate**

Run: `node --test test/contracts/named-node-contract.test.mjs && npx tree-sitter test --stat total-only`

Expected: PASS.

**Step 5: Apply commit mode**

Use `serial-worker-commit`; commit the green compatibility gate and record its SHA.

**Acceptance criteria:**
- [x] The required Julie-facing named nodes have an executable compatibility gate.
- [x] The full pre-change corpus remains green.
- [x] Later tasks cannot silently rename or remove the downstream AST contract.

### Task 2: Complete the markup and scanner seam

**Files:**
- Modify: `package.json`
- Modify: `package-lock.json`
- Modify: `grammar.js:499-697`
- Modify: `src/scanner.c:7-129`
- Modify: `src/grammar.json`
- Modify: `src/node-types.json`
- Modify: `src/parser.c`
- Modify: `src/tree_sitter/array.h`
- Modify: `src/tree_sitter/parser.h`
- Modify: `test/corpus/html.txt`
- Modify: `test/corpus/razor_html.txt`
- Modify: `test/corpus/fullexamples.txt`

**Interfaces:**
- Consumes: Task 1 named-node compatibility gate.
- Produces: `element`-compatible doctype, entity, text, attribute, void, raw-text, qualified-tag, and tag-matching behavior.

**Contract inputs:** Set `tree-sitter-cli` to exactly `0.26.11` and `tree-sitter-c-sharp` to exactly `0.23.5` before the first regeneration. Existing C# external tokens must retain their generated names and order; scanner state must remain serializable within `TREE_SITTER_SERIALIZATION_BUFFER_SIZE`.

**File ownership:** `package.json`, `package-lock.json`, `grammar.js`, `src/scanner.c`, generated `src/grammar.json`, `src/node-types.json`, `src/parser.c`, `src/tree_sitter/array.h`, `src/tree_sitter/parser.h`, `test/corpus/html.txt`, `test/corpus/razor_html.txt`, `test/corpus/fullexamples.txt`

**Serialization required:** Yes.

**Dependency reason:** Pins generation inputs, preserves Task 1, and establishes a self-consistent markup parser before transition work.

**Step 1: Write markup and recovery corpus cases**

Add exact valid forms for doctype, entities, qualified components, void elements, raw script/style text, single/unquoted/boolean attributes, and ordinary punctuation/text. Add named .NET 11 preview fixtures for `DisplayName`, `BasePath`, `EnvironmentBoundary`, and MathML. Add invalid cases for `<div></span>` and a missing attribute quote followed by a valid sibling. Record Microsoft source URLs and the `dotnet/AspNetCore.Docs` commit beside copied examples.

**Step 2: Confirm the focused failures**

Run: `npx tree-sitter test --file-name html.txt && npx tree-sitter test --file-name razor_html.txt`

Expected: FAIL only on the new markup/recovery cases.

**Step 3: Implement the markup rules**

First set the two generation dependencies to their exact contract versions and run `npm ci`. Keep the public wrapper stable while introducing private rules equivalent to:

```javascript
_tag_name: (_) => /[a-zA-Z0-9_:-]+(?:\.[a-zA-Z0-9_:-]+)*/,
doctype: (_) => token(seq(/<!DOCTYPE/i, /\s+/, /[^>]+/, ">")),
html_entity: (_) => token(/&(?:[A-Za-z][A-Za-z0-9]+|#[0-9]+|#x[0-9A-Fa-f]+);/),
_unquoted_html_attribute_value: (_) => token(/[^\s"'=<>`@]+/),
```

Add explicit single-quoted, double-quoted, unquoted, and boolean attribute lanes. Add void-element and raw-text-element handling while still emitting `element` for downstream component/markup consumers.

Adapt the proven tag-name stack strategy from the current `tree-sitter-html` external scanner instead of inventing regex equality. Serialize tag state together with the inherited C# interpolation state and reject mismatched/crossing end tags through recovery.

Stop `HTML_ATTRIBUTE_TAIL_TEXT` before newline, `<`, `>`, quote, or `@` without consuming the structural boundary.

Run `npx tree-sitter generate` after the grammar/scanner implementation. Review every generated diff and do not hand-edit `src/grammar.json`, `src/node-types.json`, `src/parser.c`, or generated headers.

**Step 4: Run the focused corpus and compatibility gate**

Run: `npx tree-sitter generate && npx tree-sitter test --file-name html.txt && npx tree-sitter test --file-name razor_html.txt && node --test test/contracts/named-node-contract.test.mjs`

Expected: PASS, including bounded recovery after malformed attributes and errors for mismatched tags.

**Step 5: Apply commit mode**

Add repeated nested tags, attribute edits, raw strings, and interpolated C# strings, then run `npx tree-sitter test --debug-build --file-name razor_html.txt`. Expected: PASS with no scanner assertion, serialization overflow, or inherited C# regression. Use `serial-worker-commit` only after this green gate.

**Acceptance criteria:**
- [x] Doctype, entities, qualified component tags, void tags, raw text, all required attribute forms, and ordinary top-level text parse without `ERROR`.
- [x] Mismatched tags and malformed attributes recover at the nearest structural boundary.
- [x] Existing `element` consumers see a stable named wrapper.
- [x] Scanner serialization covers both C# interpolation and markup tag state.
- [x] Named `DisplayName`, `BasePath`, `EnvironmentBoundary`, and MathML fixtures are committed with source provenance.

### Task 3: Complete Razor transitions and directives

**Files:**
- Modify: `grammar.js:20-464`
- Modify: `src/scanner.c` only if immediate-token or recovery behavior requires it
- Modify: `src/grammar.json`
- Modify: `src/node-types.json`
- Modify: `src/parser.c`
- Modify: `src/tree_sitter/array.h`
- Modify: `src/tree_sitter/parser.h`
- Modify: `test/corpus/directives.txt`
- Modify: `test/corpus/codeblocks.txt`
- Modify: `test/corpus/expressions.txt`
- Modify: `test/corpus/escapes.txt`
- Modify: `test/corpus/transitions.txt`

**Interfaces:**
- Consumes: Task 2 markup boundaries and current C# expression grammar.
- Produces: Correct nested blocks, bare page directives, contiguous transitions, templates, escapes, punctuation, and tag-helper targets.

**Contract inputs:** Whitespace is forbidden between `@` and directive/control/expression tokens; ordinary whitespace remains an extra elsewhere.

**File ownership:** `grammar.js`, `src/scanner.c`, generated `src/grammar.json`, `src/node-types.json`, `src/parser.c`, `src/tree_sitter/array.h`, `src/tree_sitter/parser.h`, `test/corpus/directives.txt`, `test/corpus/codeblocks.txt`, `test/corpus/expressions.txt`, `test/corpus/escapes.txt`, `test/corpus/transitions.txt`

**Serialization required:** Yes.

**Dependency reason:** Shares grammar/scanner/generated files with Task 2 and must build on its green parser commit.

**Step 1: Write Razor transition/directive corpus cases**

Add nested `@{}`, bare `@page`, templated delegates in initializer and argument positions, standalone `@@`, punctuation after explicit expressions, qualified/wildcard tag-helper targets, and invalid `@ page`, `@ if`, and `@ (value)` cases. Add named .NET 11 preview fixtures for `NavLink RelativeToCurrentUri` and asynchronous form-validation markup, with their Microsoft source URL and `dotnet/AspNetCore.Docs` commit.

**Step 2: Confirm transition failures**

Run: `npx tree-sitter test --file-name directives.txt && npx tree-sitter test --file-name codeblocks.txt && npx tree-sitter test --file-name expressions.txt && npx tree-sitter test --file-name escapes.txt && npx tree-sitter test --file-name transitions.txt`

Expected: FAIL for the new behaviors.

**Step 3: Implement the minimal grammar changes**

Make route text optional, admit anonymous blocks in nested blended content, and make the transition token contiguous:

```javascript
_razor_marker: (_) => token("@"),
razor_page_directive: ($) =>
  seq(alias(seq($._razor_marker, token.immediate("page")), "at_page"), optional($.string_literal)),
```

Use immediate tokens for every keyword or delimiter that must touch `@`. Make `razor_escape` independently accept `@@`. Add `razor_template` only at verified C# initializer/argument expression seams, not as a global expression alternative. Accept qualified/wildcard tag-helper type targets while preserving the assembly field.

Run `npx tree-sitter generate` after the grammar/scanner implementation and review the generated diff before testing.

**Step 4: Run positive and negative transition cases**

Run: `npx tree-sitter generate && npx tree-sitter test --include 'nested block|bare page|whitespace after|templated delegate|escape|explicit punctuation|tag helper|relative navigation|async form validation'`

Expected: PASS; invalid spaced transitions contain an `ERROR` and do not become Razor directive/control nodes.

**Step 5: Apply commit mode**

Run `npx tree-sitter generate && npx tree-sitter test --stat total-only && node --test test/contracts/named-node-contract.test.mjs`. Expected: every corpus/highlight/compatibility case passes and a second generation produces no diff. Use `serial-worker-commit` and record the grammar/scanner/generated-parser SHA.

**Acceptance criteria:**
- [x] Nested `@{}` blocks, bare `@page`, general templated delegates, `@@`, explicit-expression punctuation, and qualified tag-helper targets parse correctly.
- [x] Invalid whitespace after `@` is not silently accepted as Razor syntax.
- [x] No broad C# expression conflict is introduced.
- [x] Named `NavLink RelativeToCurrentUri` and asynchronous form-validation fixtures are committed with source provenance.

### Task 4: Guard upstream coupling and prove deterministic regeneration

**Files:**
- Create: `test/contracts/external-token-order.test.mjs`
- Verify: `package.json`, `package-lock.json`, `src/grammar.json`, `src/node-types.json`, `src/parser.c`, `src/tree_sitter/array.h`, `src/tree_sitter/parser.h`

**Interfaces:**
- Consumes: Task 3's final grammar/scanner/generated parser and Task 2's exact dependency versions.
- Produces: A compatibility guard for the complete external-token order plus final deterministic-regeneration proof.

**Contract inputs:** `tree-sitter-cli` exactly `0.26.11`; `tree-sitter-c-sharp` exactly `0.23.5`.

**File ownership:** new `test/contracts/external-token-order.test.mjs`; verification-only access to generated `src/**` and pinned manifests

**Serialization required:** Yes.

**Dependency reason:** Requires the final Task 3 grammar/scanner/generated state.

**Step 1: Write the external-token contract test**

Create a Node test that reads `src/grammar.json`, extracts every external token name in order, and compares the complete array with a literal array copied from the reviewed Task 3 grammar. The expected array must include the inherited C# prefix and every Razor/markup external; do not assert only a suffix and do not derive the expected value from `src/grammar.json` at runtime. Task 1 remains the named-node guard.

**Step 2: Run the contract tests against the final parser**

Run: `node --test test/contracts/external-token-order.test.mjs test/contracts/named-node-contract.test.mjs`

Expected: PASS against Task 3's reviewed generated parser. Any mismatch is a Task 2/3 plan defect; fix the owning grammar slice before continuing.

**Step 3: Reinstall pinned dependencies and regenerate**

Confirm the manifest and lock resolve `tree-sitter-cli 0.26.11` and `tree-sitter-c-sharp 0.23.5`, run `npm ci`, then run `npx tree-sitter generate`. Generated files must remain unchanged.

**Step 4: Prove deterministic regeneration**

Run: `node --test test/contracts/external-token-order.test.mjs && npx tree-sitter generate && git diff --exit-code -- src/grammar.json src/node-types.json src/parser.c src/tree_sitter`

Expected: PASS and no second-generation diff.

**Step 5: Apply commit mode**

Use `serial-worker-commit` and record the external-token contract SHA plus the verified parser ABI.

**Acceptance criteria:**
- [x] Generated sources match grammar and scanner at the tagged commit.
- [x] Regeneration is clean with CLI 0.26.11.
- [x] External-token and named-node contract tests pass.

### Task 5: Repair advertised bindings

**Files:**
- Modify: `package.json`, `package-lock.json`, `bindings/node/binding_test.js`
- Modify: `Package.swift`, `bindings/swift/TreeSitterRazorTests/TreeSitterRazorTests.swift`
- Modify: `go.mod`, `bindings/go/binding_test.go`
- Create: `go.sum`
- Delete: `bindings/go/go.mod`
- Modify: `pyproject.toml`, `bindings/python/tests/test_binding.py`
- Modify: `bindings/rust/lib.rs` tests if a parse smoke is absent
- Modify: `Cargo.toml`

**Interfaces:**
- Consumes: Task 3's ABI-15 generated parser plus Task 4's deterministic-regeneration and external-token contract evidence.
- Produces: Five bindings that load, parse a mixed Razor sample, and expose packaged queries/resources truthfully.

**Contract inputs:** Node must use a Tree-sitter runtime compatible with tree-sitter-c-sharp's `^0.25.0` requirement. Python Tree-sitter must be `>=0.25,<0.27`; test 0.25 as the ABI-15 floor and 0.26 as current. Go must use `github.com/tree-sitter/go-tree-sitter v0.25.0`.

**File ownership:** Binding-specific files listed above; one worker per binding; lead owns shared manifests.

**Serialization required:** No.

**Dependency reason:** Separate binding files can be assigned to distinct workers after Task 4; lead integrates under `parallel-lead-commit`.

**Step 1: Add failing parse smoke tests**

Use the same sample in every binding:

```razor
@page
<EditForm Model="@model"><InputDate @bind-Value="model.Date" /></EditForm>
```

Each test must assert a `compilation_unit` root and no parse errors, not only successful `setLanguage`.

**Step 2: Run each binding before fixes**

Run: `npm test`; `cargo test --locked`; `go test ./...`; `swift test`; build a wheel and run `python -m pytest bindings/python/tests` under Python Tree-sitter 0.25 and 0.26.

Expected: Node dependency/load, Swift link, nested Go module, Go ABI-15 load, and Python 0.22-floor evidence fail as recorded in the review.

**Step 3: Implement binding fixes**

- Node: make the runtime peer compatible and a development dependency so clean `npm test` works.
- Node: make `npm test` run both `bindings/node/*_test.js` and `test/contracts/*.test.mjs`.
- Swift: compile both `src/parser.c` and `src/scanner.c`.
- Go: use the root module, pin official `github.com/tree-sitter/go-tree-sitter v0.25.0`, and remove the duplicate nested module.
- Python: declare `tree-sitter>=0.25,<0.27` and exercise both 0.25 and 0.26 environments.
- Rust: keep scanner compilation and add a real parse smoke if absent.
- Cargo: anchor package includes so ignored `node_modules/**/grammar.js` cannot enter the crate.

**Step 4: Run binding tests independently**

Run the commands from Step 2 in clean dependency environments.

Expected: PASS for all bindings and no missing external-scanner symbols.

**Step 5: Apply commit mode**

Use `parallel-lead-commit`: workers hand off verified diffs; the lead stages shared-manifest changes after conflict review.

**Acceptance criteria:**
- [x] Every advertised binding parses the same mixed Razor sample.
- [x] Clean dependency installation works at every declared minimum.
- [x] No duplicate Go module or omitted Swift scanner remains.
- [x] Package contents exclude ignored dependency sources.

### Task 6: Make CI and package metadata truthful

**Files:**
- Modify: `.github/workflows/ci.yml`
- Modify: `.github/workflows/publish.yml`
- Modify: `.github/workflows/regnerate.yml`
- Modify: `Cargo.toml`
- Modify: `package.json`
- Modify: `pyproject.toml`
- Modify: `tree-sitter.json`
- Modify: `README.md`

**Interfaces:**
- Consumes: Working binding commands and final package identities.
- Produces: CI/release workflows and metadata that match what the fork actually supports.

**Contract inputs:** CI must trigger on every manifest, lockfile, query, binding, grammar, scanner, generated source, and workflow change. Publishing remains disabled unless separately approved.

**File ownership:** Workflow, package-metadata, and README files listed above.

**Serialization required:** Yes.

**Dependency reason:** Starts after Task 5 integrates shared manifests, avoiding concurrent edits to `package.json`, `pyproject.toml`, and `Cargo.toml`.

**Step 1: Add failing workflow/metadata convention checks**

Add a small Node convention test or shell validation that asserts required CI path entries, enabled binding inputs, scanner comparison against the PR base rather than only `HEAD^`, current fork URLs, and both `razor` and `cshtml` file types.

**Step 2: Run the convention check**

Expected: FAIL on current workflow paths, skipped bindings, stale URLs/authors, and missing `cshtml`.

**Step 3: Update workflows and metadata**

Enable Node, Python, Go, Rust, and Swift inputs supported by `tree-sitter/parser-test-action@v2` or add explicit jobs where the reusable action lacks the necessary smoke. Compare scanner changes against `${{ github.event.pull_request.base.sha }}` for PRs and the push before-SHA for pushes. Pin reusable workflow references to immutable release tags or SHAs after checking upstream provenance. Keep npm/crates/PyPI publication commented or otherwise disabled.

Set repository links to `https://github.com/anortham/tree-sitter-razor`, use the real maintainer identity consistently, add `cshtml`, and link README references to the official .NET 10 Razor syntax/component pages plus the clearly labeled .NET 11 preview release notes.

**Step 4: Run workflow/package checks**

Run: convention test, `npm pack --dry-run`, `cargo package --locked --list`, Python wheel build, and a YAML syntax/action-lint check available in the repo environment.

Expected: PASS; no registry upload occurs.

**Step 5: Apply commit mode**

Use `serial-worker-commit`; Task 5's manifest integration must already be committed, and this task records its own workflow/metadata SHA.

**Acceptance criteria:**
- [x] CI exercises every advertised binding and triggers on relevant manifests/queries.
- [x] Scanner fuzzing cannot be skipped by a multi-commit PR.
- [x] Fork identity, authors, file types, and current docs are consistent.
- [x] Publication remains approval-gated.

### Task 7: Run branch certification and record Julie handoff

**Files:**
- Modify: this plan's checkboxes
- Create: `docs/findings/2026-07-13-current-razor-certification.md`

**Interfaces:**
- Consumes: Tasks 1-6 at one clean HEAD.
- Produces: A verified commit SHA that Julie Extractors can pin.

**Contract inputs:** Julie's next plan is `/Users/murphy/source/julie-extractors/docs/plans/2026-07-13-razor-parser-contract-hardening.md`.

**File ownership:** Plan progress and optional certification evidence only.

**Serialization required:** Yes.

**Dependency reason:** Requires every preceding task and produces the exact commit for Julie.

**Step 1: Run a clean install and deterministic generation**

Run: `npm ci && npx tree-sitter generate && git diff --exit-code -- src/grammar.json src/node-types.json src/parser.c src/tree_sitter`

Expected: PASS with no generated drift.

**Step 2: Run the full branch gate**

Run every command in the Branch gate, including clean floor/current binding environments.

Expected: all gates pass at one HEAD.

**Step 3: Verify official examples**

At execution time, record the current `dotnet/AspNetCore.Docs` commit and compare its Razor-fenced examples in `aspnetcore/release-notes/aspnetcore-11/includes/blazor.md` with the named fixtures committed by Tasks 2 and 3: `DisplayName`, `BasePath`, `NavLink RelativeToCurrentUri`, `EnvironmentBoundary`, MathML, and asynchronous form validation. Parse those checked-in fixtures plus current ASP.NET Core 10 App.razor/Razor Pages examples and any newly added preview snippets. If documentation drift requires a new fixture or grammar change, return it to Task 2 or 3 ownership and rerun the affected gates before certification. Assert zero unexpected `ERROR` nodes and keep .NET 11 labeled preview until GA.

**Step 4: Verify repository state**

Run: `git rev-parse --show-toplevel`, `git branch --show-current`, `git rev-parse HEAD`, `git status --short --branch`, and `git worktree list`; inspect every related worktree status.

Expected: only intentional task changes remain; pre-existing untracked/dirty state is preserved and named.

**Step 5: Apply commit mode and hand off**

Use `serial-worker-commit`, record the final commit SHA, parser ABI, CLI version, and gate ledger. Do not push or publish without approval. Supply the exact SHA to the Julie plan; never substitute a branch name or floating dependency.

**Acceptance criteria:**
- [x] All valid official-current examples parse without unexpected errors.
- [x] Every binding and package gate passes at the same final SHA.
- [x] Regeneration is deterministic and the named-node contract is intact.
- [x] Exact Julie pin SHA and verification ledger are recorded.
- [x] No tag, push, package publication, or Eros change occurred without approval.
