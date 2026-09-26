# lexbor v3.0.0 port notes (branch lexbor-v3)

Submodule `vendor/lexbor`: b2c0a617 (2023-01-06) -> v3.0.0 (2026-03-31), 457 commits.
`source/lexbor/selectors/selectors.c` was rewritten upstream (state-machine matcher, 2,060 lines changed).

| Patch | Status on v3.0.0 | Notes |
|---|---|---|
| 0001 support-text-pseudo-element | **conflicts; port required** | Parser-table hunks (`pseudo_const.h`, `pseudo_res.h`, `utils/.../pseudo.py`) apply as-is. Matcher hunks must be redone: v3 gates traversal on `node->type == LXB_DOM_NODE_TYPE_ELEMENT` in ~12 places (`selectors.c` ~312-1200) and `lxb_selectors_pseudo_element()` (~2071) returns false for all pseudo-elements. `::text` must (a) admit TEXT nodes as candidates when the compound's last simple selector is `::text`, (b) match them in `lxb_selectors_pseudo_element`. SerpApi app: 153 `::text` call sites -> mandatory. |
| 0002 match-id-class-case-sensitive | **ported** (`patches/0002-...patch.v3`) | 2 sites: `selectors.c:1313` class (`true` -> `false`), `:1372` id (`ncasecmp` -> `ncmp`). SerpApi app: 3,293 mixed-case class selectors -> mandatory. |
| 0003 attach-template-content-to-self | applies clean | |
| 0004 fix-template-clone | conflicts (context only: `document.c` mode 100755 and includes moved) | Re-apply by hand in `lxb_dom_document_import_node`/clone path; logic unchanged. |
| 0005 add-source-location-to-node | applies clean | |
| 0006 fix-sibling-combinator-in-pseudo-class-functions | conflicts; **likely obsolete** | v3 runs `:is()/:not()/:where()` nested lists through the full matcher via `lxb_selectors_nested_make` + `state_find`, which should already backtrack correctly. Verify with nokolexbor's `test/` cases added in 3900438 / 2e420e3 before dropping. |

Perf-relevant upstream changes to enable/measure: `LXB_DOM_DOCUMENT_OPT_WO_EVENTS` (3.0, skip mutation callbacks during parse), selector engine rewrite (2.5), SWAR core (2.4), slow-realloc fix (2.5).

Build knob: lexbor flags reach the build only via `ENV["CMAKE_FLAGS"]` in `ext/nokolexbor/extconf.rb`
(`-DLEXBOR_OPTIMIZATION_LEVEL='-O3 -mcpu=native' -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON`); a `--with-lexbor-cflags` option is TODO.

## Status after Phase 0 night (2026-09-24)

Built and linked against lexbor v3.0.0 in the v13 reference container (Debian 13, Ruby 3.3.10 aarch64).

Binding fixes (`ext/nokolexbor/nl_node.c`): `lxb_css_parser_init(parser, tkz)`, `lxb_tag_name_by_id(id, &len)`,
callback `spec` by value, and — the crash — the thread-cached CSS parser must own an explicit
`lxb_css_memory_t`/`lxb_css_selectors_t` and be cleaned per call with `lxb_css_memory_clean()`;
`lxb_css_selector_list_destroy_memory()` destroyed the parser's arena → segfault on the 2nd `css()`.

lexbor changes in-tree (`patches.v2-pin/ALL-PATCHES-ON-v3.0.0.diff` is the combined diff):
0001 `::text` ported (match(): non-element nodes only match `::text`; tree walk/match_node/descendant_forward
admit TEXT nodes; `pseudo_element()` handles PSEUDO_ELEMENT_TEXT), 0002 re-applied, 0003/0005 applied,
0004 by hand, 0006 dropped (obsolete: v3 nests `:is()/:not()` through the full matcher; tests pass).
New: `lxb_selectors_nested_t.scope` + `lxb_selectors_leading_combinator_ok()` — upstream v3 ignores the
leading combinator of relative lists (`> h1` behaved like `h1`); now scoped to the search root /
the node under test in nested contexts.

nokolexbor tests: **305/307**. Remaining: `spec/patch_spec.rb` template-content tests (0003/0004 apply
textually but v3 restructured `<template>` handling — redo).

**SerpApi parity: 89/200 fixtures differ, all with LESS data.** Root cause is NOT selectors (match counts
identical) but **serialization**: v3 emits empty attributes as `jsshadow=""` / `data-sms=""` where the 2023
pin emits bare `jsshadow` / `data-sms`. The app's JS-data extractors regex over `to_html`/`inner_html`, so
the extra `=""` breaks them. Fix in `source/lexbor/html/serialize.c` (attribute serialization: omit `=""` for
empty values) or in the extractors' regexes; the former restores byte-parity.

**Performance (SerpApi `benchmark:parser`, 200 fixtures × 3, single process, same container, interleaved):**
stock 0.6.4 (2023 lexbor) 20.42 / 19.90 parses/s; 0.8.0 + lexbor v3.0.0 19.35 / 19.40 parses/s → **≈ −3 %,
no gain**. Selector matching is 45 % of parse! CPU but v3's rewritten engine is not faster on our selector
mix (many short class/attribute selectors per document; the cost is the per-call tree walk, not the matcher).
`LXB_DOM_DOCUMENT_OPT_WO_EVENTS` not yet tried (parse_native is only 2 % of CPU, so it cannot matter much).

Conclusion: the lexbor upgrade is a maintenance/security decision, not a performance lever. The performance
lever inside nokolexbor would be **caching/indexing at the binding level** (e.g. an id/class index per
document so `css(".Ab")` doesn't walk the tree; ~45 % of CPU is repeated tree walks) — an app/gem-level
project outside the OS-image scope.

## How to reproduce this branch's lexbor state
`vendor/lexbor` is checked out at upstream `v3.0.0` (2ae88a1); the SerpApi changes are **uncommitted edits in the
submodule**, captured in full as `patches.v2-pin/ALL-PATCHES-ON-v3.0.0.diff`. To rebuild:
```
git submodule update --init && git -C vendor/lexbor checkout v3.0.0 && git -C vendor/lexbor apply ../../patches.v2-pin/ALL-PATCHES-ON-v3.0.0.diff
bundle exec rake compile && bundle exec rake test   # 305/307 (2 <template> content specs pending)
```
Before this can replace `patches/`, split the diff back into per-feature patches (::text, case-sensitive id/class,
template content ×2, source location, relative-selector scope) and add a serializer option to keep bare empty
attributes (`x` not `x=""`) for downstream parity.
