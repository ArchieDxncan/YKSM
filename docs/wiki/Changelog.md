## 10.2.1

- Fix bag editor search feature
- Allow partial item name search in the bag editor screen

## 10.2.0
- **Improve startup times by 63%**
  - Start time went down from ~27s to ~11s on average
  - This has been achieved by:
    - removing spritesheet decompression calls at startup
    - lazy loading system fonts at startup
- Fix local gpss api url keyboard not allowing symbols
- All submodules and build dependancies updated to latest release
- Boot splash screen look has been slightly improved
- Fix and remove unused localization strings
- Removed mystery gifts update at boot time
- Removed deprecated alpha channel update feature
- Removed deprecated Patreon features code
- Removed leftover code from past functionalities that have been removed or replaced
- Removed unused configuration variables
- Removed FlagBrew Patreon's links from the project and wiki
- General system stability improvements to enhance the user's experience.

## 10.1.3
- Added curl timeouts to GPSS pages

## 10.1.2
- New settings page tab introduced
   - Titled `API`, it allows you to enter the server URL of Local GPSS.
- Updated GPSS to require setting the Local GPSS server URL before it can be accessed.
- Updated Legality Check & Auto Legality to require GPSS server URL to be set before it can be used.
   - Added timeout value (10 seconds for legality check, 120 seconds for auto legalization) to ensure you don't lock your console up if you input a bad URL or forget to start the server
- Removed GPSS Mobile button
- French Ribbons Labels fixed (thank you @Zanguu)
- Removed Random GPSS Pokemon script from PKSM-Scripts
   - Perhaps someone could open a PR and add the functionality into Local GPSS so that it could be added again?
- Disabled patreon builds.
   - Build action on GitHub now publishes the builds

## 10.1.1
This release fixes the bag editor item list bug, as well as the commit hash missing from the version in the top right corner of the screen.

## 10.1.0
- Fixed #1372
- Fixed #1373
- Fixed #1374
- Fixed #1381
- Fixed #1384
- Fixed #1391
- Fixed #1392
- Fixed #1394
- Fixed GPSS download codes not working.

## 10.0.0
- Gen 1 & 2 support!
  - Huge thank you to @SNBeast for his work on this!
