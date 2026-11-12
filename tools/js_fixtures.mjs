#!/usr/bin/env node
// Writes test/fixtures/pk6.json: PK6/PK7 records encrypted by the souldew desktop app's own JavaScript
// (native/shared/live/gen67.mjs encryptPK6) and what its pk6Mon() reads from them – the C++ port must agree.
//   node tools/js_fixtures.mjs <souldew>/native/shared/live/gen67.mjs
import { writeFileSync } from 'node:fs';
import { join, dirname } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const mod = await import(pathToFileURL(process.argv[2]).href);
const root = join(dirname(fileURLToPath(import.meta.url)), '..');
const w16 = (b, o, v) => { b[o] = v & 0xff; b[o + 1] = (v >> 8) & 0xff; };
const w32 = (b, o, v) => { for (let i = 0; i < 4; i++) b[o + i] = (v >>> (8 * i)) & 0xff; };
let seed = 12345;
const rnd = () => (seed = (Math.imul(seed, 1103515245) + 12345) >>> 0);
const cases = [];
const specs = [
  { species: 656, level: 5, hp: 20, maxHp: 20, nick: 'Froxy', met: 10, gen: 6, status: 0 },
  { species: 661, level: 31, hp: 0, maxHp: 88, nick: 'Dartiri', met: 12, gen: 6, status: 1 },
  { species: 722, level: 14, hp: 30, maxHp: 41, nick: 'Bauzé', met: 6, gen: 7, status: 4 },
  { species: 807, level: 100, hp: 300, maxHp: 301, nick: 'Zeraora', met: 40, gen: 7, status: 0 },
  { species: 29, level: 7, hp: 22, maxHp: 25, nick: 'Nido', met: 30, gen: 6, status: 5, female: true },
];
for (const s of specs) {
  const b = new Uint8Array(260);
  w32(b, 0, rnd());
  w16(b, 0x08, s.species);
  w16(b, 0x0a, 0);
  w16(b, 0x0c, rnd() & 0xffff);
  w16(b, 0x0e, rnd() & 0xffff);
  w32(b, 0x18, rnd());
  b[0x1d] = s.female ? 1 << 1 : 0;
  [...s.nick].forEach((c, i) => w16(b, 0x40 + i * 2, c.charCodeAt(0)));
  w16(b, 0x5a, 33); w16(b, 0x5c, 45);
  w32(b, 0x74, rnd() & 0x3fffffff);
  w16(b, 0xda, s.met);
  b[0xdd] = s.level;
  w32(b, 0xe8, s.status);
  b[0xec] = s.level;
  w16(b, 0xf0, s.hp); w16(b, 0xf2, s.maxHp);
  for (let o = 0xf4; o < 0xfe; o += 2) w16(b, o, 50);
  const enc = mod.encryptPK6(b);
  const mon = mod.pk6Mon(enc, s.gen);
  cases.push({ gen: s.gen, hex: Buffer.from(enc).toString('hex'), expect: mon });
}
writeFileSync(join(root, 'test', 'fixtures', 'pk6.json'), JSON.stringify(cases, null, 1) + '\n');
console.log('test/fixtures/pk6.json:', cases.length, 'cases');
