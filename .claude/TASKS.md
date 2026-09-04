# TASKS

## Hotfix lane

### TASK-010 — Derive `lib_profile_define` from `lib_short_define`
- **Linked issue:** ISSUE-002
- **Symptom:** `-Profile` presets build clean but easy_profiler is silently disabled. `CMakeLists.txt` gets `set(LIB_PROFILE_DEFINE DTID_PROFILING)` while `CMakePresets.json` and the generated `<name>_debug.h` use `DTI801_PROFILING`.
- **Root cause:** `CMAKE_settings::autosetLibProfileDefine()` (`core/src/ProjectSettings.cpp:198-218`) derives the token from the capitals of `libraryName`; the presets path derives it from `lib_short_define` (`ProjectExporter.cpp:574`). Two sources, one token.
- **Acceptance criteria:**
  - `autosetLibProfileDefine()` returns `lib_short_define + "_PROFILING"`.
  - It runs (or is re-derived) whenever `lib_short_define` changes, including when the user edits the short define by hand in `ProjectSettingsDialog`. Check the ordering at `ProjectSettings.cpp:174-179` — `autosetLibProfileDefine()` currently runs *before* `autosetLibShortDefine()`, which would leave it empty; fix the order.
  - `ProjectExporter.cpp:784` (`readCmakeVariable(..., "LIB_PROFILE_DEFINE", ...)`) still round-trips an existing project without clobbering a user-customised value on read.
  - After export: the token in `set(LIB_PROFILE_DEFINE ...)`, the profiling key in `CMakePresets.json`, and the `#ifdef` in `<name>_debug.h` are byte-identical.
- **Estimate:** S
- **Status:** implemented 2026-09-04 — awaiting user manual test
- **Implementation:** `autosetLibProfileDefine()` reduced to `lib_short_define + "_PROFILING"` (`ProjectSettings.cpp:198-200`). Constructor call order corrected to derive the short define first (`:177-179`), same correction in `ProjectSettingsDialog.cpp:298-300`. New slot `on_libraryNameShort_lineEdit_textChanged` (`ProjectSettingsDialog.cpp:324-335`, declared in `core/inc/ui/ProjectSettingsDialog.h`) re-derives the profile define when the short define is edited by hand; it respects the `m_ignoreNameChangeEvents` guard.
- **Verified by PM:** widget name `libraryNameShort_lineEdit` confirmed at `core/ui/ProjectSettingsDialog.ui:357`, so Qt auto-connect binds (a mismatch here would compile clean and silently never fire). `getValidated()` (`ProjectSettings.cpp:115-127`) calls no autosetters, so a `LIB_PROFILE_DEFINE` read off disk at `ProjectExporter.cpp:784` is not clobbered on load and a hand override still survives to export. Signal cascade from a library-name edit terminates — the nested short-define slot re-derives an identical string.
- **Owner agent:** Task010
- **Stage checklist:**
  - [x] implemented   (compiles; `build.bat` pass both presets)
  - [x] tested        (`unittests/ExportRenameTest` → `TST_profileDefine`, 8 assertions, PASS. Drives the real `ProjectSettingsDialog` via the UnitTest GUI module and asserts exactly the reported case: library `DTI801_Driver` + hand-typed short define `DTI801` → `DTI801_PROFILING`, where the old code produced `DTID_PROFILING`.)
  - [x] documented    (`changelogs/1.8.0.md` → Bugfixes)
  - [x] reviewed      (N/A — manual review gate disabled per PREFERENCES.md)

---

