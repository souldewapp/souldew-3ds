#!/usr/bin/env python3
"""Puts together the beta release from the built files (run by ./build.sh after the console build).

  python3 tools/make_release.py [--base URL] [--revision N] [--qr-only]

dist/release/   the files to upload to the GitHub release, under exactly these names (the URLs depend on them):
                souldew.cia  souldew.3dsx  souldew.3gx  souldew.unistore  souldew.t3x  souldew-3ds-<version>.zip
dist/qr/        QR codes for the Discord post (PNG): Universal-Updater (the UniStore's URL), FBI (the .cia's URL)

--base is what replaces {{RELEASE_BASE}} in release/souldew.unistore.template: the URL the release's assets are
downloaded from, without the trailing slash, e.g. https://github.com/<owner>/<repo>/releases/download/<tag>.
The QR codes need the Python package `qrcode` (+ Pillow); without it they are skipped with a hint. It is not a
dependency of the build: install it in a throwaway venv (see docs/RELEASING.md).
"""
import argparse, datetime, json, os, shutil, sys, zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DIST = os.path.join(ROOT, 'dist')
REL = os.path.join(DIST, 'release')
QR = os.path.join(DIST, 'qr')
# the proposed home of the beta builds: a "beta" release in a repository souldew-3ds whose assets are replaced on
# every beta – the URLs (and the printed QR codes) then never change. Override with --base.
DEFAULT_BASE = 'https://github.com/souldewapp/souldew-3ds/releases/download/beta'

RELEASE_NOTES = 'First beta. Please report what you see in #beta-feedback on the souldew Discord.'


def version():
    for line in open(os.path.join(ROOT, 'common', 'screens.h'), encoding='utf-8'):
        if 'kVersion = "' in line:
            return line.split('kVersion = "')[1].split('"')[0]
    sys.exit('kVersion not found in common/screens.h')


def unistore(base, ver, revision, now):
    tpl = open(os.path.join(ROOT, 'release', 'souldew.unistore.template'), encoding='utf-8').read()
    out = (tpl.replace('{{RELEASE_BASE}}', base.rstrip('/'))
              .replace('{{VERSION}}', ver)
              .replace('{{REVISION}}', str(revision))
              .replace('{{DATE}}', now.strftime('%Y-%m-%d at %H:%M (UTC)'))
              .replace('{{RELEASE_NOTES}}', RELEASE_NOTES))
    assert '{{' not in out, 'unfilled placeholder in the UniStore template'
    data = json.loads(out)  # must be valid JSON
    assert data['storeInfo']['version'] in (3, 4) and isinstance(data['storeInfo']['revision'], int)
    assert '/' not in data['storeInfo']['file']
    return out


def sd_zip(path, ver):
    files = [
        ('3ds/souldew/souldew.3dsx', os.path.join(ROOT, 'launcher', 'souldew.3dsx')),
        ('3ds/souldew/souldew.3gx', os.path.join(ROOT, 'plugin', 'souldew.3gx')),
        ('cias/souldew.cia', os.path.join(ROOT, 'launcher', 'souldew.cia')),
    ]
    readme = open(os.path.join(ROOT, 'release', 'README_TESTERS.txt'), encoding='utf-8').read().replace('{{VERSION}}', ver)
    stamp = (2026, 1, 1, 0, 0, 0)  # fixed timestamps: the same inputs give the same zip
    with zipfile.ZipFile(path, 'w', zipfile.ZIP_DEFLATED) as z:
        for d in ('3ds/', '3ds/souldew/', 'cias/'):
            z.writestr(zipfile.ZipInfo(d, stamp), '')
        for arc, src in files:
            zi = zipfile.ZipInfo(arc, stamp)
            zi.compress_type = zipfile.ZIP_DEFLATED
            zi.external_attr = 0o644 << 16
            z.writestr(zi, open(src, 'rb').read())
        zi = zipfile.ZipInfo('README_TESTERS.txt', stamp)
        zi.compress_type = zipfile.ZIP_DEFLATED
        zi.external_attr = 0o644 << 16
        z.writestr(zi, readme.replace('\n', '\r\n'))  # CRLF: readable in Windows' Notepad too


