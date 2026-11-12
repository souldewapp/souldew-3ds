# souldew-3ds – status report

State of 12 Nov 2026. Everything below was built with `./build.sh` (Docker toolchain) and tested on a Mac
(Apple Silicon) with Azahar 2126.2 – **no Pokémon game was available**, so nothing has run inside a real Pokémon
game yet.

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
| **Launcher in Azahar** (home, run tabs, rules, new-run wizard incl. rule toggles) | `tools/emu_test.py launcher` with the self-driving demo build → `docs/screenshots/emu_launcher_*.png` |
| **Plugin end to end in Azahar** (Azahar's 3GX plugin loader, CTRPF, memory reads, engine, overlays on the game's screen, corner tab, souldew screens over the paused game, run file written) | `tools/emu_test.py plugin`: the mock game plays town → Route 2 → wild Fletchling (first encounter) → catch → forest → Fletchling again (dupe) → Froakie over cap 12 → trainer battle (cap badge) → Fletchling faints (death) → badge 1 (cap 25). The plugin's run file afterwards: starter + Route 2 caught + forest dupe, Fletchling dead ("Purmel Lv 6 (Trainer)"), 1 badge → `docs/screenshots/emu_plugin_*.png` |
| All screens in German and English | `docs/screenshots/launcher_*.png`, `ingame_*.png` (`*_en.png`): rendered on the computer by the console's drawing code; overlays composited through the plugin's blitter onto a placeholder picture |

Unit tests: 147 checks, all passing (`./build.sh host`).

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
- `settings.lang` (de/en) only in the run file – no switch in the UI yet.
- Long card texts are cut with "…" (e.g. the dupe hint while the battle badge is shown).
