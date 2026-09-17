This section contains a collection of info on creating scripts for PKSM.

## Setting up your environment

To start creating scripts for PKSM you will need to make sure you have the following tools/resources installed:

### PC
- [Python 3](https://www.python.org/downloads/release/python-370/) or [Node.js](https://nodejs.org)
- [PKSM-Scripts source code](https://github.com/FlagBrew/PKSM-Scripts)
- [PKHeX](https://projectpokemon.org/home/files/file/1-pkhex/) - A Pokémon save editor for Windows written in C#
    - For those on Mac/Linux there is a Mono build
- hex editor
    - Windows: [HxD](https://mh-nexus.de/en/hxd/)
    - Mac: [iHex](https://itunes.apple.com/us/app/ihex-hex-editor/id909566003?mt=12)
    - Linux: [Bless](https://github.com/bwrsandman/Bless)

### 3DS
- [PKSM](https://github.com/FlagBrew/PKSM/releases/latest) -- v6.0.0 or later
- Save manager app for Homebrew/CFW
    - `CFW` - [Checkpoint](https://github.com/FlagBrew/Checkpoint/releases) - works with both 3DS and DS games

## Compiling Existing Scripts
Open your Command Prompt (Windows) or Terminal (Mac/Linux) to `PKSM-Scripts` then follow the instructions for your scripting language below

### Python 3
- Run one of the following commands
    - Windows
        - All scripts (in the `.txt` files): `py -3 genScripts.py`
        - Single script: `py -3 PKSMScript.py "USUM - Set max money" -i 0x4404 4 9999999 1`
    - Mac / Linux
        - All scripts (in the `.txt` files): `python3 genScripts.py`
        - Single script: `python3 PKSMScript.py "USUM - Set max money" -i 0x4404 4 9999999 1`

### Node.js
- Run one of the following commands
    - All scripts (in the `.txt` files): `node genScripts.js`
    - Single script: `node PKSMScript.js "USUM - Set max money" -i 0x4404 4 9999999 1`

## "Legacy" Script file format

```
## PKSM script structure
# u8 magic[10]        // PKSMSCRIPT
# u32 offset          // save offset to write the data to
# u32 length          // payload length
# u8 payload[len]     // payload data
# u32 repeat_times    // repeat payload n times
# ...
```

## Making New Scripts
To create entirely new scripts you will need the following data:
- **offset** -- where in the save file you find the value controlling the change you want to make
- **new value** -- the result of the change you want to make
- **value length** -- the number of bytes the save uses to represent the value(s) you're changing

### Finding Offsets
There are a few options for finding the offset(s) you want to edit in the save files:
- This folder contains a consolidation of much of the save offset info from PKHeX's source code and Project Pokémon's Tech Doc pages. While it may not cover everything, it should at least give you an idea of where to look when searching for your offset manually
- [PKHeX's source code](https://github.com/kwsch/PKHeX) - it helps if you can read C# code (another C-like language works too) and understand the hexadecimal system
- [Project Pokémon's Technical Documentation pages](https://projectpokemon.org/docs/)
- search for the offset manually
- ask for help on the [FlagBrew Discord server](https://discord.gg/bGKEyfY) (preferably in \#pksm-tools-general)

#### Searching for an Offset Manually
If the value you want to edit has already had its offset documented, skip down to ["Testing Offsets and Values"](#testing-offsets-and-values). If you can't find the value you want to edit documented, you'll have to search for the proper offset manually
1. Save your game before performing an in-game action that will make the change you want
2. Use the save manager on your 3DS to backup your save (if possible give it a name letting you know what the save is for)
3. Go back into your game and perform an action that will make the change you want
4. Make a *new* backup of your save (separate from the previous one)
    - for repeatable changes (eg. gaining or losing money), repeating steps 3 and 4 (and making a *new* backup each time) can help narrow down the possibilities for your target offset
5. Move/copy your saves to your PC
6. Open and compare the saves in your hex editor
    - any offset that changes between each file is a possibility for your desired change
    - **NOTE**: be sure to compare your list to the offsets documented in this folder so that you don't accidentally use the offset of something unrelated that often changes during normal gameplay, like play time or checksums
    - **NOTE**: some effects may require changing multiple offsets
7. Once you've found a likely candidate for the change you want to make, move on to "Testing Offsets and Values" below

### Testing Offsets and Values
1. In your hex editor, change the value of your offset and save the changes
2. To make your edited save recognizable by your game you need to get it resigned by doing the following:
    1. Open the save in PKHeX
    2. Export the save (File > Export SAV... > Export main), saving it over the version you opened
3. Transfer your edited save back to your 3DS and import/restore with your save manager
4. Boot your game and check what has changed
    1. If your change affected what you wanted, try different values for your change until you get a final result you're happy with, then move on to "Compilation" below
    2. If your change didn't affect what you wanted, return to "Testing Offsets" step 1 and try another possible offset
    3. If you don't have any more possibilities remaining, try starting your search over again from ["Searching for an Offset Manually"](#searching-for-an-offset-manually) step 1

### Compilation
Once you have the correct offset and value for the change you want to make, all that's left is to construct the command for compiling your new script and making sure it works as a script.

The command you need to use to compile your scripts can vary depending on what operating system you're using. Replace the `<bracketed_values>` with the appropriate values for your script.
- Python
    - Windows: `py -3 PKSMScript.py <name> -i <offset> <length> <payload> <repeat>`
    - Mac / Linux: `python3 PKSMScript.py <name> -i <offset> <length> <payload> <repeat>`
- Node.js: `node PKSMScript.js <name> -i <offset> <length> <payload> <repeat>`
> If your script changes multiple, non-consecutive offsets, just add an extra set of `-i <offset> <length> <payload> <repeat>` for each one

Where...
- `<name>` -- the name you want your new script to have, enclosed in quotation marks: `"Set max money"`
- `<offset>` -- the save offset you are editing
- `<length>` -- the number of bytes you are writing to the save
- `<payload>` -- the value(s) you are writing to the save; can be one of the following
    - an integer (either decimal or hex)
    - the name of a binary file containing the data (values) to use, enclosed in quotation marks: `"data/USUM_AllItems.bin"`
- `<repeat>` -- how many times in succession the value should be written to the save

## PKSMScript Syntax
`PKSMScript.py [-h] output [-d subdir] [-i ofs len pld rpt]`<br />
`PKSMScript.js [-h] output [-d subdir] [-i ofs len pld rpt]`
> You can use `PKSMScript.py -h` (Python) or `PKSMScript.js -h` (Node.js) to view PKSMScript's own documentation

To create completely new scripts, you will need to find the following values:
- `output` -- the name of your new script
- `-d subdir` -- denotes an optional subdirectory to place the compiled script in
- `-i` -- denotes the beginning of input values (can be repeated, along with extra sets of `ofs`, `len`, `pld`, and `rpt` values, to change more than one offset with a single script)
- `ofs` -- the offset (location) in the game's save of the value you want to edit
- `len` -- how many bytes (offsets) need to be written over
- `pld` -- the new value you want to write to the save (or a `.bin` file containing a list of values to write)
- `rpt` -- how many times you want `pld` to be written to the save in succession

# picoC Script Documentation

## API
Header inclusion
```c
#include <pksm.h>
```

### Arguments passed to `main`
- `argv`
    - `argv[0]` pointer to save data
    - `argv[1]` save data length (int passed as text)
    - `argv[2]` save's game version (game's index number as found in a Pokémon's *source game* field)
        - DP = 10, Pt = 12, HGSS = 7
            - Based on content alone Diamond saves cannot be told apart from Pearl saves and HeartGold saves cannot be told apart from SoulSilver saves
        - B = 21, W = 20, B2 = 23, W2 = 22
        - X = 24, Y = 25, OR = 27, AS = 26
        - S = 30, M = 31, US = 32, UM = 33
        - LGP = 42, LGE = 43

### GUI Functions
> All `char*` arguments for showing text to the user can contain newlines (`\n`), but sometimes it is not advised. When in doubt, test it to see if things look okay.

```c
int gui_choice(char* message);
void gui_warn(char* warning);
```
![gui_choice](https://i.imgur.com/vPrM81g.png) ![gui_warn](https://i.imgur.com/fsB2uE1.png)

Use of newlines is not advised in the arguments of these functions.
- `gui_choice` returns `1` if the user exits with `A` or it returns `0` if the user exits with `B`
- `gui_warn` only allows the user to exit with `A`. "Warn" is somewhat of a misnomer -- it was originally made to warn users that something wrong or unexpected happened, but can be used to show any simple information to the user

```c
int gui_menu_6x5(char* question, int options, char** labels, struct pkx* pokemon, enum Generation generation);
int gui_menu_10x2(char* question, int options, char** labels);
```
![gui_menu_6x5](https://i.imgur.com/YuisnjQ.png) ![gui_menu_10x2](https://i.imgur.com/XrLOjRo.png)
- `char* question`: Text shown on the bottom screen
- `int options`: Total number of options to be displayed to the user
- `char** labels`: Array of strings to use as text labels for the options
- `struct pkx* pokemon`: Array of `pkx` structs (see [Enums and Structs](#enums-and-structs) below)
- `enum Generation generation`: Which generation of species info and sprites should be used in the display (some species have different forms in different generations, like Pikachu)
- `gui_menu_10x2` was previously called `gui_menu_20x2`. Its rows were made taller for legibility, so a page now holds 20 entries instead of 40. The old name still works, but new scripts should use `gui_menu_10x2`.

```c
void gui_numpad(unsigned int* out, char* hint, int maxDigits);
void gui_keyboard(char* out, char* hint, int maxChars);
```
![gui_numpad](https://i.imgur.com/5h4vjsd.png) ![gui_numpad hint](https://i.imgur.com/x1bPwMh.png)
![gui_keyboard](https://i.imgur.com/JThRhQY.png)
- `int maxChars`: The number of UTF-16 codepoints allowed to be input, including the null terminator. The data written to `out` is UTF-8 encoded

Brings up the numpad/keyboard to allow user input.
- `int* out`: pointer to an existing `int` variable to hold the user's input
- `char* out`: pointer to an existing string variable that should be large enough to hold whatever you're prompting the user for, including the `NULL` terminator
- `char* hint`:
    - **numpad**: string to be shown to the user if they click on the `What?` button in the bottom left
    - **keyboard**: string shown in input box when it's empty
- `int maxDigits`: max length of the number provided by user
- `int maxChars`: max length of the string provided by user, including the `NULL` terminator

```c
int gui_boxes(int* fromStorage, int* box, int* slot, int doCrypt);
```
![gui_boxes](https://i.imgur.com/5qwOewI.png)
- All arguments except `doCrypt` should be pointers to existing variables
- `fromStorage`: whether or not the user's selection is in PKSM's storage (`1`) or save's PC (`0`)
- `box`, `slot`: Box and slot numbers of user's selection
- `doCrypt`: whether this should (`1`) or should not (`0`) it should decrypt and encrypt the boxes itself
    - If you use `gui_boxes` after `sav_box_decrypt`, make sure this is `0`
- Returns `0` if a selection was successfully made

### Save and Storage Functions
```c
int sav_sbo();
int sav_gbo();

// example usage
int ofs = sav_gbo() + 0x0;
```
These are only needed for Gen 4 (DP, PT, HGSS). Gen 4 save files store 2 saves (current and backup) broken up into 3 blocks apiece and the blocks can be mixed up within the file. A Gen 4 save file could look like the following:
- Save 1 (starting at `0x0`): current general block, backup storage block, current HoF block
- Save 2 (starting at `0x40000`): backup general block, current storage block, backup HoF block

The return values of `sav_gbo` and `sav_sbo` point you the proper portion of the file containing the current version of the general and storage blocks respectively.

A value of `0` is returned if used on a Gen 5+ save, meaning they'll have no adverse effect on setting offsets on other games.

```c
int sav_get_value(enum SAV_Field field, ...);
int sav_get_max(enum SAV_MaxField field, ...);
```
These are used to get values out of the currently loaded save. Most fields will not require even a second argument. See [Enums and Structs](#enums-and-structs) below for what fields are available. Notable caveats and arguments are:
- `MAX_FORM`: Requires a species number as an argument
- `SAV_OT_NAME`: Returns a UTF-8 formatted string that must be manually freed. Cast to `char*` to use it properly
- `SAV_TID` and `SAV_SID`: Both return the 5-digit format
- `SAV_ITEM`: Requires an `enum Pouch` and a slot number. Returns the item ID of the specified slot
- `MAX_IN_POUCH`: Returns the maximum amount of items for the save. Requires an `enum Pouch`.

```c
char* sav_get_string(unsigned int offset, unsigned int codepoints);
void sav_set_string(char* string, unsigned int offset, unsigned int codepoints);
```
`sav_get_string`: Used to get a UTF-8 encoded string from an arbitrary offset in the save, stopping at the null terminator. `codepoints` is the character limit, including the null terminator. This string must be manually freed.
`sav_set_string`: Used to write a UTF-8 string to an arbitrary offset in the save, and overwrites unnecessary bytes with 0. `codepoints` is the character limit, including the null terminator.

#### Encryption and Decryption
```c
void sav_box_decrypt();
void sav_box_encrypt();
```
**IMPORTANT**: These should *always* be used as a pair and *always* in this order. Mixing them or not using them in pairs *will* produce unpredictable results.

Any edits you aim to make should be done after calling `sav_box_decrypt` and before calling `sav_box_encrypt`.

```c
void pkx_decrypt(char* data, enum Generation type);
void pkx_encrypt(char* data, enum Generation type);
```
For encrypting or decrypting PKX data, usually for reading from or writing to a `.pk*` file
- `char* data`: pointer to an existing variable with Pokémon data to encrypt/decrypt
- `enum Generation type`: which generation the data comes from, so that it can be properly decrypted/encrypted

#### Pkx Editing
```c
void party_get_pkx(char* data, int slot);
void sav_get_pkx(char* data, int box, int slot);
```
Read Pkm data into a variable
- `char* data`: pointer to an existing variable to write the Pokémon data to
- `slot`: slot within the party or box to read the Pokémon from
- `box`: which box number the Pokémon should be read from

```c
void party_inject_pkx(char* data, enum Generation type, int slot);
void sav_inject_pkx(char* data, enum Generation type, int box, int slot, int doTradeEdits);
void bank_inject_pkx(char* data, enum Generation type, int box, int slot);
```
Storing pkm data from a variable into the party, PC boxes, or PKSM bank
- `char* data`: pointer to an existing variable to write the Pokémon data from
- `enum Generation type`: which generation the data comes from, so that it can be properly written
- `slot`: slot within the party or box to inject to
- `box`: which box number the Pokémon should be injected into
- `doTradeEdits`: boolean controlling whether PKSM applies appropriate trade logic (non-`0`) or not (`0`)

```c
int pkx_box_size(enum Generation gen);
int pkx_party_size(enum Generation gen);
```
Gets the box or party size of a single Pokémon structure for the given format. Mainly meant for easy allocation of char arrays for storing Pokémon data

```c
void pkx_generate(char* data, int species);
```
This wipes the current data in the array, then initializes it with the default values that PKSM would provide if you were to generate the specified species from the GUI.

```c
void pkx_set_value(char* data, enum Generation gen, enum PKX_Field field, ...);
```
This is a variadic function. It can take arguments with differing types based on the PKX_Field passed in. See [Enums and Structs](#enums-and-structs) below for what fields can be set. The requested arguments are normally single integers, with a few exceptions:
- `OT_NAME`: Requests a single UTF-8 encoded, null terminated string
- `NICKNAME`: Requests a single UTF-8 encoded, null terminated string
- `MOVE`: Requests two integers. The first is the index of the move you wish to change (0-3), the second is the value that you wish to set it to.
- `PP`: Requests two integers. The first is the index of the move you want to change (0-3), the second is the value that you wish to set it to.
- `PP_UPS`: Requests two integers. The first is the index of the move you want to change (0-3), the second is the value that you wish to set it to.
- `POKERUS`: Requests two integers. The first is the Pokérus strain, the second is the amount of days left before Pokérus is no longer spreadable

```c
unsigned int pkx_get_value(char* data, enum Generation gen, enum PKX_Field field, ...);
```
Another variadic function. This will get values out of a PKX_Field, returning the results as an unsigned integer, which, if not what you want already, can be casted to the correct type. See [Enums and Structs](#enums-and-structs) below for what fields can be retrieved.
Most fields do not request an argument. Notable caveats and arguments are:
- `OT_NAME`: Returns a UTF-8 encoded, null terminated string. This must be manually freed.
- `NICKNAME`: Returns a UTF-8 encoded, null terminated string. This must be manually freed.
- `POKERUS`: Returns both the strain and the amount of days left as a single byte. The high nybble is the strain, the low nybble is the amount of days left
- `MOVE`: Requests a single integer argument and returns the corresponding move ID. Zero means no move is present in that move slot.
- `PP`: Requests a single integer argument and returns the corresponding move's PP.
- `PP_UPS`: Requests a single integer argument and returns the corresponding move's amount of PP Ups used.

```c
int pkx_is_valid(char* data, enum Generation gen)
```
A boolean utility function to see Pokémon data (such as that received from `sav_get_pkx`) actually contains a Pokémon. If it does, as is indicated by either the species value or the encryption constant being nonzero, it returns 1, otherwise returning zero.

### IO Functions
```c
char* current_directory();
```
Returns the directory from which the script was called
- This must be manually freed

```c
struct directory* read_directory(char* dir);
```
For reading the files in directory `dir`. Returns a `directory` struct (see [Enums and Structs](#enums-and-structs) below for details)
- To free the directory struct, use `delete_directory`

```c
void delete_directory(struct directory* dir);
```
Deletes a directory retrieved via `read_directory`

### Config Reading Functions
All these functions return the values exactly as they appear in PKSM's `config.json`
```c
char* cfg_default_ot();
```
- Older generations have smaller OT name limits so depending on the contents of the default OT field and the generation(s) you're editing your script may have to trim the string to fit
- You will have to use `utf8_to_utf16` (see [Text Functions](#text-functions) below) before writing the OT to a pkm or the save, unless using `pkx_set_value`. Note that Gen 4 text is not stored as UTF-16, so it is recommended that you use `pkx_set_value` there.
- This must be manually freed

```c
unsigned short cfg_default_tid();
unsigned short cfg_default_sid();
```
Return 5-digit IDs

```c
int cfg_default_day();
int cfg_default_month();
int cfg_default_year();
```
- Scripts should check the result of `cfg_default_year()` to make sure it is the correct format for the intended usage (i.e. 2-digit or 4-digit) and modify it if necessary

### Networking Functions
```c
char* net_ip();
```
Returns the IP address of your 3DS as a string
- This string *SHOULD NOT* be freed

```c
int net_udp_recv(char* buffer, int size, int* received);
int net_tcp_recv(char* buffer, int size, int* received);
```
Receives data (such as pkx, WC, etc.) sent from a client running on another device
- `char* buffer`: pointer to an existing `char` Array to hold the data being received
- `int size`: expected number of bytes
- `int* received`: pointer to an existing `int` variable to hold the number of received bytes
- returns `0` if data was successfully received (scripts will need to check the validity of the received data themselves)

```c
int net_tcp_send(char* ip, int port, char* buffer, int size);
```
Sends data to a compatible client on another device
- `char* ip`: IP address of device to send data to
- `int port`: port on device to send data to
- `char* buffer`: data to send
- `int size`: size of data being sent, in bytes
- returns `0` if successful

### Text Functions
```c
char* i18n_species(int species);
```
Translates the National Dex number of a species into it's name in the user's language
- This string ***SHOULD NOT*** be freed or edited! It is a direct pointer to where it is stored in PKSM, so freeing/editing it will result in terrible things happening

```c
char* utf16_to_utf8(char* data);
char* utf8_to_utf16(char* data);
```
Converts strings between UTF8 (like OT is stored in config) and UTF16 (like OT is stored in save files)
- returned strings must be manually freed

### Enums and Structs
```c
enum Generation {
    GEN_FOUR,
    GEN_FIVE,
    GEN_SIX,
    GEN_SEVEN,
    GEN_LGPE
};

// example usage
int pokePick = gui_menu_6x5("Choose Honey Tree Pokémon", optNum, &treePokes[0], &pokes[0], GEN_FOUR);
```
When something depends on the generation being worked with (sprites, data structure, etc.), use one of these. The example usage code comes from the [DPPT Honey Tree](https://github.com/FlagBrew/PKSM-Scripts/blob/master/src/universal/DPPt%20-%20Honey%20Tree%20Manipulator.c) script, though there are other examples available in the [repo](https://github.com/FlagBrew/PKSM-Scripts/tree/master/src).

```c
struct pkx {
    int species;
    int form;
};
```
- `species` is the National Dex number of the Pokémon you want to display
- `form` is always necessary, even if all the species you're working with do not have alternate forms. If you're not trying to display a certain alternate form, set `form` to `0`.

```c
struct directory {
    int count;
    char** files;
};
```

```c
enum PKX_Field {
    OT_NAME,
    TID,
    SID,
    SHINY,
    LANGUAGE,
    MET_LOCATION,
    MOVE,
    BALL,
    LEVEL,
    GENDER,
    ABILITY,
    IV_HP,
    IV_ATK,
    IV_DEF,
    IV_SPATK,
    IV_SPDEF,
    IV_SPEED,
    NICKNAME,
    ITEM,
    POKERUS,
    EGG_DAY,
    EGG_MONTH,
    EGG_YEAR,
    MET_DAY,
    MET_MONTH,
    MET_YEAR,
    FORM,
    EV_HP,
    EV_ATK,
    EV_DEF,
    EV_SPATK,
    EV_SPDEF,
    EV_SPEED,
    SPECIES,
    PID,
    NATURE,
    FATEFUL,
    PP,
    PP_UPS,
    EGG,
    NICKNAMED,
    EGG_LOCATION,
    MET_LEVEL,
    OT_GENDER,
    ORIGINAL_GAME
};
```

```c
enum SAV_MaxField {
    MAX_SLOTS,
    MAX_BOXES,
    MAX_WONDER_CARDS,
    MAX_SPECIES,
    MAX_MOVE,
    MAX_ITEM,
    MAX_ABILITY,
    MAX_BALL,
    MAX_FORM,
    MAX_IN_POUCH
};
```

```c
enum SAV_Field
{
    SAV_OT_NAME,
    SAV_TID,
    SAV_SID,
    SAV_GENDER,
    SAV_COUNTRY,
    SAV_SUBREGION,
    SAV_REGION,
    SAV_LANGUAGE,
    SAV_MONEY,
    SAV_BP,
    SAV_HOURS,
    SAV_MINUTES,
    SAV_SECONDS,
    SAV_ITEM
};
```

```c
enum Pouch {
    NormalItem,
    KeyItem,
    TM,
    Mail,
    Medicine,
    Berry,
    Ball,
    Battle,
    Candy,
    ZCrystals
};
```

## Tips
- Avoid allocating arrays based a size stored in a variable (for instance `char buffer[size];`). Use something along the lines of `char* buffer = malloc(size);` instead and free the pointer after use.

### Accessing Save Values
All data types in these examples are signed, but unsigned works just as well (and might be needed instead depending on the data you're working with)
```c
// 1 byte value
char val = saveData[offset];
saveData[offset] = val;

// 2 byte values
short val = *(short *)(saveData + offset);
*(short)(saveData + offset) = val;

// 4 byte values
int val = *(int *)(saveData + offset);
*(int)(saveData + offset) = val;
```


## Differences From C90
> From [this wiki page](https://code.google.com/archive/p/picoc/wikis/DifferencesFromC90.wiki)

### How picoC differs from C90
picoC is a tiny C language, not a complete implementation of C90. It doesn't aim to implement every single feature of C90 but it does aim to be close enough that most programs will run without modification.

picoC also has scripting abilities which enhance it beyond what C90 offers.

### C preprocessor
There is no true preprocessor in picoC. The most popular preprocessor features are implemented in a slightly limited way.

#### #define
define macros are implemented but have some limitations. They can only be used as part of expressions and operate a bit like functions. Since they're used in expressions they must result in a value.

#### #if / #ifdef / #else / #endif
The conditional compilation operators are implemented, but have some limitations. The operator "defined()" is not implemented. These operators can only be used at statement boundaries.

#### #include
include is supported however the level of support depends on the specific port of picoC on your platform. Linux/UNIX and cygwin support #include fully.

### Function declarations
This style of function declaration is supported:

`int my_function(char param1, int param2, char *param3) { ... }`

The old "K&R" form of function declaration is not supported.

### Predefined macros
A few macros are pre-defined:

- **picoC_VERSION** - gives the picoC version as a string eg. `"v2.1 beta r524"`
- **LITTLE_ENDIAN** - is 1 on little-endian architectures or 0 on big-endian architectures
- **BIG_ENDIAN** - is 1 on big-endian architectures or 0 on little-endian architectures

### Function pointers
Pointers to functions are currently not supported.

### Storage classes
Many of the storage classes in C90 really only have meaning in a compiler so they're not implemented in picoC. This includes: static, extern, volatile, register and auto. They're recognised but currently ignored.

### struct and unions
Structs and unions can only be defined globally. It's not possible to define them within the scope of a function.

Bitfields in structs are not supported.

### Linking with libraries
Because picoC is an interpreter (and not a compiler) libraries must be linked with picoC itself. Also a glue module must be written to interface to picoC. This is the same as other interpreters like python.

If you're looking for an example check the interface to the C standard library time functions in cstdlib/time.c.

### goto
The goto statement is implemented, but only supports forward gotos, not backward. The rationale for this is that backward gotos are not necessary for any "legitimate" use of goto.

Some discussion on this topic: * http://www.cprogramming.com/tutorial/goto.html * http://kerneltrap.org/node/553/2131
