# souldew-3ds – status report

State of 14 Nov 2026 (beta packaging – first section; 13 Nov: redesign, title menu, introduction, achievements, settings). Everything below was built with `./build.sh` (Docker toolchain) and tested on a Mac
(Apple Silicon) with Azahar 2126.2 – **no Pokémon game was available**, so nothing has run inside a real Pokémon
game yet.

## 14 Nov 2026: beta packaging (0.9.0-beta.1)

| What | Where |
| --- | --- |
| **Distribution**: Universal-Updater with souldew's own UniStore (recommended: one QR code, updates in the app, installs the `.cia` *and* the `.3gx`), FBI remote install of the `.cia` by QR code, or an SD card zip – all from one GitHub release with the fixed tag `beta` (stable URLs, so posted QR codes keep working) | `release/souldew.unistore.template`, `tools/make_release.py`, `docs/TESTERS.md`, `docs/RELEASING.md` |
| **`.cia`** of the launcher: title ID `000400000F5D3D00`, product code `CTR-H-SDEW`, HOME menu icon + banner (rings + wordmark) + a soft synthesised 1.3 s chime; makerom/ctrtool/bannertool pinned in the Docker image | `launcher/cia/`, `launcher/Makefile` (`make cia`), `tools/gen_cia_assets.py` |
| **The app installs its plugin**: the `.3gx` sits in the RomFS of the `.cia` and the `.3dsx`; every start writes it to `/3ds/souldew/souldew.3gx` when missing or different (`.tmp` + rename) – one file is a complete install | `syncPlugin()` in `launcher/source/main.cpp` |
| Version `0.9.0-beta.1` (`kVersion`, shown on the title screen and in Settings; the `.cia`'s title version 0.9.0) | `common/screens.h` |
| Plugin build: header dependencies now tracked (`DEPSDIR`), so a changed header rebuilds the plugin | `plugin/Makefile` |

Verified: `ctrtool --verify` on the `.cia` – exheader, ExeFS, RomFS and content hashes good (ticket/TMD signatures
are fake, as for every homebrew CIA – Luma3DS' signature patches accept them); ExeFS has code, icon (SMDH "souldew",
region free, flags visible/allow3d/recordusage), banner (CBMD with CWAV), logo; the RomFS's `souldew.3gx` is byte-identical
to the plugin. The `.3dsx` carries the same RomFS. In Azahar 2126.2: the launcher demo **and** the `.cia`'s program
(its NCCH booted directly) each started on an SD card without the plugin and wrote `sdmc:/3ds/souldew/souldew.3gx`,
identical to the build (`docs/screenshots/emu_cia_intro.png`). The UniStore is valid JSON in the format Universal-Updater's
source checks (`version` 3, int `revision`, `file` without a slash); both QR codes decode back to their URLs.

Not testable here: installing the `.cia` with FBI / Universal-Updater on a console (no HOME menu, FBI or camera in
this emulator setup), the HOME menu banner and its sound, Universal-Updater actually reading the store, the download URLs (nothing
is uploaded yet).

## 13 Nov 2026: prettier and more complete

| What | Where | Screenshots |
| --- | --- | --- |
| **Look**: a backdrop with the website's blue/pink glows, surfaces with vertical gradients, a light top rim and soft shadows, segmented tabs (with open / dead counts), pixel icons (`drawIcon`), the logo as the favicon's pixel art (rings in P1 blue / P2 pink), buttons like the website's play button, key caps, switches, badge pips, dashed empty slots, the desktop app's "HP" label, type tiles in both types' colours (no sprites), cartridges in the two versions' colours (no logos), list rows with status discs and section labels, a bottom sheet with a grabber | `common/ui.cpp`, `common/gfx.cpp` (`roundRectVGradient`, `glow`, `shadow`, `dashedRoundRect`) | every `launcher_*`, `ingame_*`; before: `docs/screenshots/before/` |
| **Title menu**: an LCD with the rings linking up (0.9 s), the pixel wordmark (`font_title`, a 46 px Pixelify subset), a line about what is focused; Continue (starts the active run's game), New run, Runs, Achievements, Settings, Help, Quit – D-pad, A and touch; screens slide + fade into each other (0.22 s) | `MainMenu`, `transition()`, `launcher/source/main.cpp` | `launcher_menu*`, `launcher_transition`, `emu_launcher_menu*` |
| **Introduction** on first launch (`settings.onboarded`): language (DE/EN), what souldew does, opening it in game (SELECT / the corner tab), the Nuzlocke rules, ready – skippable; again under Help and Settings | `Onboarding` | `launcher_intro_1…5`, `emu_launcher_intro_*` |
| **Achievements**: 63 of the desktop app's trophies (ids, rarities, de/en texts) for what the plugin can observe; medals per category + the champion's (the desktop app's computed pixel medals, cell by cell); coins by rarity; progress, unlock date, secret ones; stored in the run file's `profile` (kept when a run is deleted); the plugin shows a card when one unlocks, the launcher a banner | `common/trophies.cpp`, `TrophyScreen`, `drawMedal`, `drawCoin` | `launcher_trophies*`, `ingame_trophy`, `launcher_trophy_banner`, `emu_plugin_trophy`, `emu_launcher_trophies*` |
| **Settings**: language, overlays, corner tab, the introduction, about – each with a preview on the top screen | `SettingsScreen` | `launcher_settings*`, `emu_launcher_settings*` |
| **End a run** as won / lost (or reopen it) in the Rules tab – wins and losses count for achievements | `RunView` sheet 4 | `launcher_run_rules_end`, `launcher_run_end` |
| **Moments** the engine now records: badge times, evolutions (with a card), exactly 1 HP, badges earned underlevelled, trainer battles started over the cap, a monotype team of six, each death's killer (species, level, trainer) | `common/nuzlocke.cpp`, `RunStats` | `ingame_evolved` |
| Faster drawing for the ARM11 (no divide instruction): blending and glows without divisions per pixel, backdrops cached in the launcher | `common/gfx.cpp`, `drawBackdrop` | – |

Medals follow the desktop app's centring fix (its commit 002c367): a 25×32 grid with the body centred on a whole cell
(12, 20), so it has a middle column; ribbon tails at 4…10 / 14…20; emblems trimmed to their ink and placed with
`CX − ceil((w−1)/2)` – a test checks every kind and tier (mirror-symmetric body, ink centre at most half a cell up/left).
The achievement coins (13×13, centre 6, 6) place their emblems the same way.

Achievements left out (not observable on the 3DS or not for a solo run): SoulLink, randomizer, cheats, nicknames,
"seen" species, Gen 1–5 dexes. Adapted: badges in Alola = grand trials (two per trial for "Eight for Eight"), "Kalos
and Alola" instead of "all generations".

## Architecture (as built)

| Part | What | Why |
| --- | --- | --- |
| `launcher/` → `souldew.3dsx` | libctru app: runs, new-run wizard (game → rules → name), run screens (team, routes, graveyard, rules), starting the game | Homebrew Launcher is what every Luma user has |
| `plugin/` → `souldew.3gx` | CTRPluginFramework plugin: memory reader (4×/s), Nuzlocke engine, overlay cards on the game's top screen, souldew screens over the paused game | the only way to show things *inside* a retail game on CFW |
| `common/` | everything platform-free: software canvas + bitmap fonts, the UI (theme, components, screens), run file (JSON), rules engine, PK6/PK7 crypto, party/battle/zone readers, RomFS/GARC/LZ11 | one drawing code for launcher, plugin **and** the computer, so screenshots and tests use the real code |
| `mockgame/` → `souldew-mock.cxi` | test title (ID `000400000F5D0000`) with Pokémon X 1.0's memory layout and a scripted Nuzlocke | the plugin can be tested end to end without game files |

Decisions worth knowing:

- **Software canvas instead of citro2d.** The plugin cannot use the GPU (it draws into the game's framebuffer), so
  its drawing must be CPU code anyway; the launcher uses the same code → identical look, and the host renderer
  produces pixel-exact screenshots of both. Screens are redrawn only on change.
- **Start with the launcher, plugin via plg:ldr.** The launcher calls Luma's `plg:ldr` `SetPluginLoadParameters`
  (plugin `/3ds/souldew/souldew.3gx`, the chosen title) and jumps to the game (`APT_DoApplicationJump`). Alternative
  without launcher: the plugin in `/luma/plugins/<TID>/`.
- **Opening souldew in the game:** SELECT (CTRPF's menu hotkey; `OnOpening` opens souldew instead of CTRPF's menu)
  or holding the small tab in the bottom-right corner ~0.2 s. The game pauses (`Process::Pause`), souldew draws both
  screens, "Back to the game" (or B/SELECT) resumes.
- **Run file** `sdmc:/souldew/runs.json` (rule names like the desktop app's: `dupes`, `levelCaps`, `blackout`,
  `gifts`, `deathless` …), written atomically (`.tmp` + rename) – by the plugin at most every 3 s, right away after a
  death/catch, and on HOME/sleep/exit.
- **Places:** the plugin reads the zone table from the running game's own RomFS (FS `ARCHIVE_ROMFS`, the same
  layouts as the desktop app's `ctrzones.mjs`) and caches it in `sdmc:/souldew/cache/`. Areas are keyed by the
  English location name (PKHeX texts), shown in German or English.
- **Overlay cards** never trail the game: a new moment shortens the current card and drops waiting unimportant ones
  (new area, info); encounters, catches, deaths, caps and badges are never dropped.
- **Design:** souldew's "handheld at night" tokens (shell/ink/LCD colours, tri/cir/crs/warn accents), Pixelify Sans
  (headings), Silkscreen (labels, numbers), IBM Plex Sans (text) rasterised to 4-bit bitmap fonts; cards, chips,
  stat tiles, tab bar and buttons after the approved mobile mockups, adapted to 400×240 + 320×240. No Pokémon
  sprites (not redistributable) – a type-tinted tile with a short name instead.

## What works – and how it was verified

| Feature | Verified by |
| --- | --- |
| Reproducible build (`./build.sh`): `souldew.3dsx`, `souldew.3gx`, test builds, SD layout in `dist/` | Docker image pinned (devkitARM by digest, 3gxtool commit, makerom tag); `build/TOOLCHAIN.txt` records libctrpf 0.8.0.r1450-1, libctru 2.7.0, devkitARM r68 |
| PK6/PK7 decryption/parsing (species, level, HP, status, nickname, met location, gender, shiny, egg) | 5 records encrypted by the **desktop app's own `gen67.mjs`** – the C++ port reads exactly what `pk6Mon()` reads; re-encryption round trip |
| Party reader (address per update version, shrink-needs-two-reads, Bad Egg skip), battle reader (Gen 6 wild/trainer bases, Gen 7 flags) | unit tests with a fake memory map |
| Zone tables (NCCH/IVFC/RomFS → GARC → LZ11 → places) | unit test; the desktop app's `ctrzones.mjs` reads the same synthetic image to the same places; **in Azahar the plugin read the mock game's RomFS zone table (6 zones)** |
| Nuzlocke engine: starter adoption, new area, first encounter, dupe (species/line), catch, gift rule, missed encounter, over-cap warnings (once per level, again at trainer battles), death from the battle copy, badge → new cap, whiteout ends the run, deathless | unit tests (scenario) **and the emulator run below** |
| Run file round trip, unicode, desktop-style `levelCaps: true` | unit tests |
| Framebuffer blitting in all 5 GSP formats, rotated layout, alpha | unit tests; in Azahar on RGB565 (top) and BGR8 (bottom) |
| **Launcher in Azahar** (introduction, title menu, runs, run tabs, achievements, settings, new-run wizard) | `tools/emu_test.py launcher` with the self-driving demo build (it presses its own buttons on a time schedule) → `docs/screenshots/emu_launcher_*.png`, 20 shots |
| **Plugin end to end in Azahar** (Azahar's 3GX plugin loader, CTRPF, memory reads, engine, overlays on the game's screen, corner tab, souldew screens over the paused game, run file written) | `tools/emu_test.py plugin`: the mock game plays town → Route 2 → wild Fletchling (first encounter) → catch → forest → Fletchling again (dupe) → Froakie over cap 12 → trainer battle (cap badge) → Fletchling goes down (death) → badge 1 (cap 25) → achievements unlocked, their cards shown ("Blech am Revers" in `emu_plugin_trophy`). The plugin's run file afterwards: starter + Route 2 caught + forest dupe, Fletchling dead ("Purmel Lv 6 (Trainer)", killer 664), 1 badge with its time, 1 cap break, `profile.trophies` firstRun / firstCatch / firstBadge → `docs/screenshots/emu_plugin_*.png`, `docs/emu_plugin_runs.json` |
| All screens in German and English | `docs/screenshots/launcher_*.png`, `ingame_*.png` (`*_en.png`): rendered on the computer by the console's drawing code; overlays composited through the plugin's blitter onto a placeholder picture |

Unit tests: 345 checks, all passing (`./build.sh host`) – new: the engine's moments, achievements (unlock, once, kept in the profile, medals, run-file round trip, old files), the launcher screens' keys and taps, the transition.

## Needs a real console (and the games)

1. **Inside the real games.** The memory addresses come from the desktop app (checked there with retail games in
   Azahar), but the plugin itself has only run inside the mock game. On a console: X/Y and OR/AS (1.0 and the last
   update), Sun/Moon, Ultra Sun/Ultra Moon – does the team appear, do the cards come at the right moments?
2. **CTRPF's hooks in the real games.** CTRPF finds a game's frame and GSP functions by code patterns (fine for retail
   games – that's what it is made for). The mock game had to carry copies of those SDK patterns
   (`mockgame/source/sdk_gsp.s`) and the test build reads buttons through its own HID session – so the emulator run
   proves souldew's code, not CTRPF's pattern search in Pokémon.
3. **SELECT and the corner tab** with real input (the emulator test opened souldew through the same code path,
   `PluginMenu::ForceOpen`, by itself – Azahar 2126.2's RPC can't press buttons). Also: whether SELECT clashes with
   anything in Gen 7, and whether the corner tap disturbs the game's own touch screen (PSS in X/Y).
4. **The launcher's game start:** `plg:ldr` (Luma3DS v13+) and `APT_DoApplicationJump` from the Homebrew Launcher –
   no installed games in the emulator, so the jump wasn't exercised; on a console test cartridge and digital copy,
   and the "plugin loader is off" dialog.
5. **RomFS from the plugin in retail games** (works in Azahar with the mock's RomFS; with LayeredFS mods the modded
   tables would be read).
6. **Performance:** the mock game ran at ~10 fps in Azahar because it redraws itself on the CPU; the plugin's own
   per-frame work is one cached card blitted (≤ 300×60 px). Check on an Old 3DS that the game doesn't stutter.
7. **3D slider on** (the plugin draws into both eyes' buffers – untested), sleep / HOME menu with souldew open.
8. **The launcher's animations on real hardware**: the title animation, screen changes and banners redraw every frame
   with the CPU (time-based, so a slow frame only skips steps). In Azahar they run smoothly; on an Old 3DS the first
   draw of each backdrop and the medal case may take a few frames – check.
9. **Achievements in real games**: the moments (badge times, 1 HP, evolutions, killers) come from the same memory reads
   as before, but only the mock game has exercised them. The unlock date on the console is the 3DS clock (shown as
   a date); runs written on a computer show no date.

## Known gaps

- **Virtual Console (GB/GBC/GBA) not supported.** GBA VC runs in AGB_FIRM (no plugins at all); GB VC would need
  memory addresses inside Nintendo's GB emulator process, which nobody has documented. The launcher lists only the
  four 3DS game families.
- **Box catches:** with a full party the catch goes to the PC, which souldew can't read; the card asks to confirm it
  in souldew's *Routes* tab (open encounter → *Caught* / *Missed*). Otherwise an open encounter becomes *missed*
  when you walk on without a new team member.
- **Shiny clause, "no items", "set mode"** are honour-system rules (the battle copy has no PID; items aren't read).
- **Gen 7 has no badges in memory** (not documented): the player steps the grand trials (− / +) in *Rules*. The SM/USUM
  caps (kahuna aces, then champion) are vanilla values as a starting point and less certain than the X/Y and OR/AS
  badge caps; they can be overridden per run (`rules.caps` in the run file).
- Gifts/statics are recognised by their met location; trades and hatched eggs never use up an area.
- No rule editor after creating a run (except Gen 7 milestones and the overlay switch); no import/export with the
  desktop app yet (field names match, so it is a small step).
- Language is switched in the launcher (Settings, introduction); in game only the overlays can be switched.
- "Won" is set by hand (Rules › End the run): souldew can't see the Hall of Fame. Shiny clause / items / set mode stay
  honour-system rules.
- The system keyboard for the run's name (Y / tap on the name field) wasn't exercised in the emulator demo.
- The Pokémon tiles are type colours with short names, not sprites (sprites aren't redistributable).
- Long card texts are cut with "…" (e.g. the dupe hint while the battle badge is shown).
