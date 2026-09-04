# UNIT_TEST_TASKS

Test suite: **KROIA UnitTest** (`https://github.com/KROIA/UnitTest.git`), fetched by `unittests/UnitTest.cmake`. `COMPILE_UNITTESTS` is `ON` by default in the root `CMakeLists.txt` since 2026-09-04, so a plain `build.bat` builds the suites.

Run: `installation/bin/<SuiteName>.exe` (also lands in `build/Debug/`). Exit code 0 = pass.

**GUI tests.** The UnitTest library ships a widget-driving module (`UnitTest::Gui`, `core/inc/gui/UnitTest_Gui.h` in the dependency source) for clicking and typing into real widgets without pixel coordinates — widgets are addressed by `objectName` or visible text. It is behind `UNITTEST_GUI_ENABLED` and the library build compiles it to nothing, so a suite that wants it must, in its own `CMakeLists.txt`: include the dependency's `core/inc/gui`, add `core/src/gui/UnitTest_Gui.cpp` to `ADDITONAL_SOURCES` (note the upstream misspelling — `exampleMaster` expects that name), define `UNITTEST_GUI_ENABLED`, and pull in the `Widgets` Qt module. See `unittests/ExportRenameTest/CMakeLists.txt` for a working copy. Note `unittest_SOURCE_DIR` does not escape the `dep()` function scope; `${FETCHCONTENT_BASE_DIR}/unittest-src` is the path that actually resolves.

**Conventions established by the first suite:**
- A test whose prerequisite is missing (no template downloaded, no GUI session) must `TEST_MESSAGE` and return — **skip, never fail**. A machine without the template must not go red.
- Any test that writes to disk needs an RAII scope guard for cleanup. `TEST_FAIL` returns out of the test function, so cleanup written at the end is skipped on exactly the runs that fail. Wipe at the start too, so a previous hard crash cannot poison the next run.
- Temp trees go under `QDir::tempPath()`, not the app-data tree the installed tool uses.
- **A test that builds a generated library must force a CMake reconfigure first.** The template globs its sources (`GLOB_FILES(H_FILES *.h)` in `core/CMakeLists.txt`), and CMake caches glob results at configure time. After an export, an upgrade or a rename, the cached list still names files that no longer exist — an incremental `cmake --build` then either fails on missing inputs or silently builds the wrong file set, and the test result describes the stale cache rather than the change under test. Never "optimise" a compile step into an incremental rebuild.

## Done

### UT-001 — Export/rename round-trip and profile-define sync
- **Target:** `ProjectExporter::migratePreviousGeneration` / `replaceTemplateCodePlaceholders` / `replaceTemplateVariablesIn_cmakePresets`; `CMAKE_settings::autosetLibProfileDefine`; `ProjectSettingsDialog` short-define slot
- **Suite:** `unittests/ExportRenameTest/`
- **Cases:** creates a library in a temp folder, seeds a marker inside a `USER_SECTION`, re-exports under a new name; asserts the marker reaches the renamed file, no `*OldName*` file or directory survives, no old name outside `USER_SECTION`s, the profiling token agrees across `CMakeLists.txt` / `CMakePresets.json` / `<name>_debug.h`, and exactly one `*_PROFILING` cache variable per `-Profile` preset. GUI case drives the real settings dialog and asserts the profile define follows a hand-typed short define.
- **Compile verification:** the generated project is built on both sides of the rename via its own `build.bat`. The pre-rename build exists to leave a configured CMake cache carrying the old file names; the post-rename build has to survive it. The reconfigure is proven by its *effect*, not by trusting a mechanism — the post-rename build output must contain `Bar_Driver_debug.cpp` and must not contain `Foo_Driver_debug.cpp`.
- **Measured:** 10 s pre-rename build, 5 s post-rename, 17.6 s whole suite, 596 MB peak temp tree (dependency cache + two build trees), removed afterwards. Needs network on a cold run.
- **Scan scope:** `build/` and `installation/` are excluded from the "no old name survives" scan. Compile output legitimately keeps whatever name it was built under until the next `build.bat clean` — see ISSUE-004.
- **Status:** done 2026-09-04 — found a third defect (old namespace surviving a rename) that was fixed as part of TASK-011. All six TASK-011 acceptance criteria now covered.

Test layout convention (mirrors KROIA template's `unitTests/ExampleTest/`):
```
unittests/
└── <SuiteName>/
    ├── main.cpp
    ├── tests.h
    └── TST_<thing>.h
```

## Pending
_None yet. Run the **unit-test** agent to scan coverage and seed tasks._

## Task entry template
```
### UT-<id> — <title>
- **Target:** <file/class under test, e.g. core/src/Utilities.cpp::readUserSections>
- **Suite:** unittests/<SuiteName>/
- **Cases:** <bullet list of cases / edge conditions>
- **Status:** pending | in-progress | done
```