- GPSS Upgrades
   - GPSS now works with the current version of the [FlagBrew website upgrades](https://flagbrew.org)
- Language updates
   - The way we structured our language files has changed to hopefully make translators' lives easier
- Storage
  - Adds dumping selected groups from within the bank storage screen
- View of Pokémon will color stats according to nature increase/decrease
  - Hyper Trained stats and hidden abilities will also be colored
- Add many G8 sprites; now they won't all be eggs
- Adds a "save and launch" button to allow immediately launching games with changes
- G3 bag item counts now don't go insane on every change
- Cloning now works properly in blue-cursor mode
- Many miscellaneous bug fixes

## 9.2.0
- The Generation 3 and 8 hex editors are now filled in.
- The QR scanner framerate is now much higher and supports Generation 3 QRs
  - This should not affect scanning time
- Fix a few Pokémon form names
- Add valid size for SWSH v1.0->1.2, not just 1.1->1.2
- Fix Platinum (and possibly HGSS) unsaved box data
- Actually create defaults folder if it's not already
- Fix Archive::init failure with error 0xC92044E6
  - Note: this fixes the underlying issue. People that currently have this issue should seek help in [our Discord server](https://discord.gg/bGKEyfY) if they care about the Pokémon in their PKSM bank.
- Fixed defaults not saving changes between application restarts
- Fix an issue with the Ranger Manaphy Wonder Card
- Fix G3 nickname case when generating

## 9.1.0
- **SWSH v1.2 support**: The Sword and Shield DLC, Isle of Armor, is now supported. Note that saves that are not updated to version 1.2 are not supported due to technical reasons.
- Generation 3 scripting that changes raw data is now possible.
- When injecting into Generation 4 games, the mystery gift main menu option is now activated.
- Fixed Generation 4 text being written unpredictably
- Fixed QR scanner crashing
- Fixed Spanish Generation 3 item text encoding
- Fixed Generation 3 and 4 nature setting
- Fixed some Pokémon editor submenus crashing when used without a save loaded
- Fixed SWSH pouches having strange items available
- Re-add strcasecmp and strncasecmp for scripts
- Add sav_get_bit and sav_set_bit for scripts
- Fix problems with the network bridge functionality
- Fix problems with SWSH cryptography
- General system stability improvements to enhance the user's experience.

## 9.0.1
This is a recovery release. It fixes several crash-inducing bugs, along with a Gen 3 party display/editing issue.

## 9.0.0
- **Generation 3** (Ruby, Sapphire, Emerald, Fire Red, and Leaf Green) support has been added. These games are accessible through normal Extra Saves configuration and direct GBA VC save edits.
  - All features should be available for these games except for Mystery Gifts
  - Most generation 3 Mystery Gifts are distributed as PK3 files, which you can inject via the script `injector.c`
  - ~~**A VERY IMPORTANT NOTE**: Generation 3 saves *must* have the Pokédex before you can edit them in any sane way. We are looking into a way to handle this in the future, but for now avoid editing Generation 3 saves without the Pokédex.~~ This was a red herring found in testing. Please disregard it.
- **Pokemon Templates**: Taking inspiration from PKHeX's method of trainer defaults, there is now one Pokémon per generation that you can set to be your "template". All values from this template will be taken except for the following:
  - Species
  - Nickname
  - Form
  - Ability
  - PID
  - Additionally, if you have the "use save data" option set, the following will be ignored as well:
    - TID
    - SID
    - OT Name
    - OT Gender
    - Origin game
    - Met location (will be set to route 1 of the current game)
- **Compatibility with [GPSS Mobile](https://play.google.com/store/apps/details?id=com.flagbrew.gpss_mobile)**: Legalization can now be done through the GPSS Mobile application, either via QR code or over the network
- **Legal Living Dex**: There is now a legal living dex script. Do note that it requires an Internet connection to work properly.
  - Other new scripts have also been added, including a random team that pulls from the GPSS and a batch editor
- **Unreleased Wonder Cards**: Unreleased wonder cards, such as the Azure Flute, are now included in PKSM.
  - When entering the injection menu, a warning will pop up if the category includes unreleased wonder cards.
- Fix DS writes being reported as far larger than they actually are
- Make sure that the save is in a consistent state after scripts run
- Make the sound playing a lot more efficient and a lot less error-prone
- Fix cloning while in blue-cursor mode acting oddly
- Make green-cursor mode cloning clone the entire selected group
- Fix bank names being reset on every load
- Fix setting party Pokémon not updating level and stats
- Locations displayed in misc editor are now based on the origin game of the Pokémon instead of the current save
- Change "release" to work in a more predictable way
  - Now releases the currently held Pokémon if they exist, otherwise the Pokémon under the cursor or, in the case of green-cursor mode, currently selected
- Fix networking code sometimes fully locking up
- Fix possible issues with mystery gift updates
- Fix error code 0xE0E046BE on Archive::init
- Fix HM07 and HM08 in item selection areas
- Fix JSON exceptions on accessing the GPSS screen without an internet connection
- Fix Wonder Card packing non-identical cards together in some cases
- General code cleanup and optimization

## 8.0.3
Issues were brought up with functions that interfaced with [our website](https://flagbrew.org). Changes server-side to address these fixes have also resulted in client-side changes needing to be made, so this release makes them.

## 8.0.2
- Now loads and converts the old bank.bin format properly
- A couple missing translation strings were added, so now they should display properly
- The legalization button now waits for you to take your finger/stylus off the screen so you don't accidentally press it after checking legality
- GPSS downloading via code works again
- An error in converting old banks that caused crashes was fixed
- Added extra error checking
- Prevented the addition of more than 6 Pokémon to a group upload, causing hilarious visual bugs
- Fixed moving empty slots in green cursor mode causing crashes
- Fixed box swap bringing up "endless" error messages
- Fixed box swap erroring on swapping empty slots
- Fixed BW/B2W2 mystery gift errors
- Fixed `inject.c` script crashing if `/3ds/PKSM/inject` doesn't exist or is empty
- Fixed resizing of banks causing crashes when they were loaded

## 8.0.1
This is a bugfix release. It was brought to my attention that it was not actually possible to enter the group download screen by pressing Y. There are no other changes from the previous release.

## 8.0.0
- **Generation 8** (Sword and Shield) support has been added to core code. Sending a save over the network will allow you to edit it. However, no new sprites are included, so your Galarian Pokémon will be invisible.
  - You will be able to send a Sword/Shield save file from your Switch running the next version of Checkpoint (time of writing this is 3.7.4, which will not work)
- **GPSS groups**: "packages" of two to six Pokémon can be downloaded all at once
  - Pressing Y in the GPSS viewer will take you to the GPSS group viewing screen.
  - They are shown on the top screen in five rows, one group per row
  - You can download a group by pressing A on any of the Pokémon in that group
  - You can upload a group by pressing A on the Pokémon in your storage. Once you have 6 Pokémon selected, a prompt will pop up asking if you'd like to upload. You can also press X to bring this prompt up before selecting 6 Pokémon.
- **Auto Legalization**: powered by @architdate's AutoLegality mod for PKHeX
  - After checking the legality of a Pokémon, you can press the on-screen button to attempt to automatically fix those issues.
  - Only works with an active Internet connection
- **Self-updating Mystery Gift Database**: on startup, PKSM will automatically check with the FlagBrew website to see if there are new Wonder Cards to download
  - This means that full application updates are no longer necessary to update the database
- Storage is now accessible for both LGPE and SWSH saves
  - Neither of these formats can be transferred to any other save type
- DS game icons are now shown
- Cart scanning should work more consistently
- The wireless button on the game selection screen is now clickable without first selecting a game
- Pokémon generated for X, Y, Omega Ruby, and Alpha Sapphire will not have legality issues referring to memories and geolocation by default
- Generation four games will use UPPERCASE letters for their names, as the games would
- Script APIs have changed. See [their wiki](https://github.com/FlagBrew/PKSM-Scripts/wiki) for specifics
  - All scripts should have been updated to use the new APIs properly. As always, report issues encountered while running scripts on [the script repository](https://github.com/FlagBrew/PKSM-Scripts/issues)
- The QR scanner now blocks going to the home menu
  - If this were not the case, trying to exit the application from the home menu would freeze the 3DS
- An error is shown if a scanned QR code does not match the expected format
- The QR scanner follows updated PKHeX Wonder Card QR specifications
- The app icon now bounces around the screen as an indicator that the application is working
- A bunch of changes to GPSS retrieval code will make the viewing experience much more seamless when switching pages
- Editing real-world location bytes is now allowed by default
  - The meanings of the values will be shown on the bottom screen
- Internal changes to remove music dependencies happened. Playback of MP3 files should still work properly (thanks to @Oreo639 for this!)
- Fix Korean fourth-gen games not being detected properly
- Fix problems in setting some long OT names and nicknames when generating
- Fix displaying incorrect BP data for fifth-gen games
- Fix setting abilities that changed when transferring upwards
- Fix fifth-gen games showing the wrong amount of Wonder Cards in the main menu
- Fix fifth-gen games having a corrupted Wonder Card in the first slot
- Fix transferring from fifth-gen games to fourth-gen games messing with the nature
- Fix duplication behavior when multiple Pokémon are selected
- In several places, text would overlap other things. This should now be fixed.
- Fix and improve translations
- General code cleanup and optimization

## 7.0.1
Bugs fixed:
+ Accidentally showing the wrong game name in ExtraSaves config for ORAS
+ Not counting Pokémon with encryption constant 0 as valid
+ Creating a new storage group would instantly error
+ Storage group name character limit being too long, causing error 0xE0E046C7
+ Not saving changes to storage box names if no Pokémon were edited
+ Setting the max length Pokémon name for generations 6 and 7 cut off the last character


## 7.0.0
+ **GPSS, a cloud-based Pokémon sharing service!** Easily upload your Pokémon to share with any other PKSM user.
  + Think the GTS, except you don't have to get/give anything in return and you can't be sniped.
  + *Note:* Once a month all Pokémon currently on the GPSS are checked for their monthly download count and are deleted if it's 0.
  + You can *browse* the GPSS from within PKSM or your [web browser](https://flagbrew.org/gpss).
  + You can also upload Pokémon from your PC.
  + This is the only planned `servepkx` replacement.
+ **Cloud-based legalization!** (replacement for `serveLegality`)
+ **Add new `Misc` section to the editor** which adds met location editing.
+ **Add instructions**: In many places PKSM now has simple built-in instructions that can be viewed by pressing `Select`.
+ **Add multiple font support**
  + Shows Pokémon-specific characters
  + Shows Korean characters on non-Korean consoles
+ **Auto updater!** Thanks to @joel16 for the CIA updating code
  + You can disable the updater in the settings menu
+ **Update scripts**: thanks to @PKMWM1 for contributing another large batch of scripts.
  + There are also new additions to the PicoC scripting API, for those interested in developing scripts. See the [wiki](https://github.com/FlagBrew/PKSM/wiki/Scripts-Development) for more info.
  + A collaboration between @piepie62 and @FM1337 has also resulted in a script sending utility and a script receiving script! These can be used for easy script development during the debugging stage
+ **Emergency Mode editor**: if you have a Pokémon stuck in storage and it is untransferable to any game, talk to @piepie62 on the FlagBrew Discord for a walkthrough on how to use it (it is pretty dangerous, which is why the way to access it is unreleased).
+ **Multi bank**: You can now organize your collection into multiple banks, similar to Pokémon Bank's box groups.
  + Yes, it is possible to pick Pokémon up from one bank and place them into another without temporarily storing them in the save
+ **Multi clone**: You can now clone multiple Pokémon at once, without going through a tedious workaround.
+ **Filtering in Storage**
  + You can filter by species, form, and moves in the storage screen! More filter options to follow in later releases...
+ **Show save info on main menu**: The top screen of the main menu now shows some basic info pulled from the save:
  + OT
  + TID/SID
  + badges/stamps
  + Wondercards
  + National Dex seen and owned counts
+ **Remove default music**: Music player is still there, but you'll have to provide your own music.
+ **Second bank backup**
+ Add setting to show all automatically generated backups
+ Redesign main menu
+ Show typing in the editor
+ Fix bug if 3ds folder doesn't exist
+ Disallow home button in scripts as it can cause issues
+ Add a screen to configure Extra Saves

## 6.2.1
+ General: Fixed bug where a new install wouldn't be able to create ExtData
+ Scripts: Add/update new scripts (@SpiredMoth)
  + Quick hatch
  + DPPt Great Marsh
  + All TMs has LGPE support
  + DPPt Honey Tree Manipulator
+ Scripts: Fix possible display glitch
+ Editor: Tweak the EV editing logic (@SadisticMystic)

## 6.2.0
+ Storage: Fix swapping multi-selected boxes and shrink the boxes when possible
+ Storage: Cursor now goes to the top-left of the multiselected box instead of moving the Pokemon to the cursor
+ Event Injector: Add wonder card filtering by language
+ Editor: Set origin game when setting save info
+ Scripts: Update to latest PKSM Scripts
  + Fix dex scripts (@SpiredMoth)
  + Update pkx-injector to match the API changes (@piepie62)
+ Scripts: New API (see [here](https://github.com/FlagBrew/PKSM-Scripts/blob/master/dev/picoC.md) for a lot of documentation)
+ Scripts: Make error screen more legible
+ Title screen: Sort by release instead of alphabetically
+ Title screen: Better dynamic cart detection
+ Bag screen: Slight redesign, courtesy of @Mike007899
+ General: Add missing form names
+ General: Update Chinese translations (thanks @kuai18)
+ General: Show the origin mark for Crystal
+ General: Show different-gender Pokemon sprites
  + This will require a new spritesheet to be downloaded.
+ General: ACTUALLY avoid PKSM crashing when bad configuration files are provided.
+ General: Use the current box value to open to the same box you closed last time (storage and editor)
+ General: Actually use LGPE personal data instead of reusing SMUSUM personal data
+ General: Refactor code to make it more organized
+ General: Get rid of C++ I/O streams and use C FILEs, saving 300 KB
+ General: Read the bank at the start of the application instead of loading and unloading on demand
+ Configuration: Get rid of the folder ExtraSaves option. It caused too much confusion
  + The configuration file will automatically be updated
  + Anyone with folders configured will have those settings deleted. Sorry, but it's better than the alternative.

## 6.1.1
+ Storage:  **implement multi pickup**. You're now able to use a three-way selector while in the Storage, like in the original games.
  + The red selector is the default one, it lets you pickup a pokemon at a time.
  + The blue selector lets you quickly swap pokemon locations.
  + The green selector lets you multi-select arbitrary areas to pickup multiple pokemons at once and place them in different places with the same layout you selected them.
+ Storage: fix nickname handling during generation change.
+ Storage: added option to sort by shiny (thanks @PlasticJustice).
+ Storage: fix occasional crashes while cloning in the storage screen.
+ Editor: if the default generation date is 0, use current date when generating.
+ Editor: set OT gender when setting save info.
+ Editor: set nickname flag when a nickname is set.
+ Editor: add genderless icon and rework the gender changing feature.
  + Note: PKSM doesn't check if you're giving a valid gender to what you're generating, so keep common sense when doing it.
+ Editor: allow Burmy form change.
  + Note: this will require PKSM to download new spritesheets. An internet connection on your 3DS is suggested to easily update them.
+ Scripts: update to latest PKSM Scripts:
  + Key System script for B2W2 (@SpiredMoth)
  + Feebas Tile Finder for DPPt (@PlasticJustice)
  + Honey Tree Manipulator for DPPt (@PlasticJustice)
  + Munchlax Trees Checker for DPPt (@PlasticJustice)
  + Mega Evolution Toggle for SMUSUM (@Gudf)
  + All TMs (@SpiredMoth)
  + Friend Safari unlocking for XY (@SpiredMoth)
+ Hex Editor: fix occasional crashes.
+ General: allow entering the QR Code scanner using the touch screen.
+ General: display Gen7 TID for Gen7+ games.
+ General: add Dutch and Korean translations and update them for most of the other languages (thanks @Rowcii, @felixlee0530, @BowFreak, @kuai18).
+ General: optimize text rendering functions to let the interface run smoother.
+ General: avoid PKSM crashing when bad configuration files are provided.

## 6.1.0
+ Storage: Fix backwards transfer when a pokemon has an invalid move, item,
  form, species, ball or ability.
+ Storage: Allow moving between storage boxes with `ZL` and `ZR`.
+ Storage: Implemented storage sorting and filtering.
+ Storage: Implemented box jumping.
+ Storage: Fix storage size to match the old storage size while converting a
  pre-6.x storage file to 6.x+.
+ Editor: Default met location values added for generated pokemon.
+ Editor: Set pokemon language from the configurations while generating a new
  one.
+ Editor: Added default, non-zero PP value and fix relearn moves during
  generation.
+ Editor: Fixed bug with OT names during generation in old gen games.
+ Editor: Fix Gen4 Nature and shinyness editing.
+ Editor: Add check for impossible genderless.
+ Editor: Fix Vivillon chain form editing.
+ Editor: Add back the `Use Save Info` button (the yellow one).
+ Editor: Fixed a couple race conditions happening in the QR Code reader.
+ Events: Wondercard database updated and unreleased wondercards added as well.
+ Hex Editor: Button behaviour fixed when changing values.
+ Hex Editor: Added display of certain editable values (fixes regression from
  v5.1.4).
+ Hex Editor: Add Marking editing.
+ Bag Editor: Fixed item count limits during editing. 
+ Bag Editor: Fixed search results.
+ Bag Editor: Z-Crystals can't be edited anymore.
+ Scripts: Added new C scripts API:
  + A function to move between boxes to choose a box/slot. It works with both
    the save boxes and the storage boxes.
  + A few functions to get pokemon data from save boxes and party.
  + A few networking function to receive/send arbitrary amount of data through
    TCP and UDP sockets.
+ General: Added Citra compatibility (you need to compile PKSM with
  `CITRA_DEBUG=1` to be able to use it on Citra).
+ Scripts: Increase C Scripts stack size and fix script loading/parsing for long
  scripts.
+ Scripts: Fix location of names and images for the pokemon choice script API.
+ Scripts: C Scripts now give feedbacks and return to the Scripts screen if they
  crash, rather than shutting down the console.
+ Scripts: Updated bundled scripts, `servepkx_server` script added.
  + Note: the `servepkx` client hasn't been updated yet, check back in the
    future.
+ Configuration: Region and Country default values have been added to the
  configuration menu to be used during generation.
+ Configuration: References to bad sectors have been removed.
+ General: Translations have been updated to include Portuguese.
+ General: Added Gen7 TID displaying.
+ General: Fixed error message display when PKSM fails to launch.
+ General: Crypto operations optimized.
+ General: Optimized text rendering functions.

## 6.0.1
+ **Fix crash when a flashcard was inserted**. You can now plug your cartridge
  reader with whatever you want.
+ **Fix various offsets** used here and there in the application. This is
  **critical** and **updating** to this version against v6.0.0 **is strongly
  suggested**.
+ **Fix storage renaming bug**, which caused your storage to not be opened
  again.
+ **Fix crashes on unsuccessfull app exit**.
+ Fix party generation.
+ Fix event injection for Pokemon Diamond/Pearl.
+ Fix transfer code from gen4 upwards and a couple backwards transfer issues.
+ Disable nature edit screen for Gen4, as it is linked to PID.
+ Added a console displaying why PKSM doesn't boot up, with explanation and
  possible fixes.
+ Fix various Hex Editor offsets (thanks @SadisticMystic ).
+ Storage transfers through boxes now automatically set dex data.
+ DS save recognition simplified.
+ Added JP translations (thanks @pass0418 ).
+ Added form changing for Arceus and Genesect, disable form changing for
  Xerneas.
+ Added Hyper Training flags to the hex editor.
+ Enable Hex Editor value changing with `A`/`X` buttons.
+ Fix empty ability after generating a pokemon from scratch.
+ Added a search bar for the moves editor, item editor and bag editor screens.
+ Hex Editor now enabled for LGPE.
+ Backup bridged save to the SD card in case the bridge disconnects, so you can
  reinject it manually later.

## 6.0.0
+ **Game support has been extended to cover every functionality from Generation
  4 to LGPE**. This includes, among the others, *Storage*, *Editor* and
  *Scripts*.
  + This means that Editor and Storage are now available for DS games. More on
    that later.
  + This also means that Editor is available for LGPE.
+ The **title loader** has been completely redesigned to highly improve the user
  experience.
  + The user interface is now more *Checkpoint-esque*. PKSM will automatically
    retrieve any valid title (cartridge and digital) that is currently available
    in your console.
  + Title loader now supports **cartridge hotswapping**. You can now remove the
    cartridge while in it and put another one. The title list will automatically
    reflect the changes in the interface.
  + Title loader is now **able to load the save file** for a certain title
    **from** both the actual save archive and **the SD card**.
    + This means that you're now able to load *any* save file for a certain
      game, provided they're located in a folder specified in the configuration
      file (more about this later).
    + PKSM automatically adds Checkpoint's working path to the list of valid
      folders to look for save files. This means you'll be able to directly edit
      your save backups you made with Checkpoint.
    + While the main title loader interface lets you only work with save files
      you have games for, *how do you work with save file you don't actually
      have games in your console*? You're welcome, we thought about this too.
      Title loader now has an additional tab in which you can choose save files
      for not directly available titles, giving you the opportunity to edit
      whatever you want. What has already been said about SD card save backup
      loading also applies to this menu.
  + It's now possible to return to the title loader once you're done making
    changes to a save file, instead of having to close PKSM and boot it back up.
  + You can edit PKSM's configurations by pressing `SELECT`, which will let you
    enter the configuration menu before the main menu is even loaded. More about
    configurations will be said later.
  + The title loader has now a **wireless loading** feature, with which you can
    load a save file from your local network, that can be sent back when you're
    done applying your changes. 
    + This directly allows you to load LGPE saves over the network using
      [Checkpoint](https://github.com/FlagBrew/Checkpoint/releases) for Switch.
    + **Warning**: once you enter the wireless loader (you'll be prompted before
      entering it, to make sure you don't misclick on it) you won't be able to
      return back without receiving a save file. If you're in this situation and
      don't know what to do, rebooting your console will be mandatory.
+ The **Storage** functionality has been completely redesigned. It's now
  possible to browse Storage for every game, from DPPt to USUM.
  + It's now possible, among the already available functionalities, to clone
    from the Storage itself.
  + **Transfer through generations** is available. This means you can perform a
    complete Gen4 <-> Gen7 swap in freedom.
    + Transfer through generations needs to be specifically allowed from the
      configurations, because it changes the original pokemon data. The option
      to allow is `Edit during transfers` (or the equivalent in the language
      you're familiar to).
    + Please note that conversions are still experimental and there will be edge
      cases not covered. Please always make a save/storage backup using
      Checkpoint.
  + **Box renaming** is available for both save and storage boxes.
  + Storage size expansion has been enabled once again. Default Storage size is
    still 150 boxes, but the upper limit is now set to **2000**.
+ The **Editor** has been completely redesigned as well.
  + Supports every game from DPPt to LGPE.
  + Hex Editor and QR Code scanner are also available from DPPt to USUM.
  + **Party editing is now possible**.
  + Cloning is possible by pressing `X`.
  + Box renaming is available here, too.
  + You're now prompted if you're exiting a pkm edit session without saving.
+ A **Bag editor** has been included. You can now edit most sections of your
  bag, for every game supported by PKSM.
  + You're allowed to change the item by tapping on the dedicated button, and
    increase or decrease the amount using the touch screen.
  + Dynamic addition and removal of items is also available.
+ The **Event database** has been finally updated to include all the latest
  wondercards. 
  + New wondercards will be automatically fetched from the EventsGallery when
    you compile PKSM from scratch, or we push a new release. This means manual
    work is not required anymore to include wondercards.
  + Dumping wondercards you own is possible by pressing `X` in the event list.
  + QR code wondercard injection is available as well, and has been extended to
    support all wondercard formats from Gen4 to Gen7.
+ **Scripts** have been greatly enhanced as well. We'll be short but this
  describing the latest changes would require a dedicate changelog.
  + Default scripts are now built-in. Thanks to the recent work by many
    scripters, there are now more than **800** scripts available for everyone to
    use.
    + This means every new PKSM update will always contain default scripts.
      Placing default ones in the SD card is not required anymore.
  + Support for **scripts written in C**. PKSM now has a built-in **C
    interpreter** which will allow you to write more complex scripts directly in
    C.
    + This directly means you're allowed to use standard library functions in
      scripts, create variables, doing math operations and work with pointers.
      Good stuff.
    + Some APIs you can use in your scripts are directly provided by PKSM, and
      they focus on user interaction, keyboard access and utility functions.
    + You can always request some more if you're about to develop a new script
      which requires a functionality not yet available from PKSM.
  + Support for folders. Scripts are now organized in folders, to make the UI
    more elegant and accessible.
  + You're allowed to place your own scripts into the SD card and switch between
    built-in and your own from the UI itself.
+ The **QR code** scanner has been vastly improved to allow for a smoother
  experience. This will allow you to scan big QR codes faster then ever before.
+ The **Configuration** menu has been vastly redesigned to get rid of the
  unintuitive *hex editor-like* user interface.
  + Interface language can be changed in real time.
    + This version of PKSM ships with English, Italian, Spanish, French and
      German already supported. Other languages haven't been contributed by
      native speakers yet, and we hope to support more languages in the next
      updates.
  + Default values like the `OT Name` can now be edited through the keyboard.
  + Some more options that affect all the other section of the application are
    available.
+ **PKSM now relies on extdata** to store important data, like the storage and
  configurations.
  + This means you can now use Checkpoint to make a proper and easy backup of
    your relevant PKSM data, like the whole storage file and configurations.
    Sharing your data is now more convenient, too.
  + You're also allowed to move the storage data to the SD card.
  + **Warning**: proper extdata support for PKSM is only granted from Checkpoint
    **3.6.0** and above. Don't try restoring a PKSM save backup with a version
    inferior to the one suggested.
+ **Support for \*hax** (also known as *just homebrew*) has been dropped. PKSM
  only works under a Luma3DS environment.
  + Specifically, PKSM will check for the `hb:ldr` port, which is available, for
    example, through the Rosalina system module.
  + If you try booting PKSM under *hax, it will return to the homebrew launcher.
  + This also means PKSM will not work in Citra.
+ **PKSM now supports audio playback**. Audio will start once boot is completed.
  + There actually is no way to stop the audio playback, so turn down the volume
    in case it starts getting annoying.
  + It's possible to provide your own audio soundtracks by placing `.mp3`
    files in the `sdmc:/3ds/PKSM/songs` folder.
  + It's suggested to provide relatively small sized files. We generally suggest tracks
    with a length of no more than 4 minutes, single channel, with a sampling
    frequency of 44100Hz and a bitrate of 96kbps.
    + These characteristics are not actual constraints: you can throw whatever
      you want into the SD card and PKSM will mostly be able to work with it fine.
    + You can convert a track to get these characteristics through
      [ffmpeg](https://stackoverflow.com/questions/3255674/convert-audio-files-to-mp3-using-ffmpeg).
+ A *search* function for species has been implemented so you're now able to
  search a Pokemon with your keyboard rather than scrolling the entire list
  while generating one or changing the species.
+ Tons of other stuff. Really *everything* changed from the latest release.
+ Last but not least, a whole **documentation** in `.html` format explaining in
  details how PKSM works! The documentation will be bundled with every release,
  and the most up-to-date version will always be accessible through the *Github
  wiki for this project.
