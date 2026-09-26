# YKSM
YKSM is a native Nintendo 3DS save manager and transfer bank for the Yo-kai Watch series.
<img width="512" height="265" alt="banner" src="https://github.com/user-attachments/assets/68889704-20a7-4e2c-9191-1e296f3709f7" />

## Supported games

- Yo-kai Watch
- Yo-kai Watch 2
- Yo-kai Watch 3
- Yo-kai Watch Blasters
- Yo-kai Watch Busters 2

YKSM detects installed SD titles and game cards automatically. It can also load original encrypted
exports from `/3ds/YKSM/saves/<GAME>/`, including `game1.yw`, `game2.yw`, `game3.yw`, and
Blasters' `game1.yw_g`. Authenticated saves must be kept beside their matching `head.yw` or
`head.yw_g`.

The local transfer bank is `/3ds/YKSM/bank.ykb`. Before an installed save is changed, a backup is
written under `/3ds/YKSM/backups`.


## Controls

Save-selection screen:

- Left/Right: choose a game card or installed title
- Up/Down: choose one of that title's save files
- A: open the selected save
- Y: switch between installed titles and manually exported saves
- START: exit

Save overview:

- The top screen uses PKSM's original main-menu composition and shows verified profile metadata.
  Yo-kai Watch 1 player names are decoded; play time currently shows **Not available** because the
  formerly used `0x60` value is not a play-time field. Unverified values are never presented as fact.
- A or tapping **Yo-Kai** opens the transfer lists
- B: return to save selection

Transfer lists:

- Top screen: local bank; bottom screen: the opened save's Yo-kai
- Up/Down: move through the selected list; hold either direction to keep scrolling
- Left/Right: jump backward/forward six entries
- L: select the local bank on the top screen; R: select the save on the bottom screen
- SELECT: enter or leave mark mode; A or touching a save-list row toggles its mark
- Y: cycle the active list through name, level, and original-order sorting
- X: confirm and save staged changes
- A: open the **Mark / Move / Copy** menu on the active screen
- B: discard staged changes, or return to the save overview when there are no changes

Active-party Yo-kai cannot be deposited. Move them to a reserve slot in-game first; this prevents
the game's party list from retaining a reference to a removed record and displaying a duplicate.
Party members are detected from each game's ordered identifier table and shown grayed out. Copy is
non-destructive and works with party members; copies inserted into a save receive a fresh internal
identifier and are placed in reserve rather than an empty party position.
Leaving mark mode with one or more entries marked opens the action menu automatically, with Move
selected by default. Move and Copy then apply to the marked batch.

## Installation

For a Homebrew Launcher installation, copy `YKSM.3dsx` to `/3ds/YKSM/YKSM.3dsx` on the SD card.
For a HOME Menu installation, install `YKSM.cia` with a trusted CIA installer.

## Project lineage and licensing

YKSM is a GPLv3-or-later derivative of FlagBrew's PKSM framework. It retains the upstream copyright,
license, and attribution notices required by GPLv3 sections 7.b and 7.c. The inherited framework was
adapted exclusively for Yo-kai Watch save handling; the upstream game-specific startup services,
asset downloads, databases, documentation, and interface are not part of YKSM's runtime.
The Nintendo 3DS application metadata identifies ArchieDxncan as the developer.

Yo-kai save cryptography and format research derived from `togenyan/yw_save` is MIT licensed. See
`LICENSE` and `THIRD_PARTY_LICENSES.md`.

The desktop behavior, staged-transfer model, YW1 signed species IDs, record offsets, and regression
expectations are cross-checked against
[`ArchieDxncan/ykw-bank`](https://github.com/ArchieDxncan/ykw-bank), the primary behavioral reference
for this 3DS port.
