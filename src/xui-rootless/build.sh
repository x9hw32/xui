#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
XUI_SCHEME="${XUI_SCHEME:-rootless}"
case "$XUI_SCHEME" in
 rootless) XUI_OUTPUT=build/xui-rootless; XUI_PREFIX=/var/jb ;;
 rootful) XUI_OUTPUT=build/xui-rootful; XUI_PREFIX= ;;
 *) echo "Unsupported XUI_SCHEME: $XUI_SCHEME" >&2; exit 2 ;;
esac
mkdir -p "$XUI_OUTPUT"
XUI_LDID="${XUI_LDID:-}"
if [[ -z "$XUI_LDID" ]]; then
 if command -v ldid >/dev/null 2>&1; then XUI_LDID="$(command -v ldid)"
 else XUI_LDID=.tools/ldid-src/ldid
 fi
fi
if [[ ! -x "$XUI_LDID" ]]; then echo "Set XUI_LDID to an executable ldid path" >&2; exit 2; fi
cat > "$XUI_OUTPUT/libSystem.tbd" <<'TBD'
--- !tapi-tbd-v3
archs: [ arm64 ]
platform: ios
install-name: /usr/lib/libSystem.B.dylib
exports:
  - archs: [ arm64 ]
    symbols: [ dyld_stub_binder, _printf, _puts, _putchar, _free, _dlopen, _dlsym, _dladdr, _dlerror, _backtrace, _strcmp, _strstr, _strchr, _strlen, _sysctlbyname, _open, _write, _close, _chmod, _access, _snprintf, _posix_spawn, _waitpid, _notify_post, _notify_register_check, _notify_set_state, _notify_get_state, _notify_register_dispatch, _dispatch_after, _dispatch_time, __NSConcreteGlobalBlock, __NSConcreteStackBlock, __dispatch_main_q ]
...
TBD
cat > "$XUI_OUTPUT/libobjc.tbd" <<'TBD'
--- !tapi-tbd-v3
archs: [ arm64 ]
platform: ios
install-name: /usr/lib/libobjc.A.dylib
exports:
  - archs: [ arm64 ]
    symbols: [ _objc_getClass, _objc_allocateClassPair, _objc_registerClassPair, _class_addMethod, _method_getImplementation, _object_getClass, _class_getSuperclass, _objc_copyClassList, _class_getName, _class_getInstanceMethod, _class_getClassMethod, _class_copyMethodList, _class_getInstanceVariable, _class_copyIvarList, _ivar_getName, _ivar_getTypeEncoding, _object_getIvar, _objc_getAssociatedObject, _objc_setAssociatedObject, _method_getTypeEncoding, _method_getName, _sel_getName, _sel_registerName, _objc_msgSend ]
...
TBD
CLANG_ARGS=(--target=arm64-apple-ios15.0 -O2 -Wall -Wextra -Werror -fno-stack-protector -fblocks -Isrc/xui-rootless)
if [[ "$XUI_SCHEME" == rootful ]]; then CLANG_ARGS+=(-DXUI_ROOTFUL=1); fi
if [[ "${XUI_DIAGNOSTICS:-0}" == 1 ]]; then CLANG_ARGS+=(-DXUI_DIAGNOSTICS=1); fi
if [[ "${1:-}" == probe || "${1:-}" == prefs-probe ]]; then
 XUI_PROBE_SOURCE=probe.c
 if [[ "${1:-}" == prefs-probe ]]; then XUI_PROBE_SOURCE=preferences_probe.c; fi
 clang "${CLANG_ARGS[@]}" -c "src/xui-rootless/$XUI_PROBE_SOURCE" -o "$XUI_OUTPUT/probe.o"
 ld64.lld -arch arm64 -platform_version ios 15.0 16.0 -e _main -adhoc_codesign -L "$XUI_OUTPUT" -lSystem -lobjc -o "$XUI_OUTPUT/probe" "$XUI_OUTPUT/probe.o"
 "$XUI_LDID" -S "$XUI_OUTPUT/probe"
else
 clang "${CLANG_ARGS[@]}" -c src/xui-rootless/tweak.c -o "$XUI_OUTPUT/tweak.o"
 ld64.lld -dylib -arch arm64 -platform_version ios 15.0 16.0 -adhoc_codesign -install_name "$XUI_PREFIX/Library/MobileSubstrate/DynamicLibraries/XUIRootless.dylib" -L "$XUI_OUTPUT" -lSystem -lobjc -o "$XUI_OUTPUT/XUIRootless.dylib" "$XUI_OUTPUT/tweak.o"
 "$XUI_LDID" -S "$XUI_OUTPUT/XUIRootless.dylib"
 clang "${CLANG_ARGS[@]}" -c src/xui-rootless/preferences.c -o "$XUI_OUTPUT/preferences.o"
 ld64.lld -bundle -arch arm64 -platform_version ios 15.0 16.0 -adhoc_codesign -L "$XUI_OUTPUT" -lSystem -lobjc -o "$XUI_OUTPUT/XUIRootlessPrefs" "$XUI_OUTPUT/preferences.o"
 "$XUI_LDID" -S "$XUI_OUTPUT/XUIRootlessPrefs"
fi
