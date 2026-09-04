# ISSUES

Issues found by code review. Sorted by priority within each section.

**Priority rubric:**
- **critical** — data loss, security vulnerability, production-down
- **high** — blocks release, blocks dependent work
- **medium** — meaningful defect, no blocker
- **low** — polish, nice-to-have

## Hotfix lane

### ISSUE-001 — Re-export with a changed library name orphans the old files *and silently drops the user's code into them*
- **Priority:** critical
- **Description:** The re-export/upgrade path never renames the previous generation of name-derived files, and — worse — writes the user's `USER_SECTION` content back into those orphaned files instead of into the new ones. Traced end to end on the upgrade branch (`ProjectExporter.cpp:111-137`; the project directory is **not** deleted there — the full delete at `:88-91` is the new-project branch only):
  1. `:120-123` `copyTemplateSourceFiles` writes `<NewName>_*.h/.cpp` containing **pristine template content**. It renames during the copy — `:287-290` substitutes `LibraryName` → target name as it writes, with `overrideMode=true`.
  2. Nothing ever touches the old `<OldName>_*` files.
  3. `:126` `replaceTemplateUserSectionsIn_codeFiles` re-applies user sections keyed by **absolute file path** (`ProjectSettings.h:93-102`, captured in `readCodeUserSections` at `:1049-1080`). The stored key still carries the old name. The old file still exists, so the `QFile::exists` guard at `:738-739` passes and `Utilities::replaceUserCodeSections` (`Utilities.cpp:756-758`) reads, patches and re-saves **the old file**. Same shape for CMake sections at `:136` / `:706-726`.
  4. `:141` `replaceTemplateFileNames` no-ops (see below). `:142-154` token replacement runs.
- **Net effect:** the new library ships with **empty `USER_SECTION`s**, and every line the user wrote sits in a stale file. `core/CMakeLists.txt:33-35` globs `*.h`/`*.cpp`, so the stale files are compiled into the new target and the build breaks loudly:
  - `<OldName>_info.h` includes `<OldName>_meta.h`, but only `${LIBRARY_NAME}_meta.h.in` is configured (`core/CMakeLists.txt:50-52`) — missing header, hard error.
  - `<OldName>_global.h` resolves `<OLDNAME>_API` to `__declspec(dllimport)` because its guard `#if defined(<OLDNAME>_LIB)` is now false; the stale `.cpp` files then define symbols declared `dllimport`.
  The loud build failure is what saved the reporter — their code was still being compiled from the stale files. **Deleting the stale files as "cleanup", which is what the bug report asks for, would lose the user's `USER_SECTION` code with no error at all.** Any fix must reorder, not just delete.
- **Correction to the bug report's diagnosis:** the report blames `replaceTemplateFileNames()` using the default placeholder at `:414`. That is a real mismatch but the wrong lever — the function is **vestigial**. `copyTemplateSourceFiles` already renamed the fresh files during the copy, so by `:141` nothing on disk contains `LibraryName` and the function has no work to do on either generation. Patching `:414` to use the loaded placeholder fixes nothing on its own.
- **Correction to the bug report's `.h.in` claim:** the report says `.h.in` is missed because only `.h`/`.cpp` are scanned. Not so — `Utilities::getFilesInFolderRecursive` matches with `QString::contains`, not a suffix compare (`Utilities.cpp:161-179`, `:174`), so the `.h` filter already matches `.h.in`. The orphaned `<OldName>_meta.h.in` is explained by the function being a no-op, not by the filter. (The `contains` matching is loose enough to be worth a look on its own — `".h"` also matches `something.header`.)
- **Evidence:** bug report `.claude/bugreports/2026-09-04-rename-leaves-old-library-files.md`, real project `RFID_Driver` → `DTI801_Driver`. Confirmed by source trace 2026-09-04.
- **Possible solution:** see TASK-011. Add one step early in the upgrade path, before any copying, that renames on-disk `<OldName>_*` files **and directories** to `<NewName>_*` and rewrites the stored section keys in the same pass. Everything downstream then behaves exactly as it does for a non-renamed project. Old name comes from `settings.getLoadedPlaceholder().Library_Name` (populated at `:174-182`, confirmed to survive the settings-dialog round trip).
- **Found by:** user bug report 2026-09-04, PM verification + source trace 2026-09-04

