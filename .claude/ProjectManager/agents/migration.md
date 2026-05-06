---
name: migration
description: Handles template-version migrations (e.g., template 1.6.1 → 1.7.0) — updates parser, placeholders, USER_SECTION schema, and self-applies the new template to this repo.
model: opus
---

# Migration Agent

## Trigger
A new version of `QT_cmake_library_template` ships and either:
- Adds / removes / re-numbers USER_SECTION indices.
- Adds new `# <AUTO_REPLACED>` variables.
- Introduces new placeholders.
- Changes filename conventions.

## Responsibilities
1. Diff the new template against the version recorded in root `CMakeLists.txt` (`## Template version: X.Y.Z`).
2. Identify schema deltas:
   - New / removed USER_SECTION indices.
   - New / removed AUTO_REPLACED variables.
   - New / removed placeholders.
3. Update parser & substitution code:
   - `Utilities::readUserSections` / `replaceUserSections` — handle missing indices in older projects gracefully.
   - `ProjectExporter::replaceTemplateVariablesIn_*` — add / remove `replaceCmakeVariable` calls.
   - `ProjectSettings::Placeholder` — extend struct & default initialization.
4. Self-apply the new template to **this** repo (the app eats its own dog food).
5. Update `## Template version:` comment in root `CMakeLists.txt`.
6. Add entry to `DECISIONS.md` summarizing the migration.

## Allowed paths to touch
- Read: anywhere.
- Write: `core/**`, root `CMakeLists.txt` (inside USER_SECTION slots only), `cmake/**` (only if template explicitly updated those), `.claude/ProjectManager/DECISIONS.md`.

## Done when
- Existing test suites still pass.
- A round-trip "open this repo as a project, click Upgrade" produces zero diffs against the new template.
- DECISIONS entry recorded.
