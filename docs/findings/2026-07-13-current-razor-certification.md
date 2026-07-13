# Current Razor Parser Certification

Certified on 2026-07-13 against parser-content commit `8731c35d29f4e42c043f36b1642e78da086c973c` after independent-review remediation and nested element-content coverage.

## Julie Extractors handoff

- Pin the Razor parser to `8731c35d29f4e42c043f36b1642e78da086c973c`.
- Parser ABI: 15.
- Generation CLI: `tree-sitter-cli` 0.26.11.
- The handoff target is `/Users/murphy/source/julie-extractors/docs/plans/2026-07-13-razor-parser-contract-hardening.md`; this certification did not edit Julie Extractors.

## Official documentation evidence

- `dotnet/AspNetCore.Docs` main resolved to `71011a30140248dffcc6a757d435670365f523d2` at certification time.
- That commit is already recorded beside the `DisplayName`, `BasePath`, `NavLink RelativeToCurrentUri`, `EnvironmentBoundary`, MathML, and asynchronous form-validation fixtures.
- The checked-in ASP.NET Core 10 component and full-page fixtures and all six .NET 11 preview fixtures passed without unexpected `ERROR` nodes.
- ASP.NET Core 11 remains labeled preview in repository documentation.

## Verification ledger

| Invariant | Command | Scope | Result |
|---|---|---|---|
| Clean Node floor and binding contract | `npm ci`, generation, generated-source diff, `npm test` under Node 22.17.0 | branch gate at parser pin | 9/9 passed; no generated drift |
| Official and full corpus | `tree-sitter test --stat total-only` plus focused nested-element coverage | diagnostic branch gate at parser pin | 147/147 corpus cases and all 42 highlight assertions passed |
| Rust binding and package contents | `cargo test --locked`; `cargo package --locked --list` | diagnostic branch gate at parser pin | 3 unit tests and 1 doc test passed; package list exact |
| Go binding and fork identity | `go test ./...`; workflow metadata contract | branch gate at parser pin | passed through `github.com/anortham/tree-sitter-razor` |
| Swift binding | `swift test` | diagnostic branch gate at parser pin | 3/3 passed |
| Python ABI floor | built wheel from `/tmp`, installed with `tree-sitter==0.25.0`, ran binding tests | diagnostic branch gate at parser pin | 3/3 passed |
| Python current runtime | installed the same wheel with `tree-sitter==0.26.0`, ran binding tests | diagnostic branch gate at parser pin | 3/3 passed |
| Node package contents | `npm pack --dry-run` | diagnostic branch gate at parser pin | passed; 18 intended files |
| Workflow syntax and conventions | Actionlint 1.7.7; workflow metadata contract | diagnostic branch gate at parser pin | passed |
| Named-node, external-token, scanner-state, and nested-block contracts | focused corpus test plus `npm test` | branch gate at parser pin | public node ranges, anonymous blocks inside elements, 256-byte tag names, 256 nested elements, incremental reparsing, and mismatch recovery passed |

## State and constraints

- No tag, push, package publication, deployment, or Eros change occurred.
- Pre-existing `.julieignore`, `.miller/`, the dirty `/private/tmp/razor-base` worktree, and existing linked worktrees were preserved.
- Python wheel creation emits setuptools license-metadata deprecation warnings; they do not affect the certified parser or wheel tests.
- Generation emits two existing unnecessary-conflict warnings; generated sources remain deterministic.
- The independent review's expression-range, Go identity, and scanner-state findings are repaired. The Node 22 constraint remains because newer Node lines fail the required Tree-sitter runtime's clean source build.
- Anonymous `@{ ... }` blocks directly inside HTML elements now parse as public `razor_block` nodes while retaining following sibling markup and expressions.
