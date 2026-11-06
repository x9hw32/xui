#!/usr/bin/env python3
"""Rebuild a flat APT repository from released debs, using Python only."""
import argparse
import bz2
from email.utils import formatdate
import gzip
import hashlib
from html import escape
import io
import json
import lzma
from pathlib import Path
import shutil
import tarfile

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--base-url', default='https://x9hw32.github.io/xui/')
parser.add_argument('--packages', type=Path, help='Copy new .deb files from this directory')
args = parser.parse_args()
base = args.base_url.rstrip('/') + '/'
site = ROOT / 'docs'
(site / 'debs').mkdir(parents=True, exist_ok=True)
(site / 'depictions').mkdir(exist_ok=True)
if args.packages:
    for package in args.packages.glob('*.deb'):
        shutil.copy2(package, site / 'debs' / package.name)

def metadata(path):
    raw = path.read_bytes()
    if raw[:8] != b'!<arch>\n':
        raise ValueError(f'Invalid deb: {path}')
    offset = 8
    while offset < len(raw):
        header = raw[offset:offset + 60]
        size = int(header[48:58])
        name = header[:16].decode().strip().rstrip('/')
        payload = raw[offset + 60:offset + 60 + size]
        offset += 60 + size + size % 2
        if name.startswith('control.tar'):
            with tarfile.open(fileobj=io.BytesIO(payload)) as archive:
                control = next(m for m in archive if m.name.lstrip('./') == 'control')
                return archive.extractfile(control).read().decode()
    raise ValueError(f'Missing control: {path}')

entries = []
versions = set()
for package in sorted((site / 'debs').glob('*.deb')):
    control = metadata(package)
    fields = dict(line.split(': ', 1) for line in control.splitlines() if ': ' in line and not line.startswith(' '))
    if fields['Name'] != 'XUI':
        raise ValueError(f'Unexpected package: {package.name}')
    versions.add(fields['Version'])
    data = package.read_bytes()
    extra = {'Filename': 'debs/' + package.name, 'Size': str(len(data)),
             'MD5sum': hashlib.md5(data).hexdigest(),
             'SHA256': hashlib.sha256(data).hexdigest(),
             'SHA512': hashlib.sha512(data).hexdigest()}
    entries.append(control.rstrip() + '\n' + ''.join(f'{k}: {v}\n' for k, v in extra.items()))
if not entries:
    raise ValueError('No packages found')
packages = ('\n'.join(entries) + '\n').encode()
(site / 'Packages').write_bytes(packages)
(site / 'Packages.gz').write_bytes(gzip.compress(packages, mtime=0))
(site / 'Packages.bz2').write_bytes(bz2.compress(packages))
(site / 'Packages.xz').write_bytes(lzma.compress(packages))
release = (f'Origin: rajok\nLabel: rajok Repo\nSuite: stable\nCodename: xui\n'
           f'Version: 1.0\nArchitectures: iphoneos-arm iphoneos-arm64\nComponents: main\n'
           f'Description: XUI by rajok for iPhone 8 on iOS 16\nDate: {formatdate(usegmt=True)}\n'
           f'Icon: {base}CydiaIcon.png\n')
for label, digest in [('MD5Sum', hashlib.md5), ('SHA256', hashlib.sha256), ('SHA512', hashlib.sha512)]:
    release += label + ':\n'
    for name in ['Packages', 'Packages.gz', 'Packages.bz2', 'Packages.xz']:
        data = (site / name).read_bytes()
        release += f' {digest(data).hexdigest()} {len(data):16d} {name}\n'
(site / 'Release').write_text(release)
version = sorted(versions, key=lambda s: tuple(map(int, s.split('.'))))[-1]
text = ('iPhone X gestures, a home indicator, floating dock, native flashlight and camera '
        'buttons on the Lock Screen, and adjustable app/keyboard spacing. '
        'Russian and English Settings. Stock iPhone 8 status bar.\n\n'
        'Supports iPhone 8 (iPhone10,1 / iPhone10,4), iOS 16. '
        'Rootless tested on iOS 16.7.16. Rootful build awaits device testing. '
        'LiquidAss is optional and is not included.\n\n'
        'Жесты iPhone X, нижняя полоса, плавающий док, нативные кнопки фонарика и камеры, '
        'настраиваемые отступы приложений и клавиатуры. Настройки на русском и английском.')
depiction = {'class': 'DepictionTabView', 'minVersion': '0.4', 'tintColor': '#b26c91', 'tabs': [
    {'class': 'DepictionStackView', 'tabname': 'XUI', 'views': [
        {'class': 'DepictionHeaderView', 'title': 'XUI'},
        {'class': 'DepictionMarkdownView', 'markdown': text},
        {'class': 'DepictionTableTextView', 'title': 'Version', 'text': version},
        {'class': 'DepictionTableTextView', 'title': 'Author', 'text': 'rajok'},
        {'class': 'DepictionTableTextView', 'title': 'Devices', 'text': 'iPhone 8 · iOS 16'}]}]}
(site / 'depictions/xui.json').write_text(json.dumps(depiction, ensure_ascii=False, indent=2) + '\n')
html = (f'<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" '
        f'content="width=device-width,initial-scale=1"><title>XUI — rajok Repo</title>'
        f'<style>body{{font:17px system-ui;max-width:650px;margin:48px auto;padding:0 24px;'
        f'line-height:1.6}}img{{border-radius:22px}}a{{color:#975b7c}}</style>'
        f'<img src="{base}icons/xui.png" width="96" height="96" alt="XUI icon">'
        f'<h1>XUI {escape(version)}</h1><p>by rajok</p>'
        f'<p>{escape(text).replace(chr(10), "<br>")}</p>'
        f'<p><a href="sileo://source/{base}">Add to Sileo / Добавить в Sileo</a></p>'
        f'<p>Repository: <a href="{base}">{base}</a></p>'
        f'<p><a href="https://github.com/x9hw32/xui">Source code</a></p></html>')
(site / 'index.html').write_text(html)
(site / 'depictions/xui.html').write_text(html)
print(f'Repository built: {len(entries)} packages; {base}')
