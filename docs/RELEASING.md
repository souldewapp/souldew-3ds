# Publishing a beta (for the owner)

Testers get souldew three ways (docs/TESTERS.md): **Universal-Updater** with souldew's own UniStore (recommended –
one QR code, updates show up in the app), **FBI** remote install of the `.cia` by QR code, or the **SD card zip**.
All three download from one place: a GitHub release whose assets keep the same names. Nothing below has been
published yet.

## 1. Build

```sh
./build.sh                      # tests + Docker toolchain → dist/
```

`dist/release/` then holds exactly the files to upload (the names matter, the URLs depend on them):

| File | Used by |
| --- | --- |
| `souldew.unistore` | Universal-Updater (the store; its QR code points here) |
| `souldew.t3x` | Universal-Updater (the store's icon) |
| `souldew.cia` | Universal-Updater, FBI QR code |
| `souldew.3dsx` | Universal-Updater (Homebrew Launcher entry) |
| `souldew.3gx` | Universal-Updater (the plugin; the app also carries a copy and installs it itself) |
| `souldew-3ds-<version>.zip` | manual download (SD card layout + README_TESTERS.txt) |

`dist/qr/` holds the two QR codes for the Discord post (see 3).

## 2. Where to host: a release with the fixed tag `beta`

Recommended: a public repository **`souldewapp/souldew-3ds`** with this code (GPL-3.0: the source has to be
available next to the binaries) and one release whose tag is **`beta`**. Its asset URLs never change:

```
RELEASE_BASE = https://github.com/souldewapp/souldew-3ds/releases/download/beta
```

That is the default in `tools/make_release.py` – the UniStore and the QR codes in `dist/` already use it. Each new
beta replaces the assets of the same release, so the QR codes posted once keep working, the release may be marked
*pre-release*, and the desktop app's releases in `souldewapp/souldew` stay as they are (keep the 3DS files out of
that repository's releases, so nothing that reads them – the desktop app's update check included – sees them). (`releases/latest/download/…` would also be stable, but
skips pre-releases.)

Somewhere else? Build with that base instead (no trailing slash) – it replaces `{{RELEASE_BASE}}` in
`release/souldew.unistore.template`:

```sh
SOULDEW_RELEASE_BASE=https://github.com/<owner>/<repo>/releases/download/<tag> ./build.sh console
# or, without rebuilding the 3DS files:
python3 tools/make_release.py --base https://github.com/<owner>/<repo>/releases/download/<tag>
```

## 3. QR codes

`tools/make_release.py` draws them when the Python package `qrcode` is installed – it isn't a dependency of the
build, so use a throwaway venv:

```sh
python3 -m venv /tmp/souldew-qr && /tmp/souldew-qr/bin/pip install "qrcode[pil]"
/tmp/souldew-qr/bin/python tools/make_release.py --qr-only --base https://github.com/souldewapp/souldew-3ds/releases/download/beta
```

- `dist/qr/souldew-universal-updater-qr.png` → `<RELEASE_BASE>/souldew.unistore` (Universal-Updater › Settings ›
  Select UniStore › Add › Add with QR code)
- `dist/qr/souldew-fbi-cia-qr.png` → `<RELEASE_BASE>/souldew.cia` (FBI › Remote Install › Scan QR Code)

## 4. Upload (first beta)

1. Create `souldewapp/souldew-3ds` on GitHub (public) and push this repository's `main` there.
2. Create the release: tag **`beta`**, title e.g. "souldew 3DS beta", tick *pre-release* if you like, and attach
   all six files from `dist/release/`. With the GitHub CLI:

   ```sh
   gh release create beta --repo souldewapp/souldew-3ds --prerelease --title "souldew 3DS beta" \
     --notes "Beta 0.9.0-beta.1 – see docs/TESTERS.md" dist/release/*
   ```
3. Check that every URL answers (a redirect to `objects.githubusercontent.com`, then 200):

   ```sh
   for f in souldew.unistore souldew.t3x souldew.cia souldew.3dsx souldew.3gx; do
     curl -sIL -o /dev/null -w "%{http_code} $f\n" https://github.com/souldewapp/souldew-3ds/releases/download/beta/$f
   done
   ```
4. Discord: post docs/TESTERS.md in the beta channel (Discord messages are limited to 2000 characters – a forum post
   split at the `---` lines works), attach both PNGs from `dist/qr/`, and give the UniStore link
   (`…/beta/souldew.unistore`, for typing it in) and the zip link (`…/beta/souldew-3ds-<version>.zip`).

## 5. Every next beta

1. Raise `kVersion` in `common/screens.h` (e.g. `0.9.0-beta.2`) – the app, the `.cia`'s title version, the zip's
   name and the UniStore take it from there. Optional: `RELEASE_NOTES` in `tools/make_release.py`.
2. `./build.sh` – the UniStore's `revision` is the build's UTC hour (`YYYYMMDDHH`); two uploads in the same hour:
   `python3 tools/make_release.py --revision <bigger number>`.
3. Replace the assets: `gh release upload beta --repo souldewapp/souldew-3ds --clobber dist/release/*` and delete the
   previous zip (`gh release delete-asset beta souldew-3ds-<old version>.zip --repo souldewapp/souldew-3ds`).
   Universal-Updater shows the update, the FBI QR code installs the new `.cia`, the app replaces the plugin on the
   SD card with its own copy on the next start.

The `.cia`'s title ID is `000400000F5D3D00` (product code `CTR-H-SDEW`) – keep it, or testers end up with two icons.
