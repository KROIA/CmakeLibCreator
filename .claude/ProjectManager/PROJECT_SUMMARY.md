# PROJECT_SUMMARY — CmakeLibCreator

## What this project is

**CmakeLibCreator** is a Qt desktop + CLI application that automates the creation and ongoing upgrade of CMake-based C++ library projects derived from a single template repository.

- **Template source of truth:** `https://github.com/KROIA/QT_cmake_library_template.git`
- **App version:** 1.6.0 — supports template version **1.6.1**
- **Author:** Alex Krieg (alexkrieg@gmx.ch)
- **Repo branch policy:** all work happens on `main`.

The app's entire job: **download template → copy files → substitute placeholders → preserve user code sections (USER_SECTION blocks)**.

For deep technical reference (placeholder system, USER_SECTION rules, key files, conventions, gotchas), read `AI_Knowledge.md` at the project root — it is authoritative and kept up to date.

## Tech stack

| Item | Value |
|---|---|
| Language | C++23 |
| GUI | Qt 5.15.x (Core, Widgets, Gui) — Qt6-style APIs in code |
| Build | CMake ≥ 3.20 |
| Compiler | MSVC (Visual Studio 2022) |
| Test framework | KROIA UnitTest library — `https://github.com/KROIA/UnitTest.git` (fetched via `unittests/UnitTest.cmake`) |
| Deployment | `windeployqt.exe` post-build |
| Output | `./build/CmakeLibraryCreator.exe`, `./installation/` |

## Repo layout

```
CmakeLibCreator/
├── CMakeLists.txt              Root config — IS ITSELF a generated-from-template file
├── CmakeLibCreator/            App shell — main.cpp, AppIcon.rc/ico
├── core/inc/, core/src/        Library implementation + ui/ subfolder
├── cmake/                      utility.cmake, QtLocator.cmake (from template)
├── dependencies/               .cmake files for fetched deps (from template)
├── unittests/                  UnitTest.cmake (fetches KROIA UnitTest); test sources to be added
├── Documentation/              User-facing .md docs + screenshots
├── installation/               Deploy target
├── build/                      CMake build dir + runtime template downloads
├── CHANGELOG.md, changelogs/   Per-version release notes
└── .claude/                    Project management
```

## Required subagents (templates in `.claude/ProjectManager/agents/`)

- **api-docs** — keeps API reference (headers in `core/inc/`) documented
- **user-docs** — maintains `Documentation/*.md` for end users
- **code-review** — Opus model, scans for bugs/flaws into `ISSUES.md`
- **unit-test** — adds tests under `unittests/` using KROIA UnitTest framework
- **dep-security-audit** — scans dependencies (`dependencies/*.cmake`) and flags risk
- **migration** — handles template version migrations (e.g. 1.6.1 → next)
- **performance** — profiles hot paths (parser, file I/O in `Utilities.cpp`, `ProjectExporter.cpp`)

## Key constraints (must-respect)

- **All paths quoted** — project root contains spaces (`Visual Studio 2022`).
- **Don't add new USER_SECTION_START/END markers** in template-managed files.
- **Don't rename `# <AUTO_REPLACED>` variables** — substitution matches by exact name.
- **Root `CMakeLists.txt` is template-managed** — only edit inside USER_SECTION slots.
- **Namespace** `CLC` for all C++ code.
- **Shell** is bash on Windows — Unix paths and `/dev/null`, not `NUL`.

## Versioning

The library version lives in root `CMakeLists.txt` line 36:
```cmake
set(LIBRARY_VERSION "1.6.0")  # <AUTO_REPLACED>
```
Pre-commit hook checks this; on bump → release workflow (see setup spec §7).
