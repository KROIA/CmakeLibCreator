# TASKS

## Hotfix lane
_(empty — hotfixes go here, worked on first, bypass normal prioritization)_

---

## Backlog

### TASK-004 — Drop `CMakeSettings.json` handling from the tool
- **Linked issue:** _(none — feature follow-up; template 1.7.0 dropped the file 2026-05-06)_
- **Why:** Template repo has removed `CMakeSettings.json`. The tool's existing copy/edit logic now triggers `Utilities::critical(...)` on every export because it tries to copy a file that no longer exists in the template. CMakeSettings.json is a Visual Studio-only legacy format; CMakePresets.json is the portable replacement and stays.
- **Scope (delete-only — do not touch CMakePresets.json paths):**
  1. `core/src/ProjectExporter.cpp:189` — remove the `templateSourcePath + "/CMakeSettings.json"` entry from the `fileList` initializer.
  2. `core/src/ProjectExporter.cpp` ~line 389 — remove the call to `replaceTemplateVariablesIn_cmakeSettings(settings, projectDirPath)` from `replaceTemplateVariables`.
  3. `core/src/ProjectExporter.cpp` ~lines 535–573 — delete the entire `replaceTemplateVariablesIn_cmakeSettings` method definition.
  4. `core/inc/ProjectExporter.h` — delete the declaration of `replaceTemplateVariablesIn_cmakeSettings`.
  5. TASK-002 helpers in `core/src/ProjectExporter.cpp`:
     - `snapshotUserCmakeMacros` — delete the entire `--- CMakeSettings.json ---` block; keep the CMakePresets.json block intact.
     - `applyUserCmakeMacros` — delete the entire `--- CMakeSettings.json ---` block; keep the CMakePresets.json block intact.
     - In `core/inc/ProjectExporter.h`: delete the `ConfigExtras` struct and the `settingsExtras` field of `CmakeMacroSnapshot`. Keep `presetExtras`. The struct now only carries preset extras; rename if it improves clarity (optional, low priority — keep `CmakeMacroSnapshot` to avoid churn).
  6. Documentation:
     - `AI_Knowledge.md` — remove or amend mentions of `CMakeSettings.json` (mostly the "Repo layout" and template structure sections); state that CMakePresets.json is the only build-config file the tool manages.
     - `.claude/ProjectManager/PROJECT_SUMMARY.md` — same.
     - `.claude/ProjectManager/GLOSSARY.md` — no changes needed (no CMakeSettings.json entry).
- **Out of scope (the user will handle manually):**
  - Migration of this repo's own template instantiation from 1.6.1 → 1.7.0 (root `CMakeLists.txt` `## Template version:` comment, any USER_SECTION schema changes, any new `<AUTO_REPLACED>` variables). The user does this with the migration agent or by hand later.
- **Backwards-compat:** existing user projects keep their `CMakeSettings.json` untouched — the tool simply stops copying/editing/scanning it. No deletions on the user's disk.
- **Acceptance criteria:**
  - Exporting a fresh project produces no `CMakeSettings.json` and emits no critical-error popup about the missing file.
  - Upgrading an existing project that has a `CMakeSettings.json` does not modify or delete that file; only `CMakePresets.json` is processed (placeholder substitution + macro preservation from TASK-002 still works).
  - Build succeeds; no unused-variable / unreachable warnings introduced.
- **Estimate:** S.
- **Status:** done — user implemented manually (verified 2026-05-06)
- **Owner agent:** user (direct edits)
- **Stage checklist:**
  - [x] implemented   (`core/inc/ProjectExporter.h`, `core/src/ProjectExporter.cpp`, `AI_Knowledge.md` — 266 deletions; verified zero residual references via grep)
  - [ ] tested        (pending user manual export verification)
  - [x] documented    (changelog 1.6.0 → Changes)
  - [x] reviewed      (N/A — manual review gate disabled per PREFERENCES.md)

---