### TASK-011 — Rename the previous generation before copying, so a re-export under a new name is clean
- **Linked issue:** ISSUE-001, ISSUE-003
- **Symptom:** Re-exporting an existing project under a new `LIBRARY_NAME` leaves the whole old library on disk, breaks the build, and writes the user's `USER_SECTION` code back into the orphaned old files instead of the new ones.
- **Do not** implement the bug report's suggested fix (patch the placeholder at `ProjectExporter.cpp:414`, then delete leftovers). `replaceTemplateFileNames()` is vestigial — `copyTemplateSourceFiles` already renames during the copy (`:287-290`) — and deleting leftovers destroys the only copy of the user's `USER_SECTION` content. Read ISSUE-001 in full before starting.
- **Approach:** add one step to `exportProject_intern`, placed **before** the copy at `:120-123` (and before `:135`), that migrates the previous generation in place:
  - Old name from `settings.getLoadedPlaceholder().Library_Name`. Skip the whole step when it is empty, equal to the new `cmakeSettings.libraryName`, or still equal to `s_defaultPlaceholder.Library_Name` (pristine template — nothing to migrate).
  - Recursively rename files whose name contains the old name → new name. Note `getFilesInFolderRecursive` matches by `QString::contains` (`Utilities.cpp:174`), so a `.h` filter already covers `.h.in`; confirm the file set actually collected covers `.h`, `.h.in`, `.cpp`, and anything else name-derived.
  - Recursively rename **directories** containing the old name (e.g. `examples/<OldName>_FullDemoApp/`). Deepest-first, so renaming a parent does not invalidate paths not yet processed.
  - In the same pass, rewrite the stored section keys `m_codeUserSections[].file` and `m_cmakeFileUserSections[].file` (`ProjectSettings.h:93-102`) through the identical substitution, so the re-apply at `:126`/`:136` lands on the renamed files.
  - Collision handling: this runs before the copy, so a pre-existing `<NewName>_*` file means a previous export was interrupted. The old file is authoritative (it holds the user's sections). Decide explicitly and log it — do not let a silent `QFile::rename` failure leave both generations behind.
  - `settings` must be non-const for the section-key rewrite; it already is at `:54` (`setDefaultPlaceholder`).
- **Third defect found by the new test, fixed here (2026-09-04):** after a rename the old **namespace** survived in `examples/` and `unittests/` (`Foo_Driver::Profiler::start()`), so the renamed project did not compile. Qualified uses match none of the name filters. Fixed by a second loaded-placeholder entry in `replaceTemplateCodePlaceholders` (`ProjectExporter.cpp`), filtered to lines containing `::` or `namespace`. Per user decision it rewrites **inside `USER_SECTION` blocks too** — see `DECISIONS.md` 2026-09-04. Deliberately filtered rather than using the empty `{}` filter the default-placeholder namespace entry carries, since that is the live TASK-006 defect; do not widen it.
- **Also in scope:**
  - ISSUE-003: drive the `replaceTemplateVariablesIn_cmakePresets` replacement table (`:567,573-579`) from `getLoadedPlaceholder()` with a fallback to the defaults, so stale `<OldShort>_PROFILING` cache variables are rewritten rather than accumulated. The commented-out block at `:687-691` is the same idea, unfinished — either finish it or delete it, don't leave it.
  - `replaceTemplateFileNames()` (`:410-441`): verify no remaining path leaves placeholder-named files on disk, then delete the function and its call at `:141`. If some path does still need it, say which — don't keep it "just in case".
- **Acceptance criteria (regression test, from the bug report):** export the template as `Foo_Driver`, put a marker line inside a `USER_SECTION` in `core/src/Foo_Driver_debug.cpp`, build it, then re-export the same directory as `Bar_Driver` and assert:
  1. no file or directory under the project matches `*Foo_Driver*`;
  2. the marker line is present in `core/src/Bar_Driver_debug.cpp` — this is the one that catches the user-code-loss regression;
  3. `grep -r "Foo_Driver\|FD_PROFILING"` over the project excluding `build/` returns nothing outside `USER_SECTION` blocks;
  4. `CMakePresets.json` carries exactly one `*_PROFILING` cache variable per `-Profile` preset;
  5. the token in `set(LIB_PROFILE_DEFINE ...)` equals the profiling key in `CMakePresets.json` and equals the `#ifdef` in `<name>_debug.h` (overlaps TASK-010);
  6. a fresh configure+build of `x64-Debug` and `x64-Release-Profile` succeeds.
- **Estimate:** M
- **Status:** implemented 2026-09-04 — awaiting user manual test
- **Implementation:** new `ProjectExporter::migratePreviousGeneration()` (`ProjectExporter.cpp:415-457`), called first thing in the upgrade branch at `:117`, before either copy. Skips entirely when the old name is empty, unchanged, or still the pristine template token. Renames under `core`, `examples`, `unittests` only — deliberately **not** the whole project directory, which keeps the sweep out of `.git/`, `build/` and `installation/`. New helper `Utilities::renameEntriesContaining()` (`Utilities.cpp:142-199`) renames files first (their parent folders still carry the old name, so targets resolve), then folders longest-path-first, which is deepest-first since a parent path is a strict prefix of its children. An existing target is treated as the remains of an interrupted export, removed with a warning, and the entry being renamed wins — it is the one carrying the user's sections. Section keys migrated in the same function and set back via the existing setters; no `ProjectSettings` change was needed. Presets replacement table now driven by the loaded placeholder with a per-token fallback to the defaults. `replaceTemplateFileNames()` deleted along with its declaration and its call site.
- **Verified by PM:** rename scope confirmed non-destructive to `.git/`. Key migration hardened after review — both sides of the path compare are normalized with `QDir::cleanPath(QDir::fromNativeSeparators(...))`, since the keys come from `QFileInfo::absoluteFilePath()` (always forward slashes) while `projectDirPath` is whatever `Resources::getLoadedProjectPath()` holds; a separator mismatch would have silently skipped every key and reinstated the data loss. Task011 additionally caught that a bare `startsWith` matches root `C:/proj` against `C:/project/core/x.h` and added a separator boundary check. Both failure paths now `logError` naming the file.
- **Correct by design, confirmed by user 2026-09-04:** example folders are not renamed. `Examples/LibraryExample` is the right name for *any* library name — it carries no placeholder and is never name-derived. Folders a user adds alongside it (the reporter's `examples/RFID_FullDemoApp/`) are free-form names that merely share a prefix with the old library, and `Examples/CMakeLists.txt` globs every subdirectory, so they build under any name. Only entries actually containing the old library name are migrated. See `DECISIONS.md` 2026-09-04 — do not "fix" this to match partial prefixes.
- **Owner agent:** Task011
- **Stage checklist:**
  - [x] implemented   (compiles; `build.bat` pass both presets)
  - [x] tested        (`unittests/ExportRenameTest` → `TST_exportRename`, PASS — **all six acceptance criteria covered**. Criterion 2: a marker seeded inside a `USER_SECTION` of `Foo_Driver_debug.cpp` is asserted present in `Bar_Driver_debug.cpp` after the rename. Criterion 6: the generated project is built on **both sides** of the rename — the pre-rename build leaves a configured CMake cache carrying the old file names, and the post-rename build must survive it. Proof the glob was re-evaluated rather than served from cache: the post-rename build output is asserted to contain `Bar_Driver_debug.cpp` and to NOT contain `Foo_Driver_debug.cpp`. Measured 10 s pre-rename, 5 s post-rename, 17.6 s for the whole suite, 596 MB peak temp tree, fully removed afterwards.)
  - [x] documented    (`changelogs/1.8.0.md` → Bugfixes)
  - [x] reviewed      (N/A — manual review gate disabled per PREFERENCES.md)

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
