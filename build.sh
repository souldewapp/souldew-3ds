#!/bin/sh
# Builds souldew-3ds reproducibly: the toolchain runs in a Docker image (devkitARM pinned by digest, 3gxtool and
# makerom pinned by commit/tag, libctrpf from ThePixellizerOSS' package database), the host tests and screenshots
# with the computer's C++ compiler.
#
#   ./build.sh            everything → dist/ (SD card layout) + tests + docs/screenshots
#   ./build.sh console    only the 3DS files
#   ./build.sh host       only tests + screenshots
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
  run launcher "make && make demo"
  run plugin "make && make test"
  run mockgame "make"
  docker run --rm "$IMAGE" sh -c 'dkp-pacman -Q libctrpf libctru devkitARM; makerom 2>&1 | head -1' > build/TOOLCHAIN.txt 2>&1 || true
  rm -rf dist
  mkdir -p dist/sd/3ds/souldew dist/test
  cp launcher/souldew.3dsx dist/sd/3ds/souldew/souldew.3dsx
  cp plugin/souldew.3gx dist/sd/3ds/souldew/souldew.3gx
  # optional: the plugin also loads without the launcher when it sits in a title's plugin folder
  for tid in 0004000000055D00 0004000000055E00 000400000011C400 000400000011C500 \
             0004000000164800 0004000000175E00 00040000001B5000 00040000001B5100; do
    mkdir -p "dist/sd-alternative/luma/plugins/$tid"
    cp plugin/souldew.3gx "dist/sd-alternative/luma/plugins/$tid/souldew.3gx"
  done
  cp launcher/souldew-demo.3dsx plugin/souldew-test.3gx mockgame/souldew-mock.cxi dist/test/
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