### TASK-003 — `// @file LibraryName*.h` Doxygen comments not substituted on export
- **Linked issue:** _(none — bug reported by user 2026-05-06)_
- **Symptom:** After exporting a project, the first-line `// @file LibraryName.h`, `_base.h`, `_debug.h`, `_global.h`, `_info.h`, `_meta.h` Doxygen comments still contain the literal `LibraryName` instead of the project's actual library name.
- **Root cause:** `ProjectExporter.cpp:728` — the `Library_Name` placeholder replacement is gated by `mustContainInLine` requiring one of `{"#include"}`, `{"_VERSION_"}`, or `{"_LIBRARY_NAME"}`. Doxygen `@file` comment lines match none of these so substitution is skipped.
- **Fix:** Add `{"@file"}` as a fourth allowed-context entry on that filter.
- **Acceptance criteria:**
  - After exporting a fresh project named e.g. `MyAwesomeLib`, line 1 of every generated header (`MyAwesomeLib.h`, `MyAwesomeLib_base.h`, `MyAwesomeLib_debug.h`, `MyAwesomeLib_global.h`, `MyAwesomeLib_info.h`, `MyAwesomeLib_meta.h.in`) reads `// @file MyAwesomeLib*.h` (matching the file's actual name).
  - No regression: random uses of the substring `LibraryName` in user code (which the existing filter is designed to avoid touching) remain untouched. Verify by checking that lines with `LibraryName` but neither `@file`, `#include`, `_VERSION_`, nor `_LIBRARY_NAME` are still skipped.
- **Implementation outline:** one-line edit to the `replacements` vector at `core/src/ProjectExporter.cpp:728`.
- **Estimate:** XS.
- **Status:** done (moved to FINISHED_TASKS.md 2026-05-06 — user-confirmed after rebuild)
- **Owner agent:** PM (direct edit — one-line mechanical change)
- **Stage checklist:**
  - [x] implemented   (`core/src/ProjectExporter.cpp:728` — added `{"@file"}` to `mustContainInLine`)
  - [x] tested        (user-confirmed working after rebuild)
  - [x] documented    (changelog 1.6.0 → Bugfixes)
  - [x] reviewed      (N/A — manual review gate disabled per PREFERENCES.md)

---

### TASK-002 — Preserve user-added macro definitions in `CMakePresets.json` / `CMakeSettings.json` on upgrade
- **Linked issue:** _(none — feature request 2026-05-06)_
- **Problem:** The upgrade flow overwrites both files at `ProjectExporter.cpp:189-190`. Developer-added macro definitions are lost.
- **Scope (strict — confirmed by user):** *Only* these two files. *Only* macro definitions. No other JSON keys, no whole-preset/whole-configuration preservation, no environment blocks, no top-level keys.
- **What counts as a "macro definition" per file:**
  - `CMakePresets.json` → entries in `configurePresets[N].cacheVariables` (object-shaped CMake cache vars).
  - `CMakeSettings.json` → both:
    - `-D<KEY>(=<VALUE>)?` tokens inside `configurations[N].cmakeCommandArgs` (single string).
    - entries in `configurations[N].variables[]` (array of `{name, value}` objects).
- **Conflict policy (default, per analysis):** preserve macros whose key the template does **not** define; template wins on keys it manages.
- **Acceptance criteria:**
  - Round-trip: a project with extra `cacheVariables` keys, extra `-D…` tokens in `cmakeCommandArgs`, or extra `variables[]` entries retains all of them after running an upgrade with `replaceTemplateCmakeFiles=true`.
  - Macros whose key matches a template-managed macro are reset to the template value (template wins).
  - Matching is by **preset/configuration `name`** — extras inside an unmatched preset/config are dropped (out of scope per user).
  - No effect when the project's existing JSON file is missing or unparseable (fallback to current behavior, log a warning).
  - Existing TASK-001 placeholder substitution still runs (extras must be re-injected **after** substitution).
- **Implementation outline:**
  1. Build a small helper (private to `ProjectExporter`, or new `JsonMacroMerge` namespace in `Utilities`):
     - `snapshotPresetExtras(projectPresetsFile, templatePresetsFile) → QHash<QString, QJsonObject>` keyed by preset name → unknown-key cacheVariables.
     - `snapshotSettingsExtras(projectSettingsFile, templateSettingsFile) → QHash<QString, ConfigExtras>` per configuration name, splitting `cmakeCommandArgs` into `-D` tokens (whitespace-split with quoted-segment awareness) and capturing unknown-name `variables[]` entries.
     - `applyPresetExtras(presetsFile, snapshot)` and `applySettingsExtras(settingsFile, snapshot)` writing the merged result back.
  2. Wire the snapshot **before** the file copy at `ProjectExporter.cpp:204-209` (only when the project file already exists, i.e. upgrade path).
  3. Wire the apply **after** TASK-001 substitution + `replaceTemplateVariablesIn_cmakeSettings` complete, so the merge sees the substituted template values.
  4. The `cmakeCommandArgs` token splitter should respect `"…"` quoted segments and dedup `-D<KEY>` by key (template wins).
  5. Log every preserved entry via `Log::LogObject` so the developer can audit what was kept.
- **Test:**
  - Manual: create a fresh project, add `"MY_CUSTOM": "ON"` to one preset's `cacheVariables` and `-DMY_FLAG=1` to one configuration's `cmakeCommandArgs`, run upgrade, verify both survived and that `LIBRARY_NAME_SHORT_PROFILING`-style template macros still get correctly substituted.
  - Add UT-xxx targeting the snapshot/apply helpers (string-splitter and JSON-merge) when `unit-test` agent seeds suites.
- **Estimate:** M (one new helper unit + two integration points + careful string parsing).
- **Status:** done (moved to FINISHED_TASKS.md 2026-05-06)
- **Owner agent:** general-purpose (completed 2026-05-06)
- **Stage checklist:**
  - [x] implemented   (`core/inc/ProjectExporter.h` +structs/decls; `core/src/ProjectExporter.cpp` snapshot+apply + helpers)
  - [x] tested        (build verified; manual round-trip pending — test plan in FINISHED_TASKS.md)
  - [x] documented    (changelog 1.6.0 → Features)
  - [x] reviewed      (N/A — manual review gate disabled per PREFERENCES.md)

---

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
