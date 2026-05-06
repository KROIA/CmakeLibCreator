# FINISHED_TASKS

Completed tasks for the **current** version. Rotated into `changelogs/<version>.md` at release.

### TASK-003 — `// @file LibraryName*.h` Doxygen comments not substituted on export — ✓ 2026-05-06
- One-line filter fix at `core/src/ProjectExporter.cpp:728`: added `{"@file"}` to the `mustContainInLine` allow-list for the `Library_Name` substitution. Doxygen `@file` comment lines now get the placeholder replaced alongside `#include`, `_VERSION_`, and `_LIBRARY_NAME` lines.
- Narrow filter preserved — random uses of the substring `LibraryName` in user code remain untouched.
- User-confirmed after rebuild.
- Files: `core/src/ProjectExporter.cpp`.

### TASK-002 — Preserve user-added macros in `CMakePresets.json` / `CMakeSettings.json` on upgrade — ✓ 2026-05-06
- Added `ProjectExporter::snapshotUserCmakeMacros` (runs before file copy on upgrade path) and `applyUserCmakeMacros` (runs after placeholder substitution).
- Preserved macros: `configurePresets[].cacheVariables` keys absent from the template; `-D<KEY>(=<VALUE>)?` tokens in `configurations[].cmakeCommandArgs` whose key is absent from the template; `configurations[].variables[]` entries whose `name` is absent from the template.
- Conflict policy: template wins on shared keys.
- Quoted-segment-aware tokenizer handles `-DPATH="C:/Some Dir"` correctly.
- Each preserved entry logged via `Log::LogObject` (info).
- Falls back gracefully if either JSON file is missing/unparseable; warning logged, normal upgrade continues.
- Files: `core/inc/ProjectExporter.h`, `core/src/ProjectExporter.cpp`. Build verified.
- **Manual test plan (pending):**
  1. Add `"MY_CUSTOM": "ON"` to a preset's `cacheVariables`.
  2. Add `-DMY_FLAG=1` and `-DMY_PATH="C:/Some Dir"` to a configuration's `cmakeCommandArgs`.
  3. Add `{ "name": "MY_VAR", "value": "42" }` to a configuration's `variables[]`.
  4. Run upgrade with `replaceTemplateCmakeFiles=true`. Verify all four survive; template-managed macros (e.g. `*_PROFILING`) still substitute; "Preserved user …" lines appear in the log.
  5. Conflict test: set a template-managed cacheVariable to a different value in the project; after upgrade, template's value wins, no preserved-log entry emitted for it.
  6. Robustness: corrupt the project's `CMakePresets.json` — upgrade completes, warning logged, no crash.
- **Footnote:** subagent attempted an unrelated `LIBRARY_VERSION` bump (1.6.0 → 1.6.1) in root `CMakeLists.txt`; PM reverted it. If a version roll is intended, do it deliberately via the release workflow.

### TASK-001 — Substitute placeholders in `CMakePresets.json` on export — ✓ 2026-05-06
- Added `ProjectExporter::replaceTemplateVariablesIn_cmakePresets`; wired into `replaceTemplateVariables` next to the existing `_mainCmakeLists`, `_cmakeSettings`, `_libraryInfo` calls.
- Substitutes all five `Placeholder` tokens (longest-prefix first) so `LIBRARY_NAME_SHORT_PROFILING` resolves to e.g. `CLC_PROFILING` in exported `CMakePresets.json`.
- Idempotent: no write if no token matched. No-op if `CMakePresets.json` is missing (older projects unaffected).
- Build verified.
- Files: `core/inc/ProjectExporter.h`, `core/src/ProjectExporter.cpp`.
