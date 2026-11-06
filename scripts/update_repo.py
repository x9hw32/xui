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
import re
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

catalog = json.loads((ROOT / 'catalog.json').read_text())
known_ids = {package_id for item in catalog for package_id in item['ids']}
entries = []
records = []
for package in sorted((site / 'debs').glob('*.deb')):
    control = metadata(package)
    fields = dict(line.split(': ', 1) for line in control.splitlines() if ': ' in line and not line.startswith(' '))
    if fields['Package'] not in known_ids:
        raise ValueError(f'Unexpected package: {package.name}')
    records.append({**fields, 'file': 'debs/' + package.name})
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
           f'Description: Packages maintained by rajok for iPhone 8 on iOS 16\nDate: {formatdate(usegmt=True)}\n'
           f'Icon: {base}CydiaIcon.png\n')
for label, digest in [('MD5Sum', hashlib.md5), ('SHA256', hashlib.sha256), ('SHA512', hashlib.sha512)]:
    release += label + ':\n'
    for name in ['Packages', 'Packages.gz', 'Packages.bz2', 'Packages.xz']:
        data = (site / name).read_bytes()
        release += f' {digest(data).hexdigest()} {len(data):16d} {name}\n'
(site / 'Release').write_text(release)
cards = []
style = '<style>body{font:17px system-ui;max-width:700px;margin:48px auto;padding:0 24px;line-height:1.6}img{border-radius:22px}a{color:#975b7c}article{margin:36px 0}</style>'
for item in catalog:
    builds = [r for r in records if r['Package'] in item['ids']]
    if not builds:
        continue
    version = max((r['Version'] for r in builds), key=lambda v: tuple(map(int, re.findall(r'\d+', v))))
    text = item['description']
    depiction = {'class': 'DepictionTabView', 'minVersion': '0.4', 'tintColor': '#b26c91', 'tabs': [
        {'class': 'DepictionStackView', 'tabname': item['name'], 'views': [
            {'class': 'DepictionHeaderView', 'title': item['name']},
            {'class': 'DepictionMarkdownView', 'markdown': text},
            {'class': 'DepictionTableTextView', 'title': 'Version', 'text': version},
            {'class': 'DepictionTableTextView', 'title': 'Author', 'text': item['author']},
            {'class': 'DepictionTableTextView', 'title': 'Devices', 'text': 'iPhone 8 · iOS 16'},
            {'class': 'DepictionMarkdownView', 'markdown': item['status']}]}]}
    (site / f"depictions/{item['slug']}.json").write_text(json.dumps(depiction, ensure_ascii=False, indent=2) + '\n')
    downloads = ''.join(f'<li><a href="{base}{escape(r["file"])}">{escape(r["Package"])} {escape(r["Version"])}</a></li>' for r in builds)
    card = (f'<article><img src="{base}{item["icon"]}" width="96" height="96" alt="">'
            f'<h2>{escape(item["name"])} {escape(version)}</h2><p>by {escape(item["author"])}</p>'
            f'<p>{escape(text).replace(chr(10), "<br>")}</p><p>{escape(item["status"])}</p>'
            f'<ul>{downloads}</ul><p><a href="{escape(item["source"])}">Source code</a></p></article>')
    page = (f'<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">'
            f'<title>{escape(item["name"])} — rajok Repo</title>{style}{card}'
            f'<p><a href="sileo://source/{base}">Add to Sileo / Добавить в Sileo</a></p></html>')
    (site / f"depictions/{item['slug']}.html").write_text(page)
    cards.append(card)
index = (f'<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">'
         f'<title>rajok Repo</title>{style}<img src="{base}CydiaIcon.png" width="96" height="96" alt="Repository icon">'
         f'<h1>rajok Repo</h1><p><a href="sileo://source/{base}">Add to Sileo / Добавить в Sileo</a></p>'
         f'<p>Repository: <a href="{base}">{base}</a></p>' + ''.join(cards) + '</html>')
(site / 'index.html').write_text(index)
print(f'Repository built: {len(entries)} packages; {base}')
