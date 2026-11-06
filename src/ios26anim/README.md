# Build iOS26Anim

See [UPSTREAM.md](UPSTREAM.md) for source revision, attribution, target and build status.

Requirements: Clang/LLVM (including `ld64.lld`), Perl, Logos, an iPhoneOS 16.5 SDK,
`curl`, `ldid`, and Python 3. Set `IOS26_SDK`, `IOS26_LOGOS`, and `IOS26_LDID` if
these are not installed under the local `.tools/` paths used by the script.

```sh
src/ios26anim/build.sh
python3 src/ios26anim/package.py
IOS26_SCHEME=rootful src/ios26anim/build.sh
python3 src/ios26anim/package.py --rootful
python3 scripts/update_repo.py --packages build
```

These packages contain the ARM64 slice for iPhone 8. They are not universal
ARM64/ARM64e builds. Upstream tuning is preserved. apply_compat.py corrects the spring settings
class name and adds setters for cached spring presets on iOS 16.

## Editor

Open Settings → iOS26Anim. Tune opening and closing separately, then tap
Apply and respring. Response controls spring timing (lower is faster); damping
controls bounce (lower is bouncier). Reset restores the original defaults.
Changes are loaded on SpringBoard startup; this also reliably rebuilds cached
spring presets. The interface follows Russian/English system localization.
