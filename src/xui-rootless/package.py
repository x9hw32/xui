#!/usr/bin/env python3
"""Package the tweak and localized Settings bundle for either jailbreak layout."""
import io
from pathlib import Path
import tarfile
import re
import argparse
import plistlib

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / 'src/xui-rootless'
parser = argparse.ArgumentParser()
parser.add_argument('--rootful', action='store_true')
args = parser.parse_args()
scheme = 'rootful' if args.rootful else 'rootless'
arch = 'iphoneos-arm' if args.rootful else 'iphoneos-arm64'
BUILD = ROOT / f'build/xui-{scheme}' 

def tar(files):
    output = io.BytesIO()
    with tarfile.open(fileobj=output, mode='w:gz', format=tarfile.USTAR_FORMAT) as archive:
        for name, path, mode in files:
            data = b'' if path is None else (path if isinstance(path, bytes) else path.read_bytes())
            entry = tarfile.TarInfo(name)
            if path is None:
                entry.type = tarfile.DIRTYPE
            entry.mode = mode
            entry.uid = entry.gid = 0
            entry.uname = entry.gname = 'root'
            entry.size = len(data)
            archive.addfile(entry, io.BytesIO(data))
    return output.getvalue()

control_text = (SRC / 'control').read_text()
if args.rootful:
    control_text = control_text.replace('Package: local.rajok.xui-rootless', 'Package: local.rajok.xui-rootful')
    control_text = control_text.replace('Architecture: iphoneos-arm64', 'Architecture: iphoneos-arm')
    control_text = control_text.replace('Conflicts: local.rajok.xui-rootful', 'Conflicts: local.rajok.xui-rootless')
loader = plistlib.loads((SRC / 'preferences/loader.plist').read_bytes())
if args.rootful:
    loader['entry']['icon'] = loader['entry']['icon'].removeprefix('/var/jb')
control = tar([('./control', control_text.encode(), 0o644)])
prefix = './Library' if args.rootful else './var/jb/Library'
files = [(f'{prefix}/PreferenceBundles', None, 0o755),
         (f'{prefix}/PreferenceBundles/XUIRootlessPrefs.bundle', None, 0o755),
         (f'{prefix}/PreferenceBundles/XUIRootlessPrefs.bundle/en.lproj', None, 0o755),
         (f'{prefix}/PreferenceBundles/XUIRootlessPrefs.bundle/ru.lproj', None, 0o755),
         (f'{prefix}/PreferenceLoader', None, 0o755),
         (f'{prefix}/PreferenceLoader/Preferences', None, 0o755),
         (f'{prefix}/MobileSubstrate/DynamicLibraries/XUIRootless.dylib', BUILD / 'XUIRootless.dylib', 0o755),
         (f'{prefix}/MobileSubstrate/DynamicLibraries/XUIRootless.plist', SRC / 'XUIRootless.plist', 0o644),
         (f'{prefix}/PreferenceLoader/Preferences/XUIRootlessPrefs.plist', plistlib.dumps(loader, sort_keys=False), 0o644),
         (f'{prefix}/PreferenceBundles/XUIRootlessPrefs.bundle/Info.plist', SRC / 'preferences/Info.plist', 0o644),
         (f'{prefix}/PreferenceBundles/XUIRootlessPrefs.bundle/XUIRootlessPrefs', BUILD / 'XUIRootlessPrefs', 0o755)]
for language in ('en', 'ru'):
    files.append((f'{prefix}/PreferenceBundles/XUIRootlessPrefs.bundle/{language}.lproj/Localizable.strings',
                  SRC / f'preferences/{language}.lproj/Localizable.strings', 0o644))
for name in ('icon.png', 'icon@2x.png', 'icon@3x.png'):
    files.append((f'{prefix}/PreferenceBundles/XUIRootlessPrefs.bundle/{name}', SRC / 'preferences' / name, 0o644))
data = tar(files)
version = re.search(r'^Version: (.+)$', control_text, re.M)[1]
package = ROOT / 'build' / f'local.rajok.xui-{scheme}_{version}_{arch}.deb'
with package.open('wb') as stream:
    stream.write(b'!<arch>\n')
    for name, payload in [('debian-binary', b'2.0\n'), ('control.tar.gz', control), ('data.tar.gz', data)]:
        header = f'{name+"/":<16}{0:<12}{0:<6}{0:<6}{"100644":<8}{len(payload):<10}`\n'.encode()
        assert len(header) == 60
        stream.write(header)
        stream.write(payload)
        if len(payload) % 2:
            stream.write(b'\n')
print(package)
