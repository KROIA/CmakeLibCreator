# GLOSSARY — Domain terms

| Term | Meaning |
|---|---|
| **Template** | The KROIA `QT_cmake_library_template` repo — source of truth for the structure of every library this app generates. |
| **Template version** | Version of the template the app is compatible with (currently 1.6.1). Distinct from app version. |
| **USER_SECTION** | Indexed `START`/`END` block in a template-managed file. Content inside is preserved across upgrades; content outside is overwritten. Indices are fixed by the template. |
| **AUTO_REPLACED** | Comment-tag (`# <AUTO_REPLACED>`) marking a CMake variable that the app rewrites by exact name during export. |
| **Placeholder** | Literal string token (`LibraryName`, `LIBRARY_NAME_API`, `LIBRARY_NAME_SHORT`, etc.) substituted in file contents and filenames during export. |
| **`Placeholder` struct** | In-memory record of one placeholder; the project keeps both `s_defaultPlaceholder` (template literals) and `loadedPlaceholder` (what was read from an existing project). |
| **`ExportSettings`** | Toggle bag controlling what `ProjectExporter::exportProject` does (copy all, replace cmake, replace code, replace variables, replace code placeholders). |
| **Export** | The act of writing a (new or upgraded) library project to disk. Implemented in `core/src/ProjectExporter.cpp`. |
| **Upgrade** | Re-applying the latest template to an existing library while preserving its USER_SECTIONs. |
| **CLI mode** | Invocation path triggered by `-u`/`--update`/`-h`/`--help`/`-v`/`--version` in argv; bypasses the GUI. Implemented in `CmakeLibCreator/main.cpp::runCli()`. |
| **Three library variants** | Every generated project produces `*_shared` (DLL), `*_static` (static lib), `*_static_profile` (static + easy_profiler). |
| **`*_AVAILABLE` macro** | Cache variable a dependency emits to signal its presence to consumers via `DEPENDENCY_NAME_MACRO`. |
| **`Resources` singleton** | Loads `settings.json` at startup; owns paths, git repo URL, branch names, recent project list. Init order: `loadSettings → loadDependencies → loadQTModules`. |
| **`Log::LogObject`** | Logging wrapper — the only sanctioned way to emit log output from library code. |
| **CLC** | C++ namespace housing all library code. |
| **Repository card** | One `RepositoryWidget` in the Repositories tab representing a single managed library project: group checkbox, status labels (dirty, HEAD, versions, build/test), and per-repo action buttons. |
| **Group actions** | Ribbon "Repositories" tab buttons that operate on all checkbox-selected, existing repositories, run strictly sequentially through the job queue. Contrast with the always-available individual card buttons. |
| **Job queue** | `RepositoryJobQueue` — a single worker thread that runs repository jobs (pull/push/commit/discard/update-template/build/clean/unittest/refresh) sequentially, with `QProcess` execution and `taskkill /T /F` cancellation of the whole process tree. |
| **`ProjectEntry`** | Persisted record `{ path, groupEnabled }` for one registered repository; stored as the object-array form of the `"projectPaths"` key in settings.json. |
