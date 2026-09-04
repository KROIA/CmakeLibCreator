# DECISIONS — Architectural Decision Log

Append entries chronologically. Keep them short — capture the *why*.

## Format
```
### YYYY-MM-DD — <decision title>
**Context:** <one or two sentences on the situation>
**Decision:** <what was decided>
**Why:** <rationale>
**Consequences:** <follow-on effects, if any>
```

---

### 2026-05-06 — Adopt Project Manager scaffolding
**Context:** Project had no formal task / issue / decision tracking; PM agent setup spec was loaded into `CLAUDE.md`.
**Decision:** Initialize the standard `.claude/` layout (top-level state files + `ProjectManager/` internal artifacts + `agents/` templates), enable changelog in repo root, work on `main` only, no manual review gate, KROIA UnitTest as test framework, commit allowed but push denied.
**Why:** Codify project workflow for repeatable, auditable AI-assisted development.
**Consequences:** Subagents inherit `CODING_STYLE.md`; PM commits are scoped (no push); release workflow keyed off `LIBRARY_VERSION` in root `CMakeLists.txt`.

### 2026-07-29 — Sequential cancelable RepositoryJobQueue for repo operations
**Context:** The new Repositories tab runs pull/push/commit/discard/update/build/clean/unittest across many repositories, including cancelable long-running external builds. The legacy pattern (one bare `QThread` + lambda on `QThread::started`, blocking `_popen` in `Utilities::executeCommand`) cannot be canceled and has no per-repo progress.
**Decision:** Introduce `RepositoryJobQueue` (`core/inc/RepositoryJobQueue.h`, `.cpp`): a single worker thread that runs jobs strictly sequentially, using `QProcess` for external commands and `taskkill /PID <pid> /T /F` to kill the whole process tree on cancel. It emits per-repo signals (`jobStarted/jobFinished/statusRefreshed/templateUpdated/unitTestLogReady/allJobsFinished`). The legacy single-`QThread` pattern is KEPT for tab-1 flows (open/save/save-as/download-template).
**Why:** Cancelability, strict sequential ordering for group builds, and per-repository status without reworking the existing tab-1 flows.
**Consequences:** New process code uses `QProcess` (working-directory based, fixes paths-with-spaces); `RepositoryInfo` must be a registered metatype for queued signal delivery. Unittest pass/fail uses the KROIA UnitTest inverted exit code (NormalExit && exitCode != 0 == PASS).

### 2026-07-29 — settings.json `projectPaths` schema: string array → object array
**Context:** Per-repository group-selection state must be persisted alongside each project path.
**Decision:** `LoadSaveProjects` now stores `QVector<ProjectEntry{ path, groupEnabled }>`; `save()` writes `[{"path": "...", "groupEnabled": true}, ...]`. `load()` is backward-compatible — a legacy plain-string entry loads as `{path, groupEnabled=true}`. The JSON key stays `"projectPaths"`.
**Why:** Persist the group checkbox without breaking existing settings files.
**Consequences:** `save()` switched from `push_front` to `push_back`, incidentally fixing the long-standing path-order-reversal bug.

### 2026-07-29 — Concurrency model: sequential queue for mutating ops, parallel unittests
**Context:** First user test of the Repositories tab: builds froze the UI (job worker was accidentally running on the GUI thread), and running unittests one repo at a time was too slow.
**Decision:** Keep a single sequential `RepositoryJobQueue` for the mutating/ordered operations (build, pull, push, commit, discard, clean, update-template) — exclusive while busy. Move unittests out of the queue into a dedicated `UnitTestRunner` (`core/inc/UnitTestRunner.h`, `.cpp`) that runs one worker thread per repository, in parallel. A repository's unittest is refused while that same repository has a Build active in the queue (and no duplicate unittest per repo); unittests may run against externally-built binaries as long as `build/Release/<Suite>.exe` exist, else a `noExecutables` error popup.
**Why:** Unittest executables are independent per repo and safe to run concurrently; the mutating ops need ordering and a shared status bar / cancel. The threading bug was a `connect(m_thread, &QThread::started, m_thread, ...)` whose GUI-affinity context object forced the worker slot onto the GUI thread — fixed by dropping the context object (functor runs direct on the emitting worker thread).
**Consequences:** `JobType::UnitTest` and `unitTestLogReady` removed from the queue; unittest status/logs now flow from `UnitTestRunner` signals. Two `JobResultDialog` instances (build / unittest) plus per-button working-state visuals surface progress; the shared `TextLogWindow` remains the single log viewer.

