# Claude Review Remediation Design

## Goal

Repair the three confirmed review findings without widening the supported Node runtime or changing the inherited C# external-token contract.

## Scope

- Keep Razor implicit-expression node ranges limited to the expression text while preserving whitespace-based expression termination.
- Make scanner serialization preserve structurally valid documents when the full tag stack or a tag name does not fit in Tree-sitter's serialization buffer.
- Make the Go module identity match the advertised fork URL.
- Keep the verified Node range at `>=22 <23`.

## Design

### Implicit-expression boundary

`RAZOR_IMPLICIT_END` remains a consuming external lookahead token so C# parsing stops before following markup text. A hidden grammar wrapper owns that token after the public `razor_implicit_expression`, whose range contains only the Razor marker and C# expression. A zero-width external token was rejected after the mixed-attribute corpus proved it could not hand off to `_html_attribute_tail_text` at the same position.

The caller-facing test parses real Razor through the Node binding and asserts the expression's text and end position for same-line and next-line markup.

### Bounded tag-stack serialization

Follow the current `tree-sitter-html` bounded-state pattern:

- Encode the total tag count and the count of complete serialized entries separately.
- Use 16-bit counts so the state format is not limited to 255 stack entries.
- Serialize only complete tag names that fit; never truncate a name into a false identity.
- Reconstruct omitted or oversized entries as unknown placeholders.
- An unknown top-of-stack entry accepts and consumes one closing tag. Known entries continue to require exact case-insensitive matching.

This preserves nesting structure after scanner state restoration while degrading only mismatch validation for entries whose identity could not fit in the fixed buffer. It avoids hash collisions and avoids rejecting otherwise valid deep or long custom-element names.

Caller-facing tests cover 256 nested elements, a 256-byte tag name, and incremental reparsing after an inner edit. Existing mismatched-tag recovery remains green.

### Go module identity

Change the root module path and binding test import to `github.com/anortham/tree-sitter-razor`. Extend the metadata contract so local tests cannot pass with a remotely uninstallable module identity.

## Architecture Quality

**Affected modules:** `grammar.js`, `src/scanner.c`, generated parser sources, Node contract tests, `go.mod`, Go binding tests, and the workflow metadata contract.

**Caller-facing interface:** Razor syntax-tree node ranges, HTML/Razor parse validity after scanner restoration, and the public Go import path.

**Depth/locality check:** Scanner complexity stays inside the existing external-scanner seam. No new module or grammar composition layer is introduced.

**Test surface:** All behavior is exercised through public parser bindings and the published module metadata rather than private scanner helpers.

**Rejected shortcuts:** Keeping the consuming boundary inside the named node, using a zero-width boundary that breaks mixed attributes, truncating tag names, hashing names with collision risk, explicitly rejecting valid deep markup, widening Node support without a green dependency build, and documenting corruption instead of fixing it.

**Architecture risk:** Medium because scanner state and named-node ranges are shared public contracts.

## Acceptance Criteria

- [x] Same-line and next-line text remains outside `razor_implicit_expression`.
- [x] Existing implicit expressions still terminate before following Razor markup text.
- [x] A 256-byte tag name parses without `ERROR`.
- [x] At least 256 nested matching elements parse without `ERROR` in full and incremental parses.
- [x] Existing mismatched-tag recovery behavior remains intact.
- [x] The Go binding is addressable through `github.com/anortham/tree-sitter-razor/bindings/go`.
- [x] The metadata contract locks the Go module path to the fork identity.
- [x] Deterministic generation, corpus/highlights, all binding suites, packages, and workflow checks pass.
- [x] The certified Julie parser pin and verification ledger are updated to the repaired parser commit.
- [x] No push, tag, publication, deployment, or external-repository edit occurs.
