# CODING_STYLE — CmakeLibCreator

All subagents inherit this. Follow it strictly.

## C++

- **Standard:** C++23, MSVC. Headers in `core/inc/`, sources in `core/src/`.
- **Namespace:** everything in `namespace CLC { ... }`.
- **Includes:** project headers with quotes (`"ProjectExporter.h"`); third-party / Qt / std with angle brackets.
- **Naming:**
  - Classes / types: `PascalCase` (e.g. `ProjectExporter`, `LibrarySettings`).
  - Methods / functions: `camelCase` (e.g. `exportProject`, `readUserSections`).
  - Member variables: `m_camelCase` (e.g. `m_templateSourcePath`).
  - Static members: `s_camelCase`.
  - Constants / macros: `UPPER_SNAKE_CASE`.
- **Logging:** use `Log::LogObject` (see `core/inc/Logging.h`) — never `std::cout` / `qDebug` directly in library code.
- **Error handling:** prefer return-value reporting + `Log::LogObject::logError(...)` over exceptions; no `try/catch` introductions unless the caller already uses them.
- **Qt patterns:** UI widgets live under `core/inc/ui/` and `core/src/ui/`. Parent ownership applies — do not `delete` Qt children manually.
- **No new dependencies** without an entry in `DECISIONS.md` and a corresponding `dependencies/*.cmake` file following the template convention (`downloadStandardLibrary` / `downloadExternalLibrary`).

## CMake

- **CMake ≥ 3.20.** Target-based commands (`target_link_libraries`, `target_include_directories`, …); avoid global `link_libraries` outside the existing root `link_directories` call.
- **Quoted paths everywhere:** `"${CMAKE_SOURCE_DIR}/foo"`. The repo path contains a space.
- **Markers are sacred:**
  - `# <AUTO_REPLACED>` — variable rewritten by the app. Don't rename.
  - `## USER_SECTION_START N` / `## USER_SECTION_END` — fixed schema. Never add new pairs in template-managed files; only edit *inside* existing slots.
- **Idempotency:** prefer `set_if_not_defined(...)` for tunables (defined in `cmake/utility.cmake`).

## Comments

- Write **why**, not **what**. Identifiers are documentation.
- No multi-paragraph block comments. One short line max for non-obvious intent.
- No "removed X" or "added for Y" comments — that's what git history is for.

## Tests

- Framework: **KROIA UnitTest** (fetched via `unittests/UnitTest.cmake`).
- Test files live under `unittests/<TestSuiteName>/` with a `main.cpp`, `tests.h`, and `TST_<thing>.h` per the template's `Examples/LibraryExample` and KROIA UnitTest reference.
- Build flag: `-DCOMPILE_UNITTESTS=ON`.
- A test suite must compile and pass before its task is `done`.

## Files & paths in tooling

- Always quote shell paths. Use forward slashes.
- Use `/dev/null`, not `NUL` (this is bash on Windows).
- Don't write to `.env`, `credentials.*`, key/cert files. Ever.
