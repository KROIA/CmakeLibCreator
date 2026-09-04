# Bug report — renaming an existing library leaves the old library behind (and breaks the build)

**Date:** 2026-09-04
**Tool:** CmakeLibCreator (`C:\Users\KRIA\Documents\Visual Studio 2022\Projects\CmakeLibCreator`)
**Template:** `C:\Users\KRIA\Documents\Visual Studio 2022\Projects\QT_cmake_library_template` (v1.7.2)
**Affected project:** `C:\Users\KRIA\Documents\Projekte\RFID Project\RFID_Driver`
**Operation:** re-export of an existing project with `LIBRARY_NAME` changed `RFID_Driver` → `DTI801_Driver`
**Severity:** high — the project does not compile after the rename, and the cause is not obvious to the user.

---

## Summary

Re-exporting an existing project under a new library name produces the new files but **does not remove
or rename the old ones**, and does not rewrite name-derived tokens that the previous export wrote.
The result is a project that contains two complete copies of the library infrastructure and fails to
build. A second, independent defect makes the profiling define diverge between `CMakeLists.txt` and
`CMakePresets.json`.

The common root cause of #1 and #2 is that the export path searches for the **default template
placeholder** (`"LibraryName"`, `"LIBRARY_NAME_SHORT"`, …) rather than the **values the project
currently uses**. In a pristine template those are the same string, so first export works. In a
re-export they are not, so every "find and replace the old token" step silently matches nothing.

`ProjectSettings` already models this correctly — it has both `m_defaultPlaceholder` and
`m_loadedPlaceholder`, and `getLoadedPlaceholder()` is public. The export code just doesn't use it.

---

## Bug 1 — Old `<OldName>_*.h` / `.cpp` files are neither renamed nor deleted

### What happened

After the rename, `core/inc/` and `core/src/` contained **both** generations:

```
core/inc/  DTI801_Driver.h        RFID_Driver.h
           DTI801_Driver_base.h   RFID_Driver_base.h
           DTI801_Driver_debug.h  RFID_Driver_debug.h
           DTI801_Driver_global.h RFID_Driver_global.h
           DTI801_Driver_info.h   RFID_Driver_info.h
           DTI801_Driver_meta.h.in RFID_Driver_meta.h.in
core/src/  DTI801_Driver_debug.cpp RFID_Driver_debug.cpp
           DTI801_Driver_info.cpp  RFID_Driver_info.cpp
```

This is not merely untidy — **it breaks the build**, because `core/CMakeLists.txt:33-35` globs sources:

```cmake
GLOB_FILES(H_FILES *.h)
GLOB_FILES(CPP_FILES *.cpp)
```

so the stale files are compiled into the new target. Two concrete failures:

- `RFID_Driver_info.h` includes `RFID_Driver_meta.h`, but `core/CMakeLists.txt:50-52` only configures
  `${LIBRARY_NAME}_meta.h.in` → only `DTI801_Driver_meta.h` is generated. Missing header, hard error.
- `RFID_Driver_global.h` defines `RFID_DRIVER_API` as `__declspec(dllimport)`, because its guard
  `#if defined(RFID_DRIVER_LIB)` is now false (`LIB_DEFINE` is `DTI801_DRIVER_LIB`). The stale
  `.cpp` files then *define* symbols declared `dllimport`.

### Cause

`ProjectExporter::replaceTemplateFileNames()` — `core/src/ProjectExporter.cpp:410-441`:

```cpp
QString targetFileNameContains = settings.getDefaultPlaceholder().Library_Name;   // line 414
QString libraryName = settings.getCMAKE_settings().libraryName;
if (targetFileNameContains == libraryName)
    return true; // nothing to do
...
if (fileName.contains(targetFileNameContains))        // line 427
    ... QFile::rename(file, newFileName);
```

`s_defaultPlaceholder.Library_Name` is the literal `"LibraryName"`
(`core/src/ProjectSettings.cpp:7-13`). In an already-exported project no file is named
`LibraryName*`, so `fileName.contains(...)` is false for every file and **zero files are renamed**.
The debug line at `ProjectExporter.cpp:439` prints `Renamed 0 files`. The new files are then written
alongside the old ones.

### Suggested fix

Use the loaded placeholder / previous library name as the search token, and fall back to the default
only when exporting a fresh template:

