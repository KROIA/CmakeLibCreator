# PROJECT_STATUS

**Current version:** Can be found in `CMakeLists.txt` (now 1.7.0). Template version comment in root `CMakeLists.txt` still reads `1.6.1` — user is migrating the tool's own template instantiation to template 1.7.0 manually.
**Branch:** `main`
**Phase:** 1.7.0 development cycle begins.

## At a glance
- No open hotfixes.
- 1 backlog task: **TASK-006** (self-upgrade mangles literal placeholder mentions; not yet started).
- No tasks in progress.
- TASK-007 (relocate to AppData) + TASK-008 (ProjectPaths editor + default library path) ✓ done 2026-05-06; pending in `changelogs/1.8.0.md` for next release.
- Code review not yet run for this cycle — recommend running the **code-review** agent against current `main` to seed `ISSUES.md`.

## Recent release notes
See `changelogs/1.7.0.md` for the most recent shipped release. `changelogs/1.8.0.md` accumulates pending work (rename if a different version number is chosen at release).

## Next suggested moves
1. User-driven: complete manual migration of this repo's own template instantiation to template 1.7.0 (drop `CMakeSettings.json` from disk, update `## Template version: 1.6.1` → `1.7.0`, apply any other template deltas).
2. Run **code-review** agent → populate `ISSUES.md`.
3. Run **unit-test** agent → seed `UNIT_TEST_TASKS.md` (no test suites authored yet).
