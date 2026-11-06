# iOS26Anim — upstream build

- Upstream: https://github.com/Stealth2115/Ios26-anim
- Author: Stealth2115 (upstream control metadata originally uses placeholder `yourname`).
- Source revision: `75c5e1c71672d9290b44599d21b56d5fa3476f69`.
- Source Tweak.x SHA-256: `2fcffe1a6bbcc5bb3b3b6fa8c03c990e2e210e0242715bfba92f7f238d28fc7b`.
- Local package maintainer: rajok.
- Local version: `0.1.0+rajok.1` (packaging revision, not an upstream release).

The animation source is fetched at the pinned revision during the build. Its
animation constants and hooks are not changed. These packages target ARM64
(non-arm64e), including iPhone 8 on iOS 16. Both rootless and rootful layouts are
provided. Neither has been tested on a device by this repository maintainer.
The filter limits injection to SpringBoard. This is a separate tweak from XUI.

The upstream revision sets 1.50-second opening/closing durations, 0.90 spring
response, 0.60 damping and a 0.95-alpha blur overlay. Those are the actual code
values; some of the upstream README descriptions describe older tuning.

The upstream source is not covered by the XUI repository's MIT license. It is
not vendored in this repository; consult upstream for source and licensing.