def qr_codes(base):
    try:
        import qrcode
        from PIL import Image, ImageDraw, ImageFont
    except ImportError:
        print('QR codes skipped: the Python package "qrcode" (with Pillow) is not installed (see docs/RELEASING.md)')
        return []
    os.makedirs(QR, exist_ok=True)
    font_path = os.path.join(ROOT, 'assets', 'fonts', 'IBMPlexSans-VF-latin.ttf')
    made = []
    for name, url, caption in (
            ('souldew-universal-updater-qr.png', base + '/souldew.unistore', 'Universal-Updater: add UniStore'),
            ('souldew-fbi-cia-qr.png', base + '/souldew.cia', 'FBI: Remote Install > Scan QR Code')):
        q = qrcode.QRCode(error_correction=qrcode.constants.ERROR_CORRECT_M, box_size=8, border=4)
        q.add_data(url)
        q.make(fit=True)
        code = q.make_image(fill_color='black', back_color='white').convert('RGB')
        # a caption under the code, so the two PNGs can't be mixed up in a Discord post
        font = ImageFont.truetype(font_path, 22)
        small = ImageFont.truetype(font_path, 13)
        probe = ImageDraw.Draw(Image.new('RGB', (1, 1)))
        W = int(max(code.width, probe.textlength(caption, font=font) + 32, probe.textlength(url, font=small) + 32))
        im = Image.new('RGB', (W, code.height + 64), 'white')
        im.paste(code, ((W - code.width) // 2, 0))
        d = ImageDraw.Draw(im)
        d.text((W // 2, code.height + 6), caption, font=font, fill='black', anchor='mt')
        d.text((W // 2, code.height + 38), url, font=small, fill=(90, 90, 90), anchor='mt')
        out = os.path.join(QR, name)
        im.save(out)
        made.append((out, url))
    return made


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--base', default=os.environ.get('SOULDEW_RELEASE_BASE', DEFAULT_BASE))
    ap.add_argument('--revision', type=int, help='UniStore revision (must grow with every upload); default: UTC YYYYMMDDHH')
    ap.add_argument('--qr-only', action='store_true', help='only (re)make the QR codes for --base')
    a = ap.parse_args()
    base = a.base.rstrip('/')
    if a.qr_only:
        for out, url in qr_codes(base):
            print(os.path.relpath(out, ROOT), '->', url)
        return
    ver = version()
    now = datetime.datetime.now(datetime.timezone.utc)
    revision = a.revision or int(now.strftime('%Y%m%d%H'))
    os.makedirs(REL, exist_ok=True)
    for src, dst in (('launcher/souldew.cia', 'souldew.cia'), ('launcher/souldew.3dsx', 'souldew.3dsx'),
                     ('plugin/souldew.3gx', 'souldew.3gx'), ('build/souldew.t3x', 'souldew.t3x')):
        shutil.copyfile(os.path.join(ROOT, src), os.path.join(REL, dst))
    with open(os.path.join(REL, 'souldew.unistore'), 'w', encoding='utf-8') as f:
        f.write(unistore(base, ver, revision, now))
    zname = 'souldew-3ds-%s.zip' % ver
    sd_zip(os.path.join(REL, zname), ver)
    shutil.copyfile(os.path.join(REL, zname), os.path.join(DIST, zname))
    print('release files (upload these):', ', '.join(sorted(os.listdir(REL))))
    print('UniStore URL:', base + '/souldew.unistore', '| revision', revision)
    for out, url in qr_codes(base):
        print(os.path.relpath(out, ROOT), '->', url)


if __name__ == '__main__':
    main()