### 2026-07-29 — Parallel build pool + per-repo collision model (review round 2)
**Context:** User feedback: a single build locked the whole UI, and builds ran one at a time. Requirement: run several builds concurrently (configurable) and only disable actions that actually collide on the same repository.
**Decision:** Move Build out of the sequential `RepositoryJobQueue` into a dedicated `BuildRunner` (`core/inc/BuildRunner.h`, `.cpp`) — a parallel per-repo build pool capped by a new `Resources::maxBuildThreads` setting (default 4, editable in the Settings dialog). Replace the global UI lock (`setAllBusy` / `setRepositoryButtonsEnabled`) with a per-repo `MainWindow::repoCollision(path, RepoOp)` model and `RepositoryWidget::applyCollisionLock`, so only same-repo colliding buttons disable while a repo is building/testing/has an active queue job. Cross-repo activity never disables anything. `RepositoryJobQueue` now handles only the quick ordered ops (pull/push/commit/discard/clean/update-template); `isAnyWorkRunning` stays for the legacy tab-1 worker-thread flows only.
**Why:** Builds are independent per repo and the slow operation; serializing them and locking the UI was the main friction. A per-repo collision model keeps the tool responsive while still preventing genuine same-repo clashes (build vs clean/update/unittest).
**Consequences:** Three concurrent execution mechanisms now coexist (sequential queue, build pool, unittest runner); collision state is unified via `repoCollision` and pushed to cards. `RepositoryJobQueue::runProcessJob`/`cancel` are now effectively unused (its remaining ops are in-process); the status bar shows a lightweight "Building N repositories…" indicator and Cancel cancels both the queue and the build pool. Build result/log now flow from `BuildRunner::started/finished` (queue `buildLogReady`/`JobType::Build` removed).

### 2026-09-04 — Rename on re-export: migrate the previous generation before copying, never after
**Context:** Re-exporting an existing project under a new `LIBRARY_NAME` left the whole previous generation of `<OldName>_*` files on disk and broke the build. The obvious fix — delete the leftovers after export — is actively dangerous: user sections are keyed by absolute file path, the old files were still present when sections were re-applied, so the user's `USER_SECTION` code was being written back into the stale files. Deleting them would have turned a loud build failure into silent code loss.
**Decision:** Added `ProjectExporter::migratePreviousGeneration()`, called first in the upgrade branch **before any copy**. It renames the old files and folders on disk and rewrites the stored user-section keys in the same pass, so every downstream step then behaves exactly as it does for a project that was not renamed. No deletion step, no post-export cleanup pass. Scope is limited to `core`/`examples`/`unittests` — deliberately not the project root, which keeps the sweep out of `.git/`, `build/` and `installation/`. Both sides of the section-key path comparison are normalized (`QDir::cleanPath` + `fromNativeSeparators`), and a key that cannot be migrated is logged as an error rather than passed through silently.
**Why:** Ordering is the root cause; deletion only treats the symptom and introduces a worse, silent one. Fixing it once at the point all callers route through is smaller than guarding each step afterwards.
**Consequences:** `replaceTemplateFileNames()` deleted — it had been dead since the template copy started renaming files inline, searching for a `LibraryName` token that no longer exists on disk by the time it ran. The `CMakePresets.json` replacement table is now driven by the loaded placeholder (falling back per-token to the defaults) so stale tokens are rewritten rather than accumulated.

### 2026-09-04 — `LibraryExample` is a fixed template name, not a name-derived one
**Context:** A bug report listed renaming `examples/<Old>_FullDemoApp/` among the things the tool should have done during a library rename. The rename pass does not touch it.
**Decision:** Confirmed by the user: `Examples/LibraryExample` is the correct folder name for **any** library name. It carries no placeholder and is never renamed per-library. Folders a user adds alongside it are free-form names and are equally not the tool's to rename — only entries actually containing the old library name are migrated.
**Why:** `Examples/CMakeLists.txt` globs every subdirectory and `add_subdirectory`s it, so example folders build under any name. Renaming on a shared prefix (`RFID_FullDemoApp` vs library `RFID_Driver`) would be guessing at user intent.
**Consequences:** Do not "fix" the rename pass to match partial prefixes or to special-case example folders. The reporter's manual rename was cosmetic preference, not a build requirement.

### 2026-09-04 — A rename rewrites the old namespace inside USER_SECTIONs too
**Context:** The new rename regression test caught a third defect: after renaming, `<OldName>::` namespace uses survive in `examples/` and `unittests/` (`Foo_Driver::Profiler::start()`), so the renamed project does not compile. `Utilities::replaceAllIfLineContains` works line by line with no USER_SECTION awareness, so the tool cannot rewrite the template's lines without also rewriting the user's. Three options were put to the user: rewrite only outside USER_SECTIONs and report what remains inside; rewrite everywhere; rewrite nothing and report.
**Decision:** User chose **rewrite everywhere, including inside USER_SECTION blocks**. A rename produces a project that compiles, with no manual fix-up step and no post-rename report.
**Why:** A rename that leaves a non-compiling project is a half-done rename. The user judged a working build worth more than the USER_SECTION guarantee in this one case.
**Consequences:** `USER_SECTION` no longer strictly means "the tool will never touch this" — a library rename is an exception. The replacement is filtered to lines containing `::` or `namespace` rather than using the empty `{}` filter the default-placeholder namespace entry carries, because an unfiltered replacement is a known live defect (TASK-006 rewrote a literal token mentioned in a comment). Do not widen it to `{}`. If the rename is ever reported as having mangled prose inside a USER_SECTION, narrowing this filter is the first place to look.
