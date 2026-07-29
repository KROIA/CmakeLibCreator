# Repositories tab

The **Repositories** tab manages every library project you have registered (Settings → Project Paths) as a group. Switch to it via the ribbon tab "Repositories" or the central page of the same name — the two stay in sync.

Screenshots are added later; this describes the behaviour.

## Repository cards

Each registered repository is shown as a card:

- **Group checkbox** — includes the repository in *group* actions (ribbon buttons). It is saved and restored across restarts. Individual card buttons always work regardless of this checkbox.
- **Name**, **library version** and **template version**. After a template update the template line shows `X.Y.Z (was A.B.C)`.
- **Uncommitted changes** (Yes / No / `?` when not a git repo), **HEAD** commit subject, **Build** status and **Unittest** status. **Show log** opens the captured output of the last unittest run.

A repository whose folder is missing shows **(not found)** and its action buttons are disabled.

### Per-repository buttons

- **General**: Open folder (opens a new Explorer window), Update template.
- **Git**: Pull, Push, Commit, Discard.
- **Build**: Build, Clean (deletes the `build` folder), Unittest.

## Group actions (ribbon buttons)

Group buttons act on all checked, existing repositories, one after another:

- **Open folders**, **Update templates**, **Refresh status**, **Pull**, **Push**, **Commit**, **Discard**, **Build**, **Clean**, **Unittest**.
- **Refresh status** re-reads git status, HEAD subject, library and template version of **all** repositories.

While a queue runs, the status bar shows the current job (`Verb n/m: repo`) with a progress bar and a **Cancel** button. Cancel kills the running process immediately, drops the rest of the queue, and marks the current job as canceled.

## Warnings

- **Update template** on a repository with uncommitted changes asks for confirmation first (the template should only be applied to a clean repository).
- **Commit** on a repository with a failed build or unittest run asks for confirmation first.
- **Discard** asks for confirmation — it runs `git reset --hard` **and** `git clean -fd`, reverting all uncommitted changes and deleting untracked files.
- Group pre-checks show a single summary dialog listing the affected repositories with three choices: **Proceed for all**, **Skip affected**, **Cancel**.

## Typical workflow

1. **Refresh status** to see which repositories are dirty.
2. Check the repositories you want to work on.
3. **Pull** → **Update templates** → **Build** → **Unittest**.
4. **Commit** (one message for the whole group, pre-filled `~ Update Library Template X.Y.Z`) → **Push**.
