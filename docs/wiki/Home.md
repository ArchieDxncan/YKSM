# YKSM manual

YKSM is a list-based Yo-kai Watch save manager and local transfer bank for Nintendo 3DS.

## Install

- Homebrew Launcher: copy `YKSM.3dsx` to `/3ds/YKSM/YKSM.3dsx`.
- HOME Menu: install `YKSM.cia` with a trusted CIA installer.

YKSM discovers supported installed titles and game cards. Exported encrypted saves can instead be
placed below `/3ds/YKSM/saves/<GAME>/`. Keep `head.yw` or `head.yw_g` beside authenticated saves.

## Controls

- D-pad: move through the selected list
- SELECT: switch between the game-save and local-bank lists
- L/R: change local-bank pages
- Y: cycle name, level, and original-order sorting
- X: confirm and save staged changes
- A: open the Mark/Move menu
- B: discard staged changes, or exit when clean

Move active-party Yo-kai into reserve slots in-game before depositing them. YKSM rejects party
deposits to avoid leaving the game with a dangling party entry, and displays those members in gray.

The local bank is `/3ds/YKSM/bank.ykb`. Installed-save backups are written under
`/3ds/YKSM/backups` before commits. Keep a backup until the edited save opens successfully in-game.

## Build

GitHub Actions users can run **Actions → Build YKSM → Run workflow** and download the `YKSM-3DS`
artifact. Local prerequisites and test commands are listed in the repository `README.md`.
