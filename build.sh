#!/bin/sh
# Builds souldew-3ds reproducibly: the toolchain runs in a Docker image (devkitARM pinned by digest, 3gxtool and
# makerom/ctrtool/bannertool pinned by commit/tag, libctrpf from ThePixellizerOSS' package database), the host tests
# and screenshots with the computer's C++ compiler.
#
#   ./build.sh            everything → dist/ (SD card layout, beta release files) + tests + docs/screenshots
#   ./build.sh console    only the 3DS files (+ dist/release: .cia, .3dsx, .3gx, UniStore, zip; QR codes if possible)
#   ./build.sh host       only tests + screenshots
#
# The release's download URL (UniStore, QR codes): SOULDEW_RELEASE_BASE=https://… ./build.sh console
#
# Needs Docker and a C++17 compiler (clang or gcc). No sudo.
set -eu
cd "$(dirname "$0")"
ROOT=$(pwd)
IMAGE=souldew-3ds-build
what=${1:-all}

console() {
  docker image inspect "$IMAGE" >/dev/null 2>&1 || docker build -t "$IMAGE" docker
  run() { docker run --rm -v "$ROOT:/src" -w "/src/$1" "$IMAGE" sh -c "$2"; }
  run plugin "make && make test"
  # the plugin travels inside the launcher (RomFS): the launcher installs it on the SD card
  mkdir -p launcher/romfs
  cp plugin/souldew.3gx launcher/romfs/souldew.3gx
  run launcher "make && make demo && make cia"
  run mockgame "make"
  # Universal-Updater's icon sheet (48×48 launcher icon) and the .cia's structure (ctrtool) for the record
  printf -- '--atlas -f rgba -z auto\n\n../launcher/icon.png\n' > build/souldew.t3s
  docker run --rm -v "$ROOT:/src" -w /src/build "$IMAGE" sh -c 'tex3ds -i souldew.t3s -o souldew.t3x'
  docker run --rm -v "$ROOT:/src" -w /src "$IMAGE" sh -c 'ctrtool --info launcher/souldew.cia' > build/cia_info.txt 2>&1 || true
  # the .cia's program (NCCH) on its own, for the emulator test (tools/emu_test.py cia)
  docker run --rm -v "$ROOT:/src" -w /src/build "$IMAGE" sh -c 'ctrtool --contents=cia_content ../launcher/souldew.cia >/dev/null 2>&1; mv cia_content.0000.* souldew-cia.cxi'
  docker run --rm "$IMAGE" sh -c 'dkp-pacman -Q libctrpf libctru devkitARM; makerom 2>&1 | head -1; bannertool 2>&1 | head -1; ctrtool --help 2>&1 | head -1' > build/TOOLCHAIN.txt 2>&1 || true
  rm -rf dist
  mkdir -p dist/sd/3ds/souldew dist/sd/cias dist/test
  cp launcher/souldew.3dsx dist/sd/3ds/souldew/souldew.3dsx
  cp plugin/souldew.3gx dist/sd/3ds/souldew/souldew.3gx
  cp launcher/souldew.cia dist/sd/cias/souldew.cia
  # optional: the plugin also loads without the launcher when it sits in a title's plugin folder
  for tid in 0004000000055D00 0004000000055E00 000400000011C400 000400000011C500 \
             0004000000164800 0004000000175E00 00040000001B5000 00040000001B5100; do
    mkdir -p "dist/sd-alternative/luma/plugins/$tid"
    cp plugin/souldew.3gx "dist/sd-alternative/luma/plugins/$tid/souldew.3gx"
  done
  cp launcher/souldew-demo.3dsx plugin/souldew-test.3gx mockgame/souldew-mock.cxi dist/test/
  python3 tools/make_release.py
  echo "dist/ ready"
}

host() {
  make -C host -j4
  ./build/host/tests test/fixtures build/host/zones_ncch.bin
  mkdir -p docs/screenshots
  ./build/host/render docs/screenshots de build/host/sample_runs.json >/dev/null
  ./build/host/render docs/screenshots en >/dev/null
  echo "docs/screenshots rendered"
}

mkdir -p build
case "$what" in
  console) console ;;
  host) host ;;
  all) host; console ;;
  *) echo "usage: ./build.sh [all|console|host]"; exit 1 ;;
esac
