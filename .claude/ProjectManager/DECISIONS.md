# DECISIONS — Architectural Decision Log

Append entries chronologically. Keep them short — capture the *why*.

## Format
```
### YYYY-MM-DD — <decision title>
**Context:** <one or two sentences on the situation>
**Decision:** <what was decided>
**Why:** <rationale>
**Consequences:** <follow-on effects, if any>
```

---

### 2026-05-06 — Adopt Project Manager scaffolding
**Context:** Project had no formal task / issue / decision tracking; PM agent setup spec was loaded into `CLAUDE.md`.
**Decision:** Initialize the standard `.claude/` layout (top-level state files + `ProjectManager/` internal artifacts + `agents/` templates), enable changelog in repo root, work on `main` only, no manual review gate, KROIA UnitTest as test framework, commit allowed but push denied.
**Why:** Codify project workflow for repeatable, auditable AI-assisted development.
**Consequences:** Subagents inherit `CODING_STYLE.md`; PM commits are scoped (no push); release workflow keyed off `LIBRARY_VERSION` in root `CMakeLists.txt`.
