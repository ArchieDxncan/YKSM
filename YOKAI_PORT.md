# Yo-kai Watch Bank for Nintendo 3DS

This branch is a list-based Yo-kai transfer bank built on FlagBrew's PKSM
application framework. It deliberately does not use PKSM's Pokémon box view.

## Implemented foundation

- Single-column, paged Game Save and Local Bank lists
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
- Installed SD-title and cartridge discovery through PKSM's save-archive layer
- Automatic `head.yw` / `head.yw_g` matching and multi-slot selection

`.ykbank` migration is intentionally not included. The native bank is the versioned
`YKB1` file described above.

## Save discovery and exported saves

Installed games and cartridges are detected automatically. As a fallback, exported
saves can be placed at:

```text
/3ds/YoKaiWatchBank/saves/YW1/game1.yw
/3ds/YoKaiWatchBank/saves/YW2/game1.yw
/3ds/YoKaiWatchBank/saves/YW3/game1.yw
/3ds/YoKaiWatchBank/saves/BLASTERS/game1.yw
/3ds/YoKaiWatchBank/saves/BUSTERS2/game1.yw
```

`game2.yw`, `game3.yw`, and Blasters' `game1.yw_g` are also recognized. Keep the
matching `head.yw` or `head.yw_g` beside every authenticated save. Original encrypted
files are expected; decrypted editor intermediates are not. The persistent list bank
is stored at `/3ds/YoKaiWatchBank/bank.ykb`, and installed-save backups are written
under `/3ds/YoKaiWatchBank/backups` before commits.

## Controls

- D-pad/Circle Pad: move through the active list
- L/R: switch between Game Save and Local Bank
- ZL/ZR: cycle save slots
- X: mark/unmark one entry
- Y: mark all entries in the active list
- A: deposit/withdraw marked entries
- SELECT: cycle the active game
- START: commit the staged game and bank
- B: discard staged changes; exit when clean

## Build and tests

The 3DS build uses the same devkitARM dependencies as upstream PKSM:

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
attribution. Yo-kai save cryptography and format research derived from
`togenyan/yw_save` remains under its MIT license; see
`THIRD_PARTY_LICENSES.md` in the supplied desktop project.
