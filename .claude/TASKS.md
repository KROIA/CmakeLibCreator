# TASKS

## Hotfix lane
_(empty — hotfixes go here, worked on first, bypass normal prioritization)_

---

## Backlog

### TASK-006 — Self-upgrade mangles literal placeholder mentions in tool source
- **Linked issue:** _(none — discovered during 1.6.1 → 1.7.0 template migration of this repo)_
- **Symptom:** When the tool self-applies a template upgrade, `replaceTemplateCodePlaceholders` rewrote a literal `LIBRARY_NAME_SHORT` mention inside a comment in `core/src/ProjectExporter.cpp` to `CLC`. The comment was discussing the placeholder itself, so the rewrite makes it less informative. Same vulnerability exists for `Library_Namespace`. (Reverted manually before the 1.7.0 migration commit.)
- **Root cause:** In `replaceTemplateCodePlaceholders` (`core/src/ProjectExporter.cpp` ~line 720+), two replacements have an empty `mustContainInLine` filter (`{}`):
  - `defaultPlaceholders.LIBRARY__NAME_SHORT → cmakeSettings.lib_short_define`
  - `defaultPlaceholders.Library_Namespace → librarySettigns.namespaceName`
  An empty filter means "replace on every line", which eats legitimate prose / comments / string literals that mention the token name.
- **Why other tokens are safe:**
  - `LIBRARY_NAME_API` — filter requires API name + space, or `#` + `define`.
  - `LIBRARY_NAME_LIB` — filter requires `#` on the line.
  - `Library_Name` — filter requires `#include`, `_VERSION_`, `_LIBRARY_NAME`, or `@file` (TASK-003).
- **Fix:** Add narrow filters to the two unfiltered substitutions, mirroring the `Library_Name` approach. For C++ code the legitimate substitution targets are typically:
  - `#define` lines (where `<SHORT>_PROFILING` etc. appear)
  - `#ifdef` / `#if defined` lines
  - `#include` lines (rare for short, but possible for namespace)
  - Lines containing `namespace` (for `Library_Namespace`)
- **Acceptance criteria:**
  - The tool's own source survives a self-upgrade unchanged in comment/prose lines that mention `LIBRARY_NAME_SHORT` or `LibraryNamespace` as text.
  - Legitimate substitutions in `#define`, `#ifdef`, `namespace`, `using namespace`, and `#include` lines still happen.
  - Manual round-trip: re-run a template upgrade against this repo after the fix; `git diff` should show zero spurious changes in C++ comments.
- **Implementation outline:** edit the `replacements` vector in `replaceTemplateCodePlaceholders`. Replace the two `{}` filter slots with narrow filter lists.
- **Estimate:** S.
- **Status:** pending
- **Owner agent:** _(to be assigned)_
- **Stage checklist:**
  - [ ] implemented
  - [ ] tested        (manual self-upgrade round-trip)
  - [ ] documented    (changelog → Bugfixes for next release)
  - [x] reviewed      (N/A — manual review gate disabled per PREFERENCES.md)

---

_Run the **code-review** agent to seed `ISSUES.md` for further backlog items._

---

## Task entry template

```
### TASK-<id> — <title>
- **Linked issue:** ISSUE-<id> (if applicable)
- **Acceptance criteria:** <bullet list>
- **Estimate:** S | M | L
- **Status:** pending | in-progress | blocked | review | done
- **Owner agent:** <agent name>
- **Stage checklist:**
  - [ ] implemented
  - [ ] tested        (N/A if not testable)
  - [ ] documented    (N/A if not a feature/API change)
  - [ ] reviewed      (N/A — manual review gate disabled)
```
