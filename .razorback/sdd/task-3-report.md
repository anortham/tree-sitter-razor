# Task 3: Razor transitions and directive hardening

## Result

- Status: DONE
- Worktree: `/Users/murphy/.config/razorback/worktrees/tree-sitter-razor/current-razor-bindings-hardening`
- Branch: `codex/current-razor-bindings-hardening`
- Base: `a9913c19bf8611b82e5ff42030e8a4ab86cd8e07`
- Implementation: `e5e7285`

## Changes

- Razor keyword directives and control blocks now require an immediate transition with no intervening whitespace.
- Explicit expressions, code blocks, templates, escapes, page directives, tag-helper directives, blended blocks, and implicit-expression boundaries cover the requested current Razor forms.
- Template admission is restricted to the proven switch-arm, variable-initializer, and argument seams instead of broadening every C# expression.
- The generated grammar, node types, and parser were regenerated with Tree-sitter CLI 0.26.11 against tree-sitter-c-sharp 0.23.5.

## TDD and verification

- RED: the new directive, code-block, escape, and invalid-transition corpus cases failed before implementation; the upstream .NET 11 fixtures already parsed and were retained as regression coverage.
- GREEN: `npx tree-sitter generate && npx tree-sitter test --stat total-only && node --test test/contracts/named-node-contract.test.mjs` passed with 145/145 corpus cases and 1/1 contract tests.
- Focused directive, code-block, expression, escape, transition, component-member-access, render-mode, mixed-attribute, and parenthesized-inline-HTML cases pass.
- Two consecutive generated-file hashes matched: `1298f9978301789fbf746e74be6787c2d8da7fe864d1285bc1c66476151482a0`.
- Final corpus throughput was 2173 bytes/ms; the named-node contract completed in about 51 ms.

## Contract evidence

- Existing external token positions 1-20 are unchanged; `_razor_marker` and `_razor_implicit_end` were appended as positions 21-22.
- Scanner serialization and deserialization are unchanged.
- Stable public nodes remain `razor_template`, `razor_explicit_expression`, `razor_implicit_expression`, and the existing directive/control nodes.
- Miller post-change traces show `razor_template` only at switch-expression arms, variable declarators, and arguments.
- Parser generation changed external token count from 20 to 22; scanner complexity moved from 51 to 54 decisions and 19 to 21 loops while maximum nesting remained 7.
- Miller refresh completed in 1791 ms scan time and 4822 ms total time.

## Fixture provenance

- Commit: `https://github.com/dotnet/AspNetCore.Docs/commit/71011a30140248dffcc6a757d435670365f523d2`
- Relative navigation: `https://github.com/dotnet/AspNetCore.Docs/blob/71011a30140248dffcc6a757d435670365f523d2/aspnetcore/release-notes/aspnetcore-11/includes/blazor.md#navigateto-and-navlink-support-for-relative-navigation`
- Async form validation: `https://github.com/dotnet/AspNetCore.Docs/blob/71011a30140248dffcc6a757d435670365f523d2/aspnetcore/release-notes/aspnetcore-11/includes/blazor.md#asynchronous-form-validation`

## Concerns

- Generation reports two non-blocking unnecessary-conflict warnings for `initializer_expression` with `razor_block` and `_expression_statement_expression` with `razor_explicit_expression`.
- Generated `src/parser.c` has large expected churn from the grammar regeneration.
