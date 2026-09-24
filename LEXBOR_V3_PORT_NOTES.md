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
