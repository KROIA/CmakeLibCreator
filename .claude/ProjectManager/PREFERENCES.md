# PREFERENCES — CmakeLibCreator

User-confirmed choices, persisted across sessions. Re-read on every bootstrap. The user can change any of these anytime — honor the latest answer.

## Project layout
- Default `.claude/` layout from setup spec §2.1 — **accepted as-is**.

## Changelog
- **Enabled.** `CHANGELOG.md` + `changelogs/` folder live in **project root**.
- Per-version files under `changelogs/<version>.md` split into Features / Bugfixes / Deprecations.

## Manual code-review gate
- **Disabled.** Tasks may be marked `done` without an explicit user review step (other DoD stages still apply).

## Unit testing
- **Enabled.** Framework: **KROIA UnitTest** library (`https://github.com/KROIA/UnitTest.git`), fetched via `unittests/UnitTest.cmake`.
- Tasks tracked in `.claude/UNIT_TEST_TASKS.md`.

## Version control
- VCS: **git**.
- **Commit permission:** granted — **automatic** (PM may commit without asking each time, following the pre-commit ritual).
- **Push permission:** **denied.** Never push.
- **Branches:** **work directly on `main`.** No custom feature branches.
- **Repo artifacts:** `CLAUDE.md` and `.claude/` are committed to the repository.

## Commit message style
- `+` for additions, `~` for changes, `-` for deletions.
- Short subject line; details belong in `CHANGELOG.md`.

## Harness hooks
- **Enabled** — see `.claude/settings.json`.

## Setup spec source of truth
- Full PM setup spec lives at: `\\Nmsvr31\data\Gruppe\transfer\Alex Krieg\ClaudeCode\Templates\ClaudeProjectManagerSetup\CLAUDE.md`
- Re-read on demand when a workflow rule isn't remembered or the user asks to change process.