```cpp
// the name the project currently uses on disk, not the template placeholder
QString oldName = settings.getLoadedPlaceholder().Library_Name;   // or previous cmakeSettings.libraryName
if (oldName.isEmpty()) oldName = settings.getDefaultPlaceholder().Library_Name;
```

Two further gaps in the same function while you are in there:

- It only scans `.h` and `.cpp` (`ProjectExporter.cpp:419-421`). **`.h.in` is missed** — hence the
  orphaned `RFID_Driver_meta.h.in`. Include `.in`, `.inl`, `.c`, `.hpp`.
- Directories are not renamed at all. `examples/RFID_FullDemoApp/` kept the old name and had to be
  renamed by hand.

A cheap safety net regardless of the above: after a successful rename, warn (or offer to delete) any
remaining file whose name matches `<oldName>_*` — a rename that leaves a compilable duplicate of the
whole library is worse than one that stops and asks.

---

## Bug 2 — Stale name-derived entries accumulate in `CMakePresets.json`

### What happened

After renaming `RFID_Driver` → `DTI801_Driver`, both profiling presets carried three entries:

```json
"cacheVariables": {
    "CMAKE_BUILD_TYPE": "Debug",
    "DTI801_PROFILING": "1",
    "DTID_PROFILING": "1",
    "RFIDD_PROFILING": "1"
}
```

`RFIDD_PROFILING` is the previous export's value and is now dead. (`DTID_PROFILING` is Bug 3.)

### Cause

`ProjectExporter::replaceTemplateVariablesIn_cmakePresets()` — `ProjectExporter.cpp:550-606`. The
replacement table at lines 573-579 is keyed on `settings.getDefaultPlaceholder()`:

```cpp
const ProjectSettings::Placeholder defaults = settings.getDefaultPlaceholder();  // line 567
const QVector<Replacement> replacements{
    { defaults.LIBRARY__NAME_SHORT, cmakeSettings.lib_short_define },
    ...
```

The line in the existing file reads `"RFIDD_PROFILING"`, which contains none of the default
placeholder tokens, so `line.contains(r.from)` at line 588 is false and the entry is left untouched.
The new entry is added, the old one stays.

Note lines 671-691 of the same file: `loadedPlaceholders` **is** fetched at line 672, and the
replacement entries that would use it (lines 687-691) are commented out. That looks like the
intended fix, unfinished.

### Suggested fix

Drive the replacement table from `getLoadedPlaceholder()` (falling back to the defaults), so a
re-export rewrites the tokens the file actually contains.

---

## Bug 3 — `LIB_PROFILE_DEFINE` and the presets' profiling define are generated from different sources

This one is independent of renaming and will bite on any project whose short define is not the
initials of its library name.

### What happened

```
CMakeLists.txt:42   set(LIB_PROFILE_DEFINE DTID_PROFILING)   # <LIB_PROFILE_DEFINE>
CMakePresets.json   "DTI801_PROFILING": "1"
DTI801_Driver_debug.h:55   #ifdef DTI801_PROFILING
```

The generated headers guard on `DTI801_PROFILING`, but the build system passes `DTID_PROFILING`
(`core/CMakeLists.txt:131` and `cmake/ExampleMaster.cmake:118` both consume `LIB_PROFILE_DEFINE`).
Net effect: **the `-Profile` presets configure and build successfully, but easy_profiler is silently
disabled** — every `*_PROFILING_*` macro expands to nothing. Silent, not a build error.

### Cause

The template defines both from one token — `CMakePresets.json:52,61` and `CMakeLists.txt:42` in the
template both say `LIBRARY_NAME_SHORT_PROFILING`, i.e. both are meant to be
`<lib_short_define>_PROFILING`. The exporter uses two different sources:

- presets: `{ defaults.LIBRARY__NAME_SHORT, cmakeSettings.lib_short_define }`
  (`ProjectExporter.cpp:574`) → `DTI801` → `DTI801_PROFILING` ✔
- CMakeLists: `replaceCmakeVariable(fileContent, "LIB_PROFILE_DEFINE", cmakeSettings.lib_profile_define)`
  (`ProjectExporter.cpp:463`) → whatever `lib_profile_define` holds ✘

