# FINISHED_TASKS

Completed tasks for the **current** version. Rotated into `changelogs/<version>.md` at release.

### TASK-001 — Substitute placeholders in `CMakePresets.json` on export — ✓ 2026-05-06
- Added `ProjectExporter::replaceTemplateVariablesIn_cmakePresets`; wired into `replaceTemplateVariables` next to the existing `_mainCmakeLists`, `_cmakeSettings`, `_libraryInfo` calls.
- Substitutes all five `Placeholder` tokens (longest-prefix first) so `LIBRARY_NAME_SHORT_PROFILING` resolves to e.g. `CLC_PROFILING` in exported `CMakePresets.json`.
- Idempotent: no write if no token matched. No-op if `CMakePresets.json` is missing (older projects unaffected).
- Build verified.
- Files: `core/inc/ProjectExporter.h`, `core/src/ProjectExporter.cpp`.
