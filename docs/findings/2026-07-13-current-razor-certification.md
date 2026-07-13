# Current Razor Parser Certification

Certified on 2026-07-13 against parser-content commit `92d774bdb291f5c271b4f8d50d9b95a150bb463d`.

## Julie Extractors handoff

- Pin the Razor parser to `92d774bdb291f5c271b4f8d50d9b95a150bb463d`.
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
| Clean Node floor and binding contract | `npm ci`, generation, generated-source diff, `npm test` under Node 22.17.0 | diagnostic branch gate at parser pin | 4/4 passed; no generated drift |
| Official and full corpus | `tree-sitter test --stat total-only` plus focused `razor_html.txt` and `transitions.txt` | diagnostic branch gate at parser pin | 146/146 corpus cases and all highlight assertions passed |
| Rust binding and package contents | `cargo test --locked`; `cargo package --locked --list` | diagnostic branch gate at parser pin | 3 unit tests and 1 doc test passed; package list exact |
| Go binding | `go test ./...` | diagnostic branch gate at parser pin | passed |
| Swift binding | `swift test` | diagnostic branch gate at parser pin | 3/3 passed |
| Python ABI floor | built wheel from `/tmp`, installed with `tree-sitter==0.25.0`, ran binding tests | diagnostic branch gate at parser pin | 3/3 passed |
| Python current runtime | installed the same wheel with `tree-sitter==0.26.0`, ran binding tests | diagnostic branch gate at parser pin | 3/3 passed |
| Node package contents | `npm pack --dry-run` | diagnostic branch gate at parser pin | passed; 18 intended files |
| Workflow syntax and conventions | Actionlint 1.7.7; workflow metadata contract | diagnostic branch gate at parser pin | passed |
| Named-node and external-token contracts | `npm test` | diagnostic branch gate at parser pin | both contracts passed |

## State and constraints

- No tag, push, package publication, deployment, or Eros change occurred.
- Pre-existing `.julieignore`, `.miller/`, the dirty `/private/tmp/razor-base` worktree, and the `codex/blazor-review-fixes` worktree were preserved.
- Python wheel creation emits setuptools license-metadata deprecation warnings; they do not affect the certified parser or wheel tests.
- Generation emits two existing unnecessary-conflict warnings; generated sources remain deterministic.
