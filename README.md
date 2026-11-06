<p><img src="docs/icons/xui.png" width="96" height="96" alt="XUI icon"></p>

# XUI

iPhone X navigation and Lock Screen controls for **iPhone 8 on iOS 16**. Author: **rajok**. Current version: **1.0.0**.

**Add to Sileo:** https://x9hw32.github.io/xui/

## Возможности / Features

- Жесты iPhone X и полоса в приложениях / Home gestures and home indicator.
- Пункт управления сверху справа при включённых жестах, снизу при выключенных / Control Center follows the gesture setting.
- Плавающий док / Floating dock.
- Нативные кнопки фонарика и камеры на экране блокировки / Native Lock Screen quick actions.
- Полоса и подсказка на блокировке с возможностью вернуть обычный интерфейс / Optional Lock Screen footer.
- Переключаемые отступы приложений и клавиатуры / Adjustable app and keyboard spacing.
- Настройки на русском и английском / Russian and English Settings.

Строка состояния остаётся штатной от iPhone 8. LiquidAss поддерживается отдельно и в пакет не включён.

## Compatibility

| Build | Package ID | Architecture | Status |
| --- | --- | --- | --- |
| Rootless | `local.rajok.xui-rootless` | `iphoneos-arm64` | Tested on iPhone 8 / iOS 16.7.16 |
| Rootful | `local.rajok.xui-rootful` | `iphoneos-arm` | Built; device testing pending |

Runtime device support is intentionally limited to `iPhone10,1` and `iPhone10,4`, running iOS 16. Other devices are not supported. Requires a substrate-compatible jailbreak, PreferenceLoader, and Cephei. Both builds display **XUI**. Package IDs, preference domain and internal bundle names stay stable to preserve upgrades and existing settings.

Configure **Settings → XUI**, then tap **Apply and respring** for SpringBoard changes. App and keyboard spacing updates are shared with sandboxed apps. Disabling the app indicator also disables both spacing switches; enabling it enables them again. Individual spacing switches can be adjusted while the indicator is enabled.

## Build from source

Linux cross-build prerequisites: `bash`, `clang` with the arm64 Apple target, LLVM `ld64.lld`, `ldid`, and Python 3. No Apple SDK is used; the small required ABI surface is declared in `runtime.h`. Put `ldid` on PATH or set `XUI_LDID=/path/to/ldid`.

```sh
src/xui-rootless/build.sh
python3 src/xui-rootless/package.py
XUI_SCHEME=rootful src/xui-rootless/build.sh
python3 src/xui-rootless/package.py --rootful
```

Artifacts are written to `build/`. To update the Sileo repository after a release:

```sh
python3 scripts/update_repo.py --packages build
```

Commit the updated `docs/` tree. GitHub Pages serves `docs/` from the `main` branch. The repository is flat APT (`deb https://x9hw32.github.io/xui/ ./`) and publishes indexes for both architectures. The release packages are also available in `docs/debs/`.

## License

Source code: MIT, see [LICENSE](LICENSE). Artwork is supplied by the repository owner and is excluded from the source-code license.

## Other packages in rajok Repo

**iOS26Anim** by [Stealth2115](https://github.com/Stealth2115/Ios26-anim):
rootless and rootful ARM64 builds for iPhone 8 on iOS 16, version
`0.1.0+rajok.1`. Built from the pinned upstream source without changing its
animation code or tuning; device testing is pending. See
[src/ios26anim](src/ios26anim/README.md) for build steps and provenance.
Third-party animation code/binaries are excluded from this repository's MIT
license. The APT catalog is described in `catalog.json`.
