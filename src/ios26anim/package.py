#!/usr/bin/env python3
"""Package iOS26Anim and its animation editor for iPhone 8."""
import argparse
import io
from pathlib import Path
import re
import tarfile
import plistlib

ROOT = Path(__file__).resolve().parents[2]
SRC = Path(__file__).resolve().parent
parser = argparse.ArgumentParser()
parser.add_argument('--rootful', action='store_true')
args = parser.parse_args()
scheme = 'rootful' if args.rootful else 'rootless'
arch = 'iphoneos-arm' if args.rootful else 'iphoneos-arm64'
prefix = '' if args.rootful else '/var/jb'
package_id = f'local.rajok.ios26anim-{scheme}'
control = (SRC / 'control').read_text()
if args.rootful:
    control = control.replace('Package: local.rajok.ios26anim-rootless', f'Package: {package_id}')
    control = control.replace('Architecture: iphoneos-arm64', 'Architecture: iphoneos-arm')
    control = control.replace('Conflicts: local.rajok.ios26anim-rootful', 'Conflicts: local.rajok.ios26anim-rootless')
version = re.search(r'^Version: (.+)$', control, re.M)[1]

def tar(files):
    out = io.BytesIO()
    with tarfile.open(fileobj=out, mode='w:gz', format=tarfile.USTAR_FORMAT) as archive:
        directories = set()
        for name, data, mode in files:
            for parent in list(Path(name).parents)[::-1]:
                dirname = str(parent)
                if dirname != '.' and dirname not in directories:
                    info = tarfile.TarInfo('./' + dirname)
                    info.type = tarfile.DIRTYPE
                    info.mode = 0o755
                    archive.addfile(info)
                    directories.add(dirname)
            info = tarfile.TarInfo('./' + name)
            info.mode = mode
            info.size = len(data)
            archive.addfile(info, io.BytesIO(data))
    return out.getvalue()

library = ROOT / f'build/ios26anim-{scheme}/iOS26Anim.dylib'
filter_plist = b'<?xml version="1.0" encoding="UTF-8"?><!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd"><plist version="1.0"><dict><key>Filter</key><dict><key>Bundles</key><array><string>com.apple.springboard</string></array></dict></dict></plist>\n'
base = (prefix + '/Library/MobileSubstrate/DynamicLibraries/').lstrip('/')
files = [(base + 'iOS26Anim.dylib', library.read_bytes(), 0o755),
         (base + 'iOS26Anim.plist', filter_plist, 0o644),
         ((prefix + f'/usr/share/doc/{package_id}/UPSTREAM.md').lstrip('/'), (SRC / 'UPSTREAM.md').read_bytes(), 0o644)]
prefs = SRC / 'preferences'
bundle = (prefix + '/Library/PreferenceBundles/IOS26AnimPrefs.bundle/').lstrip('/')
loader = plistlib.loads((prefs / 'loader.plist').read_bytes())
if args.rootful:
    loader['entry']['icon'] = loader['entry']['icon'].removeprefix('/var/jb')
files.extend([
    (bundle + 'IOS26AnimPrefs', (ROOT / f'build/ios26anim-{scheme}/IOS26AnimPrefs').read_bytes(), 0o755),
    ((prefix + '/Library/PreferenceLoader/Preferences/IOS26AnimPrefs.plist').lstrip('/'), plistlib.dumps(loader, sort_keys=False), 0o644)])
for name in ['Info.plist', 'Root.plist', 'en.lproj/Root.strings', 'ru.lproj/Root.strings', 'icon.png', 'icon@2x.png', 'icon@3x.png']:
    files.append((bundle + name, (prefs / name).read_bytes(), 0o644))
parts = [('debian-binary', b'2.0\n'), ('control.tar.gz', tar([('control', control.encode(), 0o644)])),
         ('data.tar.gz', tar(files))]
output = ROOT / 'build' / f'{package_id}_{version}_{arch}.deb'
with output.open('wb') as stream:
    stream.write(b'!<arch>\n')
    for name, data in parts:
        header = f'{name+"/":<16}{0:<12}{0:<6}{0:<6}{"100644":<8}{len(data):<10}`\n'.encode()
        assert len(header) == 60
        stream.write(header)
        stream.write(data)
        if len(data) % 2:
            stream.write(b'\n')
print(output)
