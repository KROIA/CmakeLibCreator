# FINISHED_TASKS

Completed tasks for the **current** version. Rotated into `changelogs/<version>.md` at release.

### TASK-007 — Relocate `settings.json` and `data/` to per-app AppData — ✓ 2026-05-06
- `Resources` ctor now anchors `settings.json` and the `data/` tree under `QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)` → `%AppData%\Roaming\KROIA\CmakeLibCreator\` on Windows. Wiping the build folder no longer destroys user state.
- `main.cpp` sets `QCoreApplication::setOrganizationName("KROIA")` and `setApplicationName("CmakeLibCreator")` before any `Resources` access.
- Caller-side cleanups in `MainWindow.cpp` and `Utilities.cpp` dropped the `QDir::currentPath() + "/" +` prefix where it concatenated relative paths returned by `Resources` — those paths are now absolute.
- Migration policy: start fresh; no copy of old in-tree files. User-confirmed.
- User-confirmed working.
- Files: `CmakeLibCreator/main.cpp`, `core/src/Resources.cpp`, `core/src/ui/MainWindow.cpp`, `core/src/Utilities.cpp`.

### TASK-008 — ProjectPaths editor + default library path setting (extend `SettingsDialog`) — ✓ 2026-05-06
- New `defaultLibraryPath` setting on `Resources` (persisted in `settings.json`); default value `QStandardPaths::DocumentsLocation + "/Visual Studio 2022/Projects"`.
- `SettingsDialog` extended:
  - Default-library-path row with browse button (icon `:/icons/folder-open.png`).
  - "Project Paths" group box with a `QScrollArea` of dynamic rows (path `QLineEdit` + browse `QToolButton` + delete `QToolButton` using `QStyle::SP_TrashIcon`).
  - "+ Add path" button below the list.
  - Group box vertical sizePolicy = Expanding so the list grows with the dialog; trailing dialog spacer removed.
- `MainWindow::onOpenExistingProject_clicked` and `onSaveAsNewProject_clicked` fall back to `defaultLibraryPath` when no project is loaded.
- **Bug fix during implementation:** removed duplicate manual `connect(...)` calls in the ctor for `defaultLibraryPathBrowse_toolButton` and `addProjectPath_pushButton` — those are auto-wired by Qt's `connectSlotsByName(this)` (called inside `ui.setupUi`) because the slots follow the `on_<widget>_<signal>` naming convention. The duplicate connections were causing each click to fire the slot twice (visible as "+ Add path" producing two rows on the first click).
- User-confirmed working.
- Files: `core/inc/Resources.h`, `core/src/Resources.cpp`, `core/inc/ui/SettingsDialog.h`, `core/src/ui/SettingsDialog.cpp`, `core/ui/SettingsDialog.ui`, `core/src/ui/MainWindow.cpp`.
