#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
IOS26_SCHEME="${IOS26_SCHEME:-rootless}"
case "$IOS26_SCHEME" in
 rootless) IOS26_PREFIX=/var/jb ;;
 rootful) IOS26_PREFIX= ;;
 *) echo "Use IOS26_SCHEME=rootless or rootful" >&2; exit 2 ;;
esac
IOS26_OUTPUT="build/ios26anim-$IOS26_SCHEME"
IOS26_SDK="${IOS26_SDK:-.tools/ios-sdks/iPhoneOS16.5.sdk}"
IOS26_LOGOS="${IOS26_LOGOS:-.tools/logos/bin/logos.pl}"
IOS26_LDID="${IOS26_LDID:-$(command -v ldid || true)}"
if [[ -z "$IOS26_LDID" ]]; then IOS26_LDID=.tools/ldid-src/ldid; fi
[[ -d "$IOS26_SDK" && -f "$IOS26_LOGOS" && -x "$IOS26_LDID" ]] || {
 echo "Set IOS26_SDK, IOS26_LOGOS and IOS26_LDID to the iOS SDK, Logos and ldid paths" >&2; exit 2;
}
mkdir -p "$IOS26_OUTPUT/include"
curl -fsSL https://raw.githubusercontent.com/Stealth2115/Ios26-anim/75c5e1c71672d9290b44599d21b56d5fa3476f69/Tweak.x -o "$IOS26_OUTPUT/Tweak.x"
python3 - "$IOS26_OUTPUT/Tweak.x" <<'VERIFY'
import hashlib, sys
from pathlib import Path
expected = '2fcffe1a6bbcc5bb3b3b6fa8c03c990e2e210e0242715bfba92f7f238d28fc7b'
if hashlib.sha256(Path(sys.argv[1]).read_bytes()).hexdigest() != expected:
    raise SystemExit('Upstream source checksum mismatch')
VERIFY
# Apply the local class-name and spring-preset setter compatibility fix.
python3 src/ios26anim/apply_compat.py "$IOS26_OUTPUT/Tweak.x"
perl "$IOS26_LOGOS" "$IOS26_OUTPUT/Tweak.x" > "$IOS26_OUTPUT/Tweak.mm"
cat > "$IOS26_OUTPUT/include/substrate.h" <<'HEADER'
#pragma once
#include <objc/runtime.h>
#ifdef __cplusplus
extern "C" {
#endif
void MSHookMessageEx(Class, SEL, IMP, IMP *);
#ifdef __cplusplus
}
#endif
HEADER
cat > "$IOS26_OUTPUT/libsubstrate.tbd" <<TBD
--- !tapi-tbd-v3
archs: [ arm64 ]
platform: ios
install-name: $IOS26_PREFIX/usr/lib/libsubstrate.dylib
current-version: 0.0.0
compatibility-version: 0.0.0
exports:
  - archs: [ arm64 ]
    symbols: [ _MSHookMessageEx ]
...
TBD
for IOS26_ARCH in arm64; do
 clang++ --target="$IOS26_ARCH-apple-ios14.0" -isysroot "$IOS26_SDK" \
  -fobjc-arc -fblocks -fobjc-exceptions -fno-autolink -O2 -Wall -Wextra \
  -Wno-unused-parameter -Wno-unused-variable -I "$IOS26_OUTPUT/include" \
  -c "$IOS26_OUTPUT/Tweak.mm" -o "$IOS26_OUTPUT/$IOS26_ARCH.o"
 ld64.lld -dylib -arch "$IOS26_ARCH" -platform_version ios 14.0 16.5 \
  -adhoc_codesign -install_name "$IOS26_PREFIX/Library/MobileSubstrate/DynamicLibraries/iOS26Anim.dylib" \
  -syslibroot "$IOS26_SDK" -L "$IOS26_OUTPUT" -L "$IOS26_SDK/usr/lib" \
  -F "$IOS26_SDK/System/Library/Frameworks" -lSystem -lobjc -lc++ -lsubstrate \
  -framework UIKit -framework Foundation -framework CoreFoundation -framework QuartzCore -framework CoreGraphics \
  -o "$IOS26_OUTPUT/iOS26Anim-$IOS26_ARCH.dylib" "$IOS26_OUTPUT/$IOS26_ARCH.o"
done
cp "$IOS26_OUTPUT/iOS26Anim-arm64.dylib" "$IOS26_OUTPUT/iOS26Anim.dylib"
"$IOS26_LDID" -S "$IOS26_OUTPUT/iOS26Anim.dylib"
echo "$IOS26_OUTPUT/iOS26Anim.dylib"