and `CMAKE_settings::autosetLibProfileDefine()` (`ProjectSettings.cpp:200-218`) derives that value
from the **library name's** capitals, not from `lib_short_define`:

```cpp
for (int i = 0; i < libraryName.size(); i++)
    if (libraryName[i] >= 'A' && libraryName[i] <= 'Z')
        shortName += libraryName[i];
lib_profile_define = shortName + "_PROFILING";
```

`"DTI801_Driver"` → capitals `D,T,I,D` → `DTID_PROFILING`, which disagrees with the user-set short
define `DTI801`. (For the previous name `"RFID_Driver"` → `R,F,I,D,D` → `RFIDD_PROFILING`, which is
why that value appears in the history.)

### Suggested fix

Derive it from the short define, so the two can never diverge:

```cpp
lib_profile_define = lib_short_define + "_PROFILING";
```

and re-run it whenever `lib_short_define` changes. Alternatively drop `lib_profile_define` as a
stored field and substitute `LIB_PROFILE_DEFINE` with `lib_short_define + "_PROFILING"` at
`ProjectExporter.cpp:463`.

---

## Bug 4 (minor) — copy constructor copies the loaded placeholder into the default

`core/src/ProjectSettings.cpp:24-26`:

```cpp
ProjectSettings::ProjectSettings(const ProjectSettings& other)
    : ...
    , m_loadedPlaceholder(other.m_loadedPlaceholder)
    , m_defaultPlaceholder(other.m_loadedPlaceholder)   // <-- should be other.m_defaultPlaceholder
```

Harmless today because both are initialised to `s_defaultPlaceholder` and the loaded one is rarely
set — but it will corrupt exactly the distinction the fixes above depend on. Worth fixing first.

---

## Files the user had to repair by hand

Reported by the project side after the rename; all of these should be handled by the tool.

| # | What | Where | Consequence if missed |
|---|---|---|---|
| 1 | Delete 8 stale `RFID_Driver_*.h/.cpp/.h.in` | `core/inc/`, `core/src/` | build fails (missing `_meta.h`, dllimport/definition clash) |
| 2 | Fix `LIB_PROFILE_DEFINE` `DTID_PROFILING` → `DTI801_PROFILING` | `CMakeLists.txt:42` | profiling silently disabled |
| 3 | Remove dead `DTID_PROFILING`, `RFIDD_PROFILING` cache vars | `CMakePresets.json` (both `-Profile` presets) | clutter; misleading |
| 4 | Rename `examples/RFID_FullDemoApp/` → `examples/DTI801_FullDemoApp/` | `examples/` | cosmetic (folder is globbed) |
| 5 | Update `#include "RFID_Driver.h"` and `RFID_Driver::` namespace uses | `examples/*/src/main.cpp`, `unittests/ExampleTest/main.cpp` | build fails once the stale headers are deleted |
| 6 | Delete stale build artifacts and preset caches | `build/Debug`, `build/Release`, `build/x64-*`, `installation/` | old `RFID_Driver-d.dll` etc. ship alongside the new ones |

Item 5 is arguably **not** a tool bug — those files live in `USER_SECTION` territory and the tool is
right not to rewrite user code. But it is a foreseeable consequence of a rename, so a post-rename
report listing every remaining occurrence of the old name outside tool-owned regions would save the
user the manual grep. Item 6 likewise: the tool need not delete build output, but it could say so.

After applying 1-6 by hand, `build.bat` produced a clean `Build successful` for both `x64-Debug` and
`x64-Release`, and `ExampleTest.exe` reported `"All Tests" Testresult: PASS`. So the generated code
itself is fine — the defect is confined to the re-export/rename path.

---

## Suggested regression test

Export the template as `Foo_Driver`, build it, then re-export the same directory as `Bar_Driver` and
assert:

1. no file or directory under the project matches `*Foo_Driver*`;
2. `grep -r "Foo_Driver\|FD_PROFILING"` over the project (excluding `build/`) returns nothing outside
   `USER_SECTION` blocks;
3. the token in `set(LIB_PROFILE_DEFINE ...)` equals the profiling key in `CMakePresets.json` and
   equals the `#ifdef` in `<name>_debug.h`;
4. a fresh configure+build of `x64-Debug` and `x64-Release-Profile` succeeds.

Point 3 is the one that catches Bug 3 without a human noticing the profiler went quiet.
