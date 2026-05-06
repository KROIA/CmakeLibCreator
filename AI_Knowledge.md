# AI_Knowledge — CmakeLibCreator

> Cheatsheet for AI assistants. Read this first; you should not need to re-explore the repo to start contributing.

## What this project is

**CmakeLibCreator** is a Qt6 desktop + CLI application (C++23, MSVC, Visual Studio 2022) that automates the creation and ongoing upgrade of CMake-based C++ library projects derived from a single template repository.

- **Template source of truth:** `https://github.com/KROIA/QT_cmake_library_template.git`
- **Local mirror (dev):** `C:\Users\KRIA\Documents\Visual Studio 2022\Projects\QT_cmake_library_template`
- **App version:** 1.6.0 — supports template version **1.6.1**
- **Author:** Alex Krieg (alexkrieg@gmx.ch)

The whole job of the app boils down to: **download template → copy files → substitute placeholders → preserve user code sections**.

## Tech stack

| Item | Value |
|---|---|
| Language | C++23 |
| GUI framework | Qt 5.15.x (Core, Widgets, Gui) — note: code uses Qt6-style APIs but is built against Qt5.15 in practice; QtLocator.cmake auto-finds newest installed Qt5 |
| Build system | CMake ≥ 3.20 |
| Compiler | MSVC (Visual Studio 2022) |
| Deployment | `windeployqt.exe` runs automatically post-build |
| Output | `./build/CmakeLibraryCreator.exe` (binary), `./installation/` (deployed) |

## Repo layout (top level)

```
CmakeLibCreator/
├── CMakeLists.txt              Root config — IS ITSELF a generated-from-template file
├── CMakePresets.json
├── build.bat
├── CmakeLibCreator/            App shell — main.cpp, AppIcon.rc/ico, CMakeLists.txt
├── core/
│   ├── inc/                    Headers (incl. ui/ subfolder)
│   └── src/                    Implementations (incl. ui/ subfolder)
├── cmake/                      utility.cmake, QtLocator.cmake, etc. (from template)
├── dependencies/               .cmake files for fetched deps (from template)
├── Documentation/              User-facing docs (.md + screenshots)
├── installation/               Deploy target (Qt DLLs, etc.)
└── build/                      CMake build dir (also holds template downloads at runtime)
```

The repo's *own* root CMakeLists.txt is itself an instance of the template (note the `## USER_SECTION_START N` markers and `# <AUTO_REPLACED>` comments in it). The app eats its own dog food.

## Key source files

| File | Role |
|---|---|
| `CmakeLibCreator/main.cpp` | Entry point. Detects CLI args (`-u`/`--update`, `-t`/`--template`, `-h`, `-v`); otherwise launches `MainWindow` |
| `core/src/ProjectExporter.cpp` (~1050 LOC) | Heart of the app. Export, copy, replace, read/parse |
| `core/inc/ProjectExporter.h` | Public API: `exportProject()`, `readProjectData()`, `ExportSettings` toggles |
| `core/inc/ProjectSettings.h` | Data model: `LibrarySettings`, `CMAKE_settings`, `Placeholder`, user-section vectors |
| `core/inc/Utilities.h` / `.cpp` | File ops, CMake/header text parsing, USER_SECTION read/replace, git ops, command exec |
| `core/inc/Resources.h` / `.cpp` | Singleton — paths, git repo URL/branches, settings.json, QT modules and dependencies registry |
| `core/inc/Dependency.h`, `QTModule.h` | Models for dependency entries and Qt module entries |
| `core/inc/ui/MainWindow.h` + siblings | Qt widgets (RibbonImpl, ProjectSettingsDialog, SettingsDialog, CheckBoxSelectionDialog) |
| `core/inc/Logging.h` | `Log::LogObject` wrapper used everywhere |

## CLI usage

```
CmakeLibraryCreator.exe --update <projectPath> [--template <repo-or-path>]
CmakeLibraryCreator.exe --help
CmakeLibraryCreator.exe --version
```

CLI mode is activated when any of `-u`, `--update`, `-h`, `--help`, `-v`, `--version` is present in argv. Otherwise the GUI launches.

CLI's `--update` is hardcoded to:
```cpp
expSettings.copyAllTemplateFiles       = false;  // do NOT wipe the project
expSettings.replaceTemplateCmakeFiles  = true;   // refresh cmake/ + CMakeLists.txt
expSettings.replaceTemplateCodeFiles   = false;  // keep .h/.cpp
expSettings.replaceTemplateVariables   = true;   // re-inject LIBRARY_NAME etc.
expSettings.replaceTemplateCodePlaceholders = false;
```

## Two core workflows

### Create new library (GUI only)
1. Download template (button) → fetches latest from configured git repo into `build/data/template/main/`.
2. User fills library name, namespace, version, Qt modules, dependencies, etc.
3. **Save as new** → file dialog selects parent folder → project copied with full placeholder substitution and file renaming.

