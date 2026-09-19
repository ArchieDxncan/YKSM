# Yo-kai Watch Bank for Nintendo 3DS

This branch is a list-based Yo-kai transfer bank for Nintendo 3DS. It uses selected
GPLv3 framework components from FlagBrew's PKSM project.

## Implemented foundation

- Single-column, paged Game Save and Local Bank lists
- Card/installed-title save selector with real system title icons and a manual-save view
- Controller and touch navigation
- Multi-select deposit and withdrawal in visible order
- Staged transfers with save/discard behavior
- Arrival-ordered, versioned `YKB1` bank with CRC32 integrity checking
- Atomic bank replacement, exported-save recovery journal, and installed-save backups
- YW1, YW2, YW3, Blasters, and Busters 2 decrypted record layouts
- Variable-size Blasters record sections
- Same-game byte-preserving transfers
- Cross-game species safety checks and destination-native record construction
- Nickname, level, XP, attitude, seriousness, HP, and IV conversion contract
- Generated species and action-game default-move tables
- Level-5 stream encryption, CRC verification, AES-CCM authentication, and per-title key derivation
- CRC-derived section ordering for YW2, YW3, Blasters, and Busters 2
- Installed SD-title and cartridge discovery through the inherited save-archive layer
- Automatic `head.yw` / `head.yw_g` matching and multi-slot selection
- One-load save context shared across the selector, overview, and transfer screens
- Cached-record batch transfers to avoid repeated full-save parsing on 3DS
- PKSM TitleLoadScreen/MainMenu-derived selector and save-overview compositions

The YW1 player-name location at `0x28` is verified by the published save dumper. No supported
format currently has a verified play-time field in the available references. The earlier assumption
that YW1 offset `0x60` stored 60 Hz play-time ticks was removed because it produced incorrect time.

`.ykbank` migration is intentionally not included. The native bank is the versioned
`YKB1` file described above.

## Save discovery and exported saves

Installed games and cartridges are detected automatically. As a fallback, exported
saves can be placed at:

```text
/3ds/YKSM/saves/YW1/game1.yw
/3ds/YKSM/saves/YW2/game1.yw
/3ds/YKSM/saves/YW3/game1.yw
/3ds/YKSM/saves/BLASTERS/game1.yw
/3ds/YKSM/saves/BUSTERS2/game1.yw
```

`game2.yw`, `game3.yw`, and Blasters' `game1.yw_g` are also recognized. Keep the
matching `head.yw` or `head.yw_g` beside every authenticated save. Original encrypted
files are expected; decrypted editor intermediates are not. The persistent list bank
is stored at `/3ds/YKSM/bank.ykb`, and installed-save backups are written
under `/3ds/YKSM/backups` before commits.

## Controls

- Top screen: Local Bank; bottom screen: the opened save's Yo-kai
- Up/Down: move through the selected list
- SELECT: switch selection between the save and bank lists
- L/R: change Local Bank pages
- X: mark/unmark one entry
- Y: mark all entries in the active list
- A: deposit/withdraw marked entries
- START: commit the staged game and bank
- B: discard staged changes; return to the save overview when clean

## Build and tests

The 3DS build uses devkitARM and the dependencies documented in `README.md`:

```sh
git submodule update --init --recursive
make all
```

The Yo-kai portable regression target does not require devkitARM:

```sh
make -C tests build/YokaiCoreTests
./tests/build/YokaiCoreTests
```

The encrypted-fixture target accepts an extracted backup directory and verifies
authenticated, byte-exact round trips without bundling personal save data:

```sh
make -C tests build/YokaiFixtureTests
./tests/build/YokaiFixtureTests /path/to/extracted/saves
```

Regenerate compact lookup tables after changing the desktop data files:

```sh
python3 tools/generate_yokai_tables.py
```

## Attribution and licensing

This derivative is GPLv3-or-later and retains PKSM's required notices and
attribution. [`ArchieDxncan/ykw-bank`](https://github.com/ArchieDxncan/ykw-bank)
is the primary desktop behavioral and save-layout reference for the port,
including staged transfers and YW1's signed species-ID representation.
Yo-kai save cryptography and format research derived from `togenyan/yw_save`
remains under its MIT license; see `THIRD_PARTY_LICENSES.md`.
