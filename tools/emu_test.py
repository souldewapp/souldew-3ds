#!/usr/bin/env python3
"""Runs souldew-3ds in the Azahar emulator and takes screenshots through Azahar's RPC server (UDP 45987).

  python3 tools/emu_test.py plugin   <Azahar.app>   # the mock game + souldew-test.3gx (plugin loader on)
  python3 tools/emu_test.py launcher <Azahar.app>   # the launcher's demo build with a sample run file
  python3 tools/emu_test.py cia      <Azahar.app>   # the .cia's program (build/souldew-cia.cxi) on an empty SD card

The launcher modes start without the plugin on the SD card and check afterwards that the launcher installed it from
its RomFS (sdmc:/3ds/souldew/souldew.3gx, byte for byte plugin/souldew.3gx).

Azahar runs in portable mode: a `user/` folder next to Azahar.app holds its settings and virtual SD card, so the
person's own Azahar setup is never touched. Sound output is set to "null" and volume 0 (silent). Screenshots land in
docs/screenshots/emu_*.png; for the plugin run, the run file the plugin wrote is checked at the end.

Needs no game files: the mock game is a homebrew test title built from this repository.
"""
import json, os, shutil, socket, struct, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PORT = 45987


class Rpc:
    def __init__(self):
        self.s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.s.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 1 << 20)
        self.s.settimeout(1.0)
        self.id = 1

    def req(self, typ, a, b, timeout=1.0):
        self.id += 1
        pkt = struct.pack('<IIII', 2, self.id, typ, 8) + struct.pack('<II', a, b)
        self.s.settimeout(timeout)
        self.s.sendto(pkt, ('127.0.0.1', PORT))
        while True:
            data, _ = self.s.recvfrom(65536)
            ver, rid, rtyp, size = struct.unpack('<IIII', data[:16])
            if rid == self.id:
                return data[16:16 + size]

    def alive(self):
        try:
            self.req(3, 0, 8, 0.5)  # process list
            return True
        except OSError:
            return False

    def read(self, addr, size):
        return self.req(1, addr, size, 1.0)

    def screenshot(self, path):
        info = self.req(5, 1, 0, 15.0)  # res scale 1, PNG
        result, w, h, size = struct.unpack('<IIII', info[:16])
        if result != 0:
            raise RuntimeError('screenshot failed: %d' % result)
        out, off = b'', 0
        while off < size:
            chunk = self.req(6, off, min(32 * 1024, size - off), 2.0)
            if not chunk:
                break
            out += chunk
            off += len(chunk)
        with open(path, 'wb') as f:
            f.write(out)
        return w, h


def select_mock(rpc):
    # memory reads go to the selected process: the mock game's
    procs = rpc.req(3, 0, 8)
    n = struct.unpack_from('<I', procs, 0)[0]
    for i in range(n):
        pid, lo, hi = struct.unpack_from('<III', procs, 4 + i * 0x14)
        if (hi << 32 | lo) == 0x000400000F5D0000:
            rpc.req(4, 1, pid)
            return pid
    return None


def setup(app, sd_files):
    base = os.path.dirname(os.path.abspath(app))
    user = os.path.join(base, 'user')
    cfg = os.path.join(user, 'config')
    os.makedirs(cfg, exist_ok=True)
    settings = {
        'System': {'plugin_loader': 'true', 'allow_plugin_loader': 'true', 'is_new_3ds': 'true'},
        'Debugging': {'enable_rpc_server': 'true'},
        'Audio': {'output_type': '1', 'volume': '0', 'audio_emulation': '0'},
        'UI': {'Updater\\check_for_update_on_start': 'false', 'firstStart': 'false', 'first_start': 'false',
               'pause_when_in_background': 'false', 'confirmClose': 'false', 'confirm_before_closing': 'false'},
        'Miscellaneous': {'check_for_update_on_start': 'false'},
    }
    with open(os.path.join(cfg, 'qt-config.ini'), 'w') as f:
        for sect, kv in settings.items():
            f.write('[%s]\n' % sect)
            for k, v in kv.items():
                f.write('%s\\default=false\n%s=%s\n' % (k, k, v))
            f.write('\n')
    sdmc = os.path.join(user, 'sdmc')
    if os.path.isdir(sdmc):
        shutil.rmtree(sdmc)
    for dst, src in sd_files.items():
        p = os.path.join(sdmc, dst)
        os.makedirs(os.path.dirname(p), exist_ok=True)
        shutil.copy(src, p)
    return user


