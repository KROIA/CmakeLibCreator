# PROJECT_STATUS

**Current version:** Can be found in `CMakeLists.txt` (now 1.7.0). Template version comment in root `CMakeLists.txt` still reads `1.6.1` — user is migrating the tool's own template instantiation to template 1.7.0 manually.
**Branch:** `main`
**Phase:** 1.7.0 development cycle begins.

## At a glance
- **2 hotfixes implemented and automatically tested 2026-09-04** (from user bug report `.claude/bugreports/2026-09-04-rename-leaves-old-library-files.md`, verified by source trace). `build.bat` passes `x64-Debug` and `x64-Release`; `installation/bin/ExportRenameTest.exe` reports `All Tests: PASS`, exit 0. The new suite found and fixed a **third** defect (old namespace surviving a rename, so the renamed project did not compile). Not committed.
  - **TASK-011 / ISSUE-001** — critical. Re-exporting an existing project under a new library name orphans the whole previous generation of files *and writes the user's `USER_SECTION` code back into the orphaned files*, so the new library ships with empty user sections. Build breaks loudly (stale files are globbed and compiled), which is the only reason the reporter noticed. The bug report's own suggested fix — delete the leftovers — would turn this into silent user-code loss; TASK-011 reorders instead. Also folds in ISSUE-003 (stale `*_PROFILING` cache vars accumulating in `CMakePresets.json`).
  - **Also fixed 2026-09-04, found while writing the test suite:** `CMAKE_settings`' constructor left `qt_versionNr[3]`, `qt_useNewestVersion` and `qt_autoFindCompiler` uninitialised. Masked on the GUI path (the dialog writes them first) but read by `getValidated()` → `getQtVersionStr()`. All Qt fields now default to the template's own values (`C:/Qt`, Qt 5, auto-find version, auto-find compiler). An empty Qt module list stays deliberate — confirmed with the user, that is what the dialog presents.
  - **TASK-010 / ISSUE-002** — critical, independent of renaming. `lib_profile_define` is derived from the capitals of the library name while the presets use `lib_short_define`; when they disagree the `-Profile` presets build fine but easy_profiler is silently disabled.
- 1 backlog task: **TASK-006** (self-upgrade mangles literal placeholder mentions; not yet started).
- 1 open low issue with no task yet: **ISSUE-004** (no post-rename report of leftovers in user-owned files / build artifacts).
- No tasks in progress.
- TASK-007 (relocate to AppData) + TASK-008 (ProjectPaths editor + default library path) ✓ done 2026-05-06; pending in `changelogs/1.8.0.md` for next release.
- **TASK-009** (Repositories tab + per-repo cards) ✓ implemented 2026-07-29 **+ review rounds 1 & 2**. Round 2: parallel `BuildRunner` pool (`maxBuildThreads` setting, default 4), per-repo collision model replacing the global UI lock, empty-build-log fix, close-crash fix. Compiles (build.bat pass both presets). **User manual re-test pending** before commit (commit on explicit user command only). Pending in `changelogs/1.8.0.md`.
- Code review not yet run for this cycle — recommend running the **code-review** agent against current `main` to seed `ISSUES.md` (new engine `RepositoryJobQueue` is a good target).

## Recent release notes
See `changelogs/1.7.0.md` for the most recent shipped release. `changelogs/1.8.0.md` accumulates pending work (rename if a different version number is chosen at release).

## Next suggested moves
1. **User: manual walkthrough of the Repositories tab** (see `changelogs/1.8.0.md` / plan verification section), then commit TASK-009 on explicit command.
2. User-driven: complete manual migration of this repo's own template instantiation to template 1.7.0 (drop `CMakeSettings.json` from disk, update `## Template version: 1.6.1` → `1.7.0`, apply any other template deltas).
3. Run **code-review** agent → populate `ISSUES.md` (target `RepositoryJobQueue`, MainWindow flows).
4. Run **unit-test** agent → seed `UNIT_TEST_TASKS.md`: `readTemplateVersion` + `LoadSaveProjects` format migration.
