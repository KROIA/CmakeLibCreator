# PROJECT_STATUS

**Current version:** 1.6.0 (supports template 1.6.1)
**Branch:** `main`
**Phase:** post-init — PM scaffolding established 2026-05-06.

## At a glance
- No open hotfixes.
- No backlog tasks. TASK-001 ✓ done 2026-05-06.
- No tasks in progress.
- Code review not yet run — recommend running the **code-review** agent against current `main` to seed `ISSUES.md`.
- Unit-test coverage: KROIA UnitTest framework wired in `unittests/UnitTest.cmake`, but no test suites authored yet.

## Recent commits
- `11ba390` + AI_Knowledge
- `473027a` ~ crashfix
- `8df655e` ~ V1.6.0
- `af8f74c` Merge branch 'feature/cli-and-version'
- `22e6445` ~ Template version: 1.6.1

## Next suggested moves
1. Run **code-review** agent → populate `ISSUES.md`.
2. Run **unit-test** agent → seed `UNIT_TEST_TASKS.md` from current coverage gaps (mainly `Utilities`, `ProjectExporter` parser logic).
3. Run **user-docs** / **api-docs** agents for staleness check against current `core/inc/` headers.
