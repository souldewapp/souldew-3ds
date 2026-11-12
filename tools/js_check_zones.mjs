#!/usr/bin/env node
// Cross-check: the desktop app's own ctrzones.mjs reads the synthetic NCCH image the C++ tests write
// (build/host/zones_ncch.bin) – both must find the same places.
//   node tools/js_check_zones.mjs <souldew>/native/shared/live/ctrzones.mjs build/host/zones_ncch.bin
import { readFileSync } from 'node:fs';
import { pathToFileURL } from 'node:url';
const { ctrZonePlaces } = await import(pathToFileURL(process.argv[2]).href);
const img = readFileSync(process.argv[3]);
const readAt = (o, n) => img.subarray(o, o + n);
const r = ctrZonePlaces(readAt, { titleId: '0004000000055D00' });
console.log(JSON.stringify(r));
process.exit(r && r.places.join(',') === '6,10,12,14,16,300' ? 0 : 1);
