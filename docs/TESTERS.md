# souldew for 3DS – beta testers' guide

Thanks for testing! souldew is a **Nuzlocke tracker that runs on the console itself**: a small app for your runs
(rules, team, routes, graveyard) and a **Luma3DS plugin** that follows the game while you play – new areas, first
encounters, dupes, catches, Pokémon going down, level caps, badges.

Games: **Pokémon X / Y, Omega Ruby / Alpha Sapphire, Sun / Moon, Ultra Sun / Ultra Moon** – your own cartridge or
eShop copy, any region. Virtual Console games (Red/Blue/Yellow, Gold/Silver/Crystal) are not supported.

---

## What you need

- A 3DS / 2DS (old or new) with **custom firmware: Luma3DS 13 or newer** – best the newest (v13.5). Not sure? Hold
  **SELECT** while turning the console on: the Luma3DS configuration screen shows the version at the top.
  (Set up with the usual guide at 3ds.hacks.guide? Then you already have everything below.)
- **Universal-Updater** or **FBI** (both come with that guide), or a computer with an SD card reader.
- Wi-Fi for the first two install ways.

---

## Install – pick ONE way

### A) Universal-Updater + QR code (recommended – updates are one tap)

1. Open **Universal-Updater**.
2. Sidebar › **Settings** (cogwheel) › **Select UniStore** › the **Add** icon (bottom right) › **Add with QR code**.
3. Scan the **Universal-Updater QR code** from the Discord post. (No camera? Use **Add with keyboard** and type the
   UniStore link from the post.)
4. Pick the **souldew (beta)** store, select **souldew**, open the **Downloads** tab (cloud icon) and choose
   **souldew.cia (HOME menu, recommended)**.
   It installs the app *and* puts the plugin where it belongs (`/3ds/souldew/souldew.3gx`).
5. Back on the HOME menu, souldew has its own icon (new titles show up wrapped as a present – tap it to unwrap).

> Prefer the Homebrew Launcher? Choose **souldew.3dsx (Homebrew Launcher)** in step 4 instead.

### B) FBI + QR code (no store, just the app)

1. Open **FBI** › **Remote Install** › **Scan QR Code**.
2. Scan the **FBI QR code** from the Discord post and confirm. FBI downloads and installs `souldew.cia`.
3. Start souldew once from the HOME menu – it **installs its plugin by itself** on the first start
   (`/3ds/souldew/souldew.3gx`; the app carries a copy). Nothing to copy by hand.

### C) SD card zip (no Wi-Fi needed)

1. Download `souldew-3ds-<version>.zip` from the Discord post / release page.
2. Turn the console off, put the SD card in your computer, and copy the folders **`3ds`** and **`cias`** from the zip
   to the **root** of the SD card (merge with the folders already there).
3. SD card back in, console on: **FBI › SD › cias › souldew.cia › Install CIA** (then you can delete the `.cia`).
   No FBI? Start souldew from the **Homebrew Launcher** instead – same app.

---

## Turn on the plugin loader (once)

Press **L + D-Pad Down + SELECT** to open the **Rosalina menu**, select **Plugin Loader: [Disabled]** and press **A**
so it reads **[Enabled]**, then **B** to leave. It stays on after a restart.
(souldew also offers to switch it on when you start a game.)

---

## Play

1. Open **souldew** (HOME menu icon, or the Homebrew Launcher).
2. The short introduction explains everything (language, how it works). Then **New run** › pick your game and
   rules › name it.
3. Press **Start game** – souldew starts your cartridge or eShop game with the plugin.
4. In game nothing changes until something matters: then a small card appears on the top screen.
   **SELECT** – or **holding the small tab in the bottom screen's corner** – pauses the game and opens souldew
   (team, routes, graveyard, rules). **B**, **SELECT** or the big button returns to the game.

Always start the game **through souldew** – that's what tells Luma to load the plugin.

---

## What to test

Everything is welcome, but these help most (nothing has run inside the real games on real hardware yet):

- **Install**: did your way (A, B or C) work first time? Does the HOME menu icon/banner look right?
- **Start game**: cartridge *and* eShop/digital copies, each game you own, old (1.0) and updated versions.
- **Cards in game**: new area, first encounter, catch, dupe, a Pokémon going down, over the level cap, badge / new
  cap, evolution, whiteout, achievements. Do they come at the right moment? Anything missed or wrong?
- **Your team** in souldew's screens: right Pokémon, levels, HP?
- **SELECT / corner tab**: does it open souldew? Does it clash with the game (e.g. SELECT in Sun/Moon, or the
  touch screen in X/Y)?
- **Old 3DS / 2DS**: does the game stutter or slow down with the plugin?
- **Edge cases**: 3D slider on, closing the lid (sleep), the HOME button while souldew is open, a full party (catch
  goes to the PC).

---

## How to report

Post in the **#beta-feedback** forum on the souldew Discord – one post per problem, please. Include:

```
Console:   (Old/New) 3DS / 3DS XL / 2DS / 2DS XL
Luma3DS:   v13.x
Game:      e.g. Pokémon Y, update 1.5, cartridge (or eShop), region
Install:   A (Universal-Updater) / B (FBI) / C (zip)
What I did:
What I expected:
What happened:
```

A **photo of the screen** helps a lot. If souldew shows an error code (e.g. `plg:ldr 0x…` or `APT 0x…`), write it
down. For tracking problems, attach `sdmc:/souldew/runs.json` (your runs) if you're fine sharing it.

---

## Known limitations

- **Beta**: tested in an emulator with a test title, **not yet in the real games on a real console** – expect rough
  edges and tell us about them.
- **Box catches**: with a full party souldew can't see the PC; the card asks you to confirm the catch in souldew's
  *Routes* tab.
- **Sun / Moon / US / UM** have no badges: step the grand trials yourself in *Rules* (− / +); the level cap follows.
- Shiny clause, "no items" and "set mode" are honour-system rules (souldew can't check them).
- "Won" is set by hand (*Rules › End the run*) – souldew can't see the Hall of Fame.
- Language (Deutsch / English) is switched in the app's *Settings*, not in game.
- No sync with the souldew desktop app yet. No Pokémon sprites (type-coloured tiles instead).

---

## Update

- **Universal-Updater**: it shows when a new beta is out – open souldew in the store and run the same download again.
- **FBI**: scan the same FBI QR code again (it always points at the newest beta), then start souldew once.
- **Zip**: copy the new zip's folders over the old ones and install the `.cia` again.

Your runs, settings and achievements (`sdmc:/souldew/`) are kept.

## Uninstall

1. Delete the app: **System Settings › Data Management › Nintendo 3DS › Software › souldew › Delete**
   (or **FBI › Titles › souldew › Delete Title**). Using the Homebrew Launcher version? Skip this.
2. Delete the folder **`/3ds/souldew/`** on the SD card (app + plugin).
3. Optional: delete **`/souldew/`** (your runs and achievements), remove the store in Universal-Updater
   (Settings › Select UniStore › Remove) and turn the plugin loader off in the Rosalina menu.

---

souldew is free and open source (GPL-3.0) and not affiliated with or endorsed by Nintendo, Game Freak or The Pokémon
Company.
