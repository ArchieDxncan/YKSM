# YKSM

YKSM is a native Nintendo 3DS save manager and transfer bank for the Yo-kai Watch series.
It uses a single-column list interface designed for the 3DS screens; it does not use a box grid.

## Supported games

- Yo-kai Watch
- Yo-kai Watch 2: Bony Spirits, Fleshy Souls, and Psychic Specters
- Yo-kai Watch 3
- Yo-kai Watch Blasters, including Moon Rabbit Crew save files
- Yo-kai Watch Busters 2

YKSM detects installed SD titles and game cards automatically. It can also load original encrypted
exports from `/3ds/YKSM/saves/<GAME>/`, including `game1.yw`, `game2.yw`, `game3.yw`, and
Blasters' `game1.yw_g`. Authenticated saves must be kept beside their matching `head.yw` or
`head.yw_g`.

The local transfer bank is `/3ds/YKSM/bank.ykb`. Before an installed save is changed, a backup is
written under `/3ds/YKSM/backups`.

`.ykbank` migration is intentionally not supported.

## Controls

Save-selection screen:

- Left/Right: choose a game card or installed title
- Up/Down: choose one of that title's save files
- A: open the selected save
- Y: switch between installed titles and manually exported saves
- START: exit

Save overview:

- The top screen shows the save's player name and play-time field (decoded for Yo-kai Watch 1;
  later formats currently show a clear fallback until their regional profile layouts are verified)
- A or tapping **Yo-Kai** opens the transfer lists
- B: return to save selection

Transfer lists:

- Top screen: local bank; bottom screen: the opened save's Yo-kai
- Up/Down: move through the selected list
- SELECT: switch selection between the save and bank lists
- L/R: change local-bank pages
- X: mark or unmark the selected Yo-kai
- Y: mark every Yo-kai in the active list
- A: stage deposits or withdrawals
- START: confirm and save staged changes
- B: discard staged changes, or return to the save overview when there are no changes

## Installation

For a Homebrew Launcher installation, copy `YKSM.3dsx` to `/3ds/YKSM/YKSM.3dsx` on the SD card.
For a HOME Menu installation, install `YKSM.cia` with a trusted CIA installer.

Always retain the automatic backup until the edited save has been opened successfully in-game.

## GitHub Actions build

The included `.github/workflows/build.yml` runs the portable core tests and builds the Nintendo 3DS
artifacts in devkitPro's `devkitarm` container. It marks the container workspace as a trusted Git
directory, installs the required 3DS libraries, builds pinned versions of `bannertool`, `makerom`,
and `3dstool`, and uploads:

- `YKSM.3dsx`
- `YKSM.cia`
- `YKSM.elf`

Use **Actions → Build YKSM → Run workflow**. The files appear in the `YKSM-3DS` artifact after a
successful run. No repository secrets are required.

## Local tests

```sh
make -C tests build/YokaiCoreTests
./tests/build/YokaiCoreTests
```

Encrypted backups can be checked without committing personal saves:

```sh
make -C tests build/YokaiFixtureTests
./tests/build/YokaiFixtureTests /path/to/extracted/saves
```

## Building locally

Install devkitARM, libctru, citro2d, citro3d, 3ds-curl, 3ds-bzip2, 3ds-mpg123,
3ds-pkg-config, 3dstools, tex3ds, bannertool, makerom, and 3dstool. Then run:

```sh
make debug -j2
```

## Project lineage and licensing

YKSM is a GPLv3-or-later derivative of FlagBrew's PKSM framework. It retains the upstream copyright,
license, and attribution notices required by GPLv3 sections 7.b and 7.c. The inherited framework was
adapted exclusively for Yo-kai Watch save handling; the upstream game-specific startup services,
asset downloads, databases, documentation, and interface are not part of YKSM's runtime.

Yo-kai save cryptography and format research derived from `togenyan/yw_save` is MIT licensed. See
`LICENSE` and `THIRD_PARTY_LICENSES.md`.