def run(app, rom, shots, total, trace=False):
    # through the bundle (`open`): started directly, Azahar first shows a "run me via Azahar.app" dialog
    subprocess.run(['pkill', '-f', 'Azahar.app/Contents/MacOS/azahar'])
    subprocess.run(['open', '-n', '-g', app, '--args', rom], check=True)

    class Proc:
        def poll(self):
            r = subprocess.run(['pgrep', '-f', 'Azahar.app/Contents/MacOS/azahar'], capture_output=True)
            return None if r.returncode == 0 else 1

        def terminate(self):
            subprocess.run(['pkill', '-f', 'Azahar.app/Contents/MacOS/azahar'])

        def wait(self, t):
            for _ in range(int(t * 10)):
                if self.poll() is not None:
                    return
                time.sleep(0.1)
            raise subprocess.TimeoutExpired('azahar', t)

        def kill(self):
            subprocess.run(['pkill', '-9', '-f', 'Azahar.app/Contents/MacOS/azahar'])

    proc = Proc()
    time.sleep(2)
    rpc = Rpc()
    t0 = time.time()
    try:
        while not rpc.alive():
            if time.time() - t0 > 240 or proc.poll() is not None:
                raise RuntimeError('Azahar did not come up (see azahar.log)')
            time.sleep(0.5)
        start = time.time()
        print('emulation running after %.1f s' % (start - t0))
        pid = None
        for at, name in shots:
            if name == 'WAIT_TABS':
                # the test plugin opens souldew's screens by itself: shoot each tab when it is up (trace slot 7)
                pid = pid or select_mock(rpc)
                seen, t1 = set(), time.time()
                while time.time() - t1 < 40:
                    v = struct.unpack('<I', rpc.read(0x08100000 + 7 * 4, 4))[0]
                    names = {1: 'souldew_team', 2: 'souldew_routes', 3: 'souldew_graves', 4: 'souldew_rules', 0x100: 'back_in_game'}
                    if v in names and v not in seen:
                        seen.add(v)
                        time.sleep(1.0 if v == 0x100 else 0.6)
                        p = os.path.join(ROOT, 'docs', 'screenshots', 'emu_plugin_%s.png' % names[v])
                        rpc.screenshot(p)
                        print('%5.1f s  %s' % (time.time() - start, os.path.relpath(p, ROOT)))
                        if v == 0x100:
                            break
                    time.sleep(0.1)
                continue
            while time.time() - start < at:
                time.sleep(0.05)
            path = os.path.join(ROOT, 'docs', 'screenshots', 'emu_%s.png' % name)
            try:
                w, h = rpc.screenshot(path)
                print('%5.1f s  %s (%dx%d)' % (time.time() - start, os.path.relpath(path, ROOT), w, h))
            except Exception as e:  # keep going: one failed shot shouldn't end the run
                print('%5.1f s  %s FAILED: %s' % (time.time() - start, name, e))
        while time.time() - start < total:
            time.sleep(0.2)
        if trace:
            try:
                select_mock(rpc)
                v = struct.unpack('<8I', rpc.read(0x08100000, 32))
                print('plugin trace: main %d, zones 0x%x, romfs 0x%08x, polls %d, osd %d, saves %d/%d' % (v[0], v[1], v[2], v[3], v[4], v[5], v[6]))
            except Exception as e:
                print('trace read failed:', e)
    finally:
        proc.terminate()
        try:
            proc.wait(10)
        except subprocess.TimeoutExpired:
            proc.kill()