### ISSUE-002 — `LIB_PROFILE_DEFINE` diverges from the presets' profiling key; profiling silently disabled
- **Priority:** critical
- **Description:** Independent of renaming. The template drives both `set(LIB_PROFILE_DEFINE ...)` (`CMakeLists.txt:42`) and the `-Profile` preset cache variable (`CMakePresets.json:52,61`) from one token, `LIBRARY_NAME_SHORT_PROFILING` — both are meant to be `<lib_short_define>_PROFILING`. The exporter uses two different sources:
  - presets: `{ defaults.LIBRARY__NAME_SHORT, cmakeSettings.lib_short_define }` (`ProjectExporter.cpp:574`)
  - CMakeLists: `replaceCmakeVariable(..., "LIB_PROFILE_DEFINE", cmakeSettings.lib_profile_define)` (`ProjectExporter.cpp:463`)
  and `CMAKE_settings::autosetLibProfileDefine()` (`ProjectSettings.cpp:198-218`) derives its value from the **capitals of the library name**, not from `lib_short_define`: `"DTI801_Driver"` → `D,T,I,D` → `DTID_PROFILING`, versus the user-set short define `DTI801` → `DTI801_PROFILING`.
- **Consequence:** the generated headers guard on `<SHORT>_PROFILING` while the build system passes the capitals-derived token. The `-Profile` presets configure and build fine, but easy_profiler is silently off — every `*_PROFILING_*` macro expands to nothing. No error, no warning.
- **Possible solution:** derive it from the short define so the two cannot diverge — `lib_profile_define = lib_short_define + "_PROFILING"` — and re-run it whenever `lib_short_define` changes (`ProjectSettingsDialog.cpp:299` already calls the autoset). Alternatively drop `lib_profile_define` as a stored field and substitute `LIB_PROFILE_DEFINE` with `lib_short_define + "_PROFILING"` at `ProjectExporter.cpp:463`. Note `ProjectExporter.cpp:784` reads the field back out of an existing project, so a stored field cannot simply be deleted without touching the read path.
- **Found by:** user bug report 2026-09-04, PM verification 2026-09-04

---

## Open issues

### ISSUE-003 — Stale name-derived cache variables accumulate in `CMakePresets.json`
- **Priority:** medium
- **Description:** Same root cause as ISSUE-001. `replaceTemplateVariablesIn_cmakePresets()` builds its replacement table from `settings.getDefaultPlaceholder()` (`ProjectExporter.cpp:567,573-579`). The line already in the file reads `<OldShort>_PROFILING`, which contains none of the default placeholder tokens, so `line.contains(r.from)` at `:588` is false and the dead entry survives. The new entry is added next to it. Observed after one rename:
  ```json
  "cacheVariables": {
      "CMAKE_BUILD_TYPE": "Debug",
      "DTI801_PROFILING": "1",
      "DTID_PROFILING": "1",
      "RFIDD_PROFILING": "1"
  }
  ```
  (`DTID_PROFILING` is ISSUE-002, `RFIDD_PROFILING` is the previous export's dead value.)
- **Possible solution:** drive the replacement table from `getLoadedPlaceholder()` with a fallback to the defaults, so a re-export rewrites the tokens the file actually contains. `replaceTemplateCodePlaceholders()` already fetches `loadedPlaceholders` at `ProjectExporter.cpp:672` and has the matching replacement entries commented out at `:687-691` — the intended fix, left unfinished.
- **Found by:** user bug report 2026-09-04, PM verification 2026-09-04

### ISSUE-004 — Renaming leaves the old name in user-owned files and in build artifacts, with no report
- **Priority:** low
- **Description:** After a rename the old library name survives in `USER_SECTION` territory (`examples/*/src/main.cpp`, `unittests/ExampleTest/main.cpp` — `#include "<OldName>.h"`, `<OldName>::` namespace uses) and in build output (`build/Debug`, `build/Release`, `build/x64-*`, `installation/`, old `<OldName>-d.dll`). The tool is right not to rewrite user code and need not delete build output, but it says nothing, so the user has to find these by hand.
- **Possible solution:** after a rename, emit a report listing every remaining occurrence of the old name outside tool-owned regions, plus a note that build/ and installation/ hold stale artifacts. Report only — no automatic edits to user code, no automatic deletion of build output.
- **Narrowed 2026-09-04:** the user-code half of this is now moot — a rename rewrites old namespace references everywhere, including inside `USER_SECTION`s (see `DECISIONS.md` 2026-09-04), so the renamed project compiles without manual edits. What remains is only the **stale build artifacts** half: after a rename, `build/` and `installation/` still hold binaries under the old name until the next `build.bat clean`. `unittests/ExportRenameTest` excludes both directories from its "no old name survives" scan for exactly this reason, so the suite documents the gap rather than hiding it.
- **Found by:** user bug report 2026-09-04

### Note — bug report item "Bug 4" (copy constructor) is not a live defect
The report flags `ProjectSettings.cpp:24-26` copying `other.m_loadedPlaceholder` into `m_defaultPlaceholder`. That copy constructor is commented out (`core/src/ProjectSettings.cpp:22-32`); `operator=` at `:36-41` copies both fields correctly. Dead code only — delete the block opportunistically, no behavior change.

---

## Issue entry template

```
### ISSUE-<id> — <title>
- **Priority:** critical | high | medium | low
- **Description:** <what's wrong, where, evidence>
- **Possible solution:** <one or more suggestions>
- **Found by:** <agent name + date>
```
