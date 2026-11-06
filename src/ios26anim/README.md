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
ARM64/ARM64e builds. The original author's animation code is preserved.