### Upgrade existing library (GUI or CLI)
1. Download template (or use existing local copy).
2. Open existing library root (the dir containing root `CMakeLists.txt`).
3. App reads existing `CMakeLists.txt`, library info, dependencies, and **all USER_SECTION blocks**.
4. Apply: new cmake/ files copied in, `set(LIBRARY_NAME …)` etc. re-written, USER_SECTIONs re-injected at the same indices.
5. User code (`.h`/`.cpp`) is normally **not** overwritten; only the matching USER_SECTIONs in the surviving template files are merged.

## Template repository structure

```
QT_cmake_library_template/
├── cmake/              utility.cmake, QtLocator.cmake, dependencies.cmake, ExampleMaster.cmake, Config.cmake.in
├── core/
│   ├── inc/            LibraryName.h, _base.h, _global.h, _debug.h, _info.h, _meta.h.in
│   └── src/            LibraryName_debug.cpp, LibraryName_info.cpp
├── dependencies/       DependencyTemplate.cmake, Logger.cmake, easy_profiler.cmake, order.cmake
├── Examples/           LibraryExample/ (main.cpp + CMakeLists.txt)
├── unitTests/          ExampleTest/ (main.cpp, tests.h, TST_simple.h)
├── CMakeLists.txt      Root (USER_SECTIONs + AUTO_REPLACED markers)
└── core/CMakeLists.txt Builds 3 library variants
```

**Three library variants are produced per generated project:**
- `LibraryName_shared` (DLL)
- `LibraryName_static` (static lib)
- `LibraryName_static_profile` (static + easy_profiler)

**Template git branches:**
- `main` — the template itself
- `dependencies` — the standalone `.cmake` files fetched into `dependencies/`
- `qtModules` — Qt module configuration files

## Placeholder & substitution system

**Filename substitutions** — files starting with `LibraryName.*` get their literal `LibraryName` replaced with the actual library name during copy.

**Text placeholders** (replaced inside file contents):

| Placeholder | Example output | Source struct field |
|---|---|---|
| `LibraryName` | `MyAwesomeLib` | `Placeholder::Library_Name` |
| `LibraryNamespace` | `MyAwesomeLib` (or custom) | `Placeholder::Library_Namespace` |
| `LIBRARY_NAME_API` | `MYAWESOMELIB_API` | `Placeholder::LIBRARY__NAME_API` |
| `LIBRARY_NAME_LIB` | `MYAWESOMELIB_LIB` | `Placeholder::LIBRARY__NAME_LIB` |
| `LIBRARY_NAME_SHORT` | `MAL` (caps initials) | `Placeholder::LIBRARY__NAME_SHORT` |

