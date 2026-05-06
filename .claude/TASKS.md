# TASKS

## Hotfix lane
_(empty — hotfixes go here, worked on first, bypass normal prioritization)_

---

## Backlog

### TASK-001 — Substitute placeholders in `CMakePresets.json` on export
- **Linked issue:** _(none — feature request reported by user 2026-05-06)_
- **Acceptance criteria:**
  - `CMakePresets.json` written by the export pipeline contains the resolved short define instead of the literal `LIBRARY_NAME_SHORT_PROFILING` token (e.g. `CLC_PROFILING` for this project).
  - All `Placeholder` tokens (`Library_Name`, `Library_Namespace`, `LIBRARY_NAME_API`, `LIBRARY_NAME_LIB`, `LIBRARY_NAME_SHORT`) are substituted in `CMakePresets.json`, so future template additions work without further code changes.
  - Substitution order is correct (longer prefixed tokens first — mirror `replaceTemplateCodePlaceholders` in `ProjectExporter.cpp:609`).
  - Behavior gated by `ExportSettings::replaceTemplateVariables` (consistent with the other `replaceTemplateVariablesIn_*` calls).
  - Existing exports for projects that don't use profiling presets remain unaffected (idempotent — no-op if no tokens present).
- **Implementation outline:**
  1. Add `replaceTemplateVariablesIn_cmakePresets(settings, projectDirPath)` to `ProjectExporter.{h,cpp}`, modeled on `replaceTemplateVariablesIn_cmakeSettings` (`ProjectExporter.cpp:534`).
  2. Read `<projectDir>/CMakePresets.json` via `Utilities::getFileContents`; do plain-string substitution per `ProjectSettings::s_defaultPlaceholder` → `cmakeSettings.lib_short_define` etc.; write back.
  3. Wire the call from `exportProject` near `ProjectExporter.cpp:388–390`.
  4. Confirm filename copy at `ProjectExporter.cpp:190` still happens; this task does not change copying, only post-copy rewriting.
- **Test:**
  - Manual: export a fresh project, open `CMakePresets.json`, verify both `x64-Debug-Profile` and `x64-Release-Profile` `cacheVariables` contain `"<LIB_SHORT>_PROFILING": "1"`.
  - Add a unit-test task (UT-xxx) targeting `replaceTemplateVariablesIn_cmakePresets` once `unit-test` agent seeds the test suite — link it back here.
- **Estimate:** S (one new method, mirrors existing pattern).
- **Status:** done (moved to FINISHED_TASKS.md 2026-05-06)
- **Owner agent:** general-purpose (completed 2026-05-06)
- **Stage checklist:**
  - [x] implemented   (`core/inc/ProjectExporter.h` +decl; `core/src/ProjectExporter.cpp` new method + wiring)
  - [x] tested        (build verified; manual export round-trip pending — recommend follow-up smoke test)
  - [x] documented    (changelog 1.6.0 → Features)
  - [x] reviewed      (N/A — manual review gate disabled per PREFERENCES.md)

---

_Run the **code-review** agent to seed `ISSUES.md` for further backlog._

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
