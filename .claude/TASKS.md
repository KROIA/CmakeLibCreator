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

### TASK-009 — Repository group-operations GUI (Repositories tab + per-repo cards)
- **Linked issue:** _(none — feature request)_
- **Acceptance criteria:**
  - Remove the four legacy "all projects" ribbon buttons; add ribbon tab "Repositories" with group buttons.
  - Central `QTabWidget` (page 0 project editor, page 1 repository overview) synced with the ribbon tab both directions.
  - One card per repository: persisted group checkbox, status (dirty, HEAD, lib + template version, build + unittest status, log window), individual action buttons.
  - Sequential cancelable `QProcess` job queue with status-bar progress + Cancel (kills process tree).
  - Warning flows: dirty-before-update, failed-build/test-before-commit, discard confirmation, group summary dialog (Proceed all / Skip affected / Cancel).
  - Persisted selection survives restart; old settings.json format still loads.
- **Estimate:** L
- **Status:** done — base feature + review round 1 implemented 2026-07-29 (user manual walkthrough pending; commit on explicit user command only)
- **Review round 1 (2026-07-29):** fixed queue-worker-on-GUI-thread freeze; build log capture + live console; parallel off-queue `UnitTestRunner` (guard: no unittest while same repo builds; missing-exe error popup); per-button status labels + blue working state + tooltips; dirty label green/orange; build & unittest result popups (`JobResultDialog`) reusing `TextLogWindow`; update skip-if-current; ignore commit-with-no-changes / push-with-nothing; clean confirmation. Build passes both presets.
- **Review round 2 (2026-07-29):** fixed empty build "Show log" (merge clobbered `buildLog`); fixed close-time crash (tab-sync teardown); moved Build into a parallel `BuildRunner` pool capped by new `maxBuildThreads` setting (default 4, Settings dialog); replaced the global UI lock with a per-repo collision model (`repoCollision` + `applyCollisionLock`) so only same-repo colliding buttons disable. Build passes both presets.
- **Owner agent:** coding subagents (coordinated by PM)
- **Stage checklist:**
  - [x] implemented   (TASKs 1–6; project compiles, build.bat pass both presets)
  - [ ] tested        (manual walkthrough by user — see changelog verification section; no automated suite yet)
  - [x] documented    (`changelogs/1.8.0.md`, `Documentation/RepositoriesTab.md`, `AI_Knowledge.md`)
  - [x] reviewed      (N/A — manual review gate disabled per PREFERENCES.md)
- **Follow-up:** unit-test suite for `readTemplateVersion` + `LoadSaveProjects` format migration (assign to unit-test agent).

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
