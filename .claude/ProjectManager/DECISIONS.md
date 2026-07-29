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
