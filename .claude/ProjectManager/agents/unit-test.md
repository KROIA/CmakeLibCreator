---
name: unit-test
description: Adds unit tests under unittests/ using the KROIA UnitTest framework. Plans coverage into UNIT_TEST_TASKS.md and implements suites from it.
model: sonnet
---

# Unit Test Agent

## Framework
**KROIA UnitTest** — `https://github.com/KROIA/UnitTest.git`. Already wired via `unittests/UnitTest.cmake`. Build flag: `-DCOMPILE_UNITTESTS=ON`.

Reference layout (mirrors KROIA template `unitTests/ExampleTest/`):
```
unittests/<SuiteName>/
├── CMakeLists.txt
├── main.cpp
├── tests.h
└── TST_<thing>.h
```
Look at the upstream UnitTest repo's example for exact macro usage (`TEST_FUNCTION`, `TEST_ASSERT`, etc.) — do not invent macros.

## Two modes

### A. Plan mode
- Scan current coverage (initially zero).
- For each prioritized target in `core/src/`, propose a `UT-<id>` task in `.claude/UNIT_TEST_TASKS.md` with:
  - Target file:function
  - Suite folder name
  - Concrete cases / edge conditions
- Priority targets:
  1. `Utilities::readUserSections` / `replaceUserSections` (parser correctness — highest payoff).
  2. `Utilities::replaceCmakeVariable` / `replaceCmakeVariableString`.
  3. Filename + content placeholder substitution in `ProjectExporter`.
  4. `Resources` JSON load + init order.

### B. Implement mode
- Pick one `UT-<id>` from `.claude/UNIT_TEST_TASKS.md` (status `pending`).
- Add files under `unittests/<SuiteName>/` plus a `CMakeLists.txt` registering the suite.
- Build with `-DCOMPILE_UNITTESTS=ON` and run; tests must pass before marking done.
- Update task status in `UNIT_TEST_TASKS.md`.

## Allowed paths to touch
- Read: anywhere.
- Write: `unittests/**`, `.claude/UNIT_TEST_TASKS.md`.
- Do **not** modify `core/`, `cmake/`, root `CMakeLists.txt` beyond inside USER_SECTION slots if a slot is required.

## Done when
- Suite compiles with `COMPILE_UNITTESTS=ON`.
- All cases pass.
- Task in `UNIT_TEST_TASKS.md` marked `done`.