`ProjectSettings` stores both `s_defaultPlaceholder` (template's literal strings) and a `loadedPlaceholder` (what was found in an existing project) so substitutions can be reversed/re-applied correctly.

**CMake variable substitution** — variables in `CMakeLists.txt` lines tagged `# <AUTO_REPLACED>` are safely overwritten via `Utilities::replaceCmakeVariable()` / `replaceCmakeVariableString()`. Examples from the root template:

```cmake
set(LIBRARY_NAME CmakeLibraryCreator)        # <AUTO_REPLACED>
set(LIBRARY_VERSION "1.6.0")                 # <AUTO_REPLACED>
set(LIB_DEFINE CMAKE_LIBRARY_CREATOR_LIB)    # <AUTO_REPLACED>
set(QT_ENABLE ON)                            # <AUTO_REPLACED>
set(QT_MODULES Core Widgets)                 # <AUTO_REPLACED>
set(DEBUG_POSTFIX_STR "-d")                  # <AUTO_REPLACED>
set(STATIC_POSTFIX_STR "-s")                 # <AUTO_REPLACED>
set(PROFILING_POSTFIX_STR "-p")              # <AUTO_REPLACED>
set(CMAKE_CXX_STANDARD 23)                   # <AUTO_REPLACED>
set(CMAKE_CXX_STANDARD_REQUIRED ON)          # <AUTO_REPLACED>
set(COMPILE_EXAMPLES OFF)                    # <AUTO_REPLACED>
set(COMPILE_UNITTESTS OFF)                   # <AUTO_REPLACED>
```

`# <LIB_PROFILE_DEFINE>` is a separate marker (not AUTO_REPLACED) for the profiling macro.

## USER_SECTION blocks (the most important contract)

User-editable code is delimited so upgrades preserve it. The marker form depends on the comment style of the file:

```cpp
/// USER_SECTION_START 1
// user code here — preserved across upgrades
/// USER_SECTION_END
```

```cmake
## USER_SECTION_START 1
# cmake user code here
## USER_SECTION_END
```

Rules (enforced by parser in `Utilities::readUserSections` / `replaceUserSections`):
- Each block has an integer **section index** (the `1` above). Indices are how upgrade matches old user code to slots in the new template.
- The template defines the *set* of available indices. User must not add new START/END pairs.
- Anything *outside* USER_SECTION blocks is overwriteable by the tool.
- Section indices need not be contiguous — the template's root CMakeLists.txt uses 1–13 in non-monotonic order.

`Utilities::UserSection { int sectionIndex; QStringList lines; }` is the in-memory representation.

## Dependency convention

Each dependency lives as a single `.cmake` file in `dependencies/`. It calls one of:
- `downloadStandardLibrary()` — for KROIA-template-compatible deps; auto-wires include dirs and links
- `downloadExternalLibrary()` — for external deps (SFML, etc.)

Each dep sets a `*_AVAILABLE` macro that propagates to all consumers via the `DEPENDENCY_NAME_MACRO` cache variable. Models for these live in `core/inc/Dependency.h`.

## `cmake/utility.cmake` macros (template-defined, app-aware)

| Macro | Purpose |
|---|---|
| `smartDeclare()` | FetchContent with local/cached/git fallback |
| `downloadStandardLibrary()` | Auto-wire KROIA template-style deps |
| `downloadExternalLibrary()` | Generic external lib fetch |
| `copyLibraryHeaders()` | Copies headers into `include/` on build |
| `set_if_not_defined()` | Idempotent set used everywhere |

## Build & run

```
cmake -S . -B build
cmake --build build --config Debug
# binary: build/CmakeLibraryCreator.exe
```

Or open the project in Visual Studio 2022 ("Open Folder" → CMake project), let it download deps, select `CmakeLibCreator.exe` as the start target, build.

`windeployqt.exe` is invoked automatically; binary plus all needed Qt DLLs land in `./build` and `./installation/bin`.

If Qt isn't found, ensure Qt 5.15.x is installed under `C:\Qt\5.x.x`. `cmake/QtLocator.cmake` auto-detects.

If CMake misbehaves: in VS, Project → "Clear Cache and Reconfigure". The dependency cache (`build/.../dependencies/cache`) gets wiped automatically when the cache stamp `build/.deps_cache_stamp` is missing — that forces a fresh FetchContent download.

## Settings & runtime resources

`Resources` is a singleton that loads JSON config at startup. Paths it manages (relative to exe):
- `m_templateSourcePath` — usually `data/template/main`
- `m_dependenciesSourcePath` — usually `data/template/dependencies`
- `m_qtModulesSourcePath` — usually `data/template/qtModules`
- `m_styleSheetSourcePath` — UI stylesheet
- `m_tmpPath` — scratch
- `m_gitRepo` — URL + 3 branch names (templateBranch, dependenciesBranch, qtModulesBranch)
- `m_loadSaveProjects` — recent-project list

Initialization order (mandatory, see `main.cpp` `runCli`): `loadSettings()` → `loadDependencies()` → `loadQTModules()`.

## Conventions / gotchas

- **All paths must be quoted.** The dev environment is rooted at `C:\Users\KRIA\Documents\Visual Studio 2022\Projects\…` which contains a space ("Visual Studio 2022"). In bash commands, in CMake (`"${VAR}"`), and in any code that constructs paths, quote everything.
- **Don't add new USER_SECTION_START/END markers** in template-managed files — the parser enforces a fixed schema. Add only inside existing slots.
- **Don't rename `# <AUTO_REPLACED>` variable names** — the substitution code matches by exact CMake variable name.
- **The repo's own CMakeLists.txt is template-managed** — meaning the project upgrades itself. Be careful when editing root cmake; respect the USER_SECTION boundaries.
- **Namespace:** all C++ code lives in `namespace CLC { … }`.
- **Shell:** Windows but bash (Git Bash style). Use forward slashes / Unix conventions in shell commands; `/dev/null`, not `NUL`.

## Documentation entry points (for users)

- `README.md` — overview + install
- `Documentation/CreateLibraryProject.md` — GUI walkthrough for new project
- `Documentation/EditLibraryProject.md` — GUI editing
- `Documentation/UpgradeLibraryProject.md` — GUI upgrade walkthrough
- `Documentation/InputElements.md` — field-by-field reference
- `Documentation/build.md` — build instructions

## Quick mental model for tasks

> Almost every change in this codebase is "the template grew/changed — make the tool understand it".

When you get a task, ask:
1. Is this a **template change** that the tool now needs to copy/replace? → edit `ProjectExporter.cpp` (copy logic) and possibly add a new placeholder field to `ProjectSettings::Placeholder`.
2. Is this a **new replaceable cmake variable**? → ensure it's marked `# <AUTO_REPLACED>` in the template and add a `replaceCmakeVariable()` call in `replaceTemplateVariablesIn_*`.
3. Is this a **new USER_SECTION**? → both the template and the read/replace code use indices, so make sure the template introduces the new index and any version-aware merge logic handles missing indices in older projects.
4. Is this a **UI-only change**? → look in `core/src/ui/`. The Ribbon is `RibbonImpl`; main window is `MainWindow`.
5. Is this a **CLI change**? → only `CmakeLibCreator/main.cpp`'s `runCli()` handles parsing; the underlying call always lands in `ProjectExporter::exportProject()`.