def main():
    mode, app = sys.argv[1], sys.argv[2]
    if mode == 'plugin':
        user = setup(app, {
            'luma/plugins/000400000F5D0000/souldew-test.3gx': os.path.join(ROOT, 'plugin', 'souldew-test.3gx'),
        })
        shots = [(5.0, 'plugin_boot'), (9.5, 'plugin_new_area'), (14.5, 'plugin_first_encounter'), (20.0, 'plugin_caught'),
                 (24.5, 'plugin_forest'), (30.0, 'plugin_dupe'), (35.5, 'plugin_level_cap'), (39.5, 'plugin_trainer'),
                 (44.0, 'plugin_death'), (50.8, 'plugin_badge'), (53.4, 'plugin_trophy'),
                 (56.0, 'WAIT_TABS')]
        run(app, os.path.join(ROOT, 'mockgame', 'souldew-mock.cxi'), shots, 0, trace=True)
        path = os.path.join(user, 'sdmc', 'souldew', 'runs.json')
        if not os.path.exists(path):
            print('NO RUN FILE: the plugin did not write sdmc:/souldew/runs.json')
            sys.exit(1)
        store = json.load(open(path))
        run_ = next(r for r in store['runs'] if r['id'] == store['active'])
        print('run file:', run_['name'], '| badges', run_['badges'], '| area', run_['lastArea'])
        for e in run_['encounters']:
            print('  encounter %-18s species %3d  %s%s' % (e['area'], e['species'], e['status'], ' (starter)' if e.get('starter') else ''))
        for m in run_['mons']:
            print('  mon %-10s species %3d Lv %2d  %s' % (m['key'][:10], m['species'], m['level'], 'DEAD (%s)' % m.get('cause', '') if m['dead'] else 'alive'))
        shutil.copy(path, os.path.join(ROOT, 'build', 'emu_plugin_runs.json'))
    elif mode == 'launcher':
        sample = os.path.join(ROOT, 'build', 'host', 'sample_runs.json')
        user = setup(app, {'souldew/runs.json': sample})
        # the demo build presses its own buttons (launcher/source/main.cpp, SOULDEW_DEMO): shots between the steps
        shots = [(2.2, 'launcher_intro_1'), (4.3, 'launcher_intro_2'), (6.3, 'launcher_intro_3'), (8.3, 'launcher_intro_4'),
                 (10.3, 'launcher_intro_5'), (12.9, 'launcher_menu'), (16.2, 'launcher_runs'), (18.6, 'launcher_run_team'),
                 (21.6, 'launcher_run_routes'), (24.6, 'launcher_run_graves'), (27.6, 'launcher_run_rules'),
                 (31.2, 'launcher_menu_again'), (34.2, 'launcher_trophies'), (37.2, 'launcher_trophies_collection'),
                 (40.9, 'launcher_settings'), (43.6, 'launcher_settings_overlays'), (47.6, 'launcher_new_game'),
                 (50.4, 'launcher_new_rules'), (53.6, 'launcher_new_name'), (57.6, 'launcher_menu_end')]
        run(app, os.path.join(ROOT, 'launcher', 'souldew-demo.3dsx'), shots, 59)
        check_plugin_installed(user)
    elif mode == 'cia':
        # the program inside souldew.cia (its NCCH, extracted by build.sh), first start: the introduction
        user = setup(app, {})
        rom = os.path.join(ROOT, 'build', 'souldew-cia.cxi')
        shots = [(3.0, 'cia_intro')]
        run(app, rom, shots, 7)
        check_plugin_installed(user)
    else:
        sys.exit('mode: plugin | launcher | cia')


def check_plugin_installed(user):
    got = os.path.join(user, 'sdmc', '3ds', 'souldew', 'souldew.3gx')
    want = os.path.join(ROOT, 'plugin', 'souldew.3gx')
    if not os.path.exists(got):
        print('PLUGIN NOT INSTALLED: sdmc:/3ds/souldew/souldew.3gx is missing')
        sys.exit(1)
    same = open(got, 'rb').read() == open(want, 'rb').read()
    print('plugin installed by the launcher: sdmc:/3ds/souldew/souldew.3gx, %d bytes, %s' %
          (os.path.getsize(got), 'identical to plugin/souldew.3gx' if same else 'DIFFERENT from plugin/souldew.3gx'))
    if not same:
        sys.exit(1)


if __name__ == '__main__':
    main()
