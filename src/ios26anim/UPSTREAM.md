# iOS26Anim — iOS 16 compatibility build

- Upstream: https://github.com/Stealth2115/Ios26-anim
- Author: Stealth2115 (upstream control metadata originally uses placeholder `yourname`).
- Source revision: `75c5e1c71672d9290b44599d21b56d5fa3476f69`.
- Source Tweak.x SHA-256: `2fcffe1a6bbcc5bb3b3b6fa8c03c990e2e210e0242715bfba92f7f238d28fc7b`.
- Local package maintainer: rajok.
- Local version: `0.2.0` (packaging revision, not an upstream release).

The animation source is fetched at the pinned revision during the build. Its
animation constants are preserved; local hook changes are described below. These packages target ARM64
(non-arm64e), including iPhone 8 on iOS 16. Both rootless and rootful layouts are
provided. Neither has been tested on a device by this repository maintainer.
The filter limits injection to SpringBoard. This is a separate tweak from XUI.

The upstream revision sets 1.50-second opening/closing durations, 0.90 spring
response, 0.60 damping and a 0.95-alpha blur overlay. Those are the actual code
values; some of the upstream README descriptions describe older tuning.

The upstream source is not covered by the XUI repository's MIT license. It is
not vendored in this repository; consult upstream for source and licensing.

## Local compatibility changes in rajok.2

The initial upstream build had no visible effect on the owner's phone.
The spring class target is corrected from SBFluidBehaviorSettings to
SBFFluidBehaviorSettings. Response and damping are also applied through
setResponse: and setDampingRatio: so cached spring presets receive the values
when constructed. Existing getter overrides remain. Startup and first setter
calls emit brief diagnostic messages. Animation tuning is unchanged.
This fix was cross-built and statically checked; on-device verification is
pending because the owner is updating wirelessly through Sileo.

## 0.2.0: native animation editor

Settings → iOS26Anim adds Enabled, separate opening/closing duration, spring
response and damping, BlurEnabled and blur opacity. Seven numeric values are
clamped to finite supported ranges; defaults preserve upstream tuning. The
shared domain is local.rajok.ios26anim. Both layouts use Cephei to read/write the
same mobile preferences. Changes apply after Apply and respring so cached spring
presets are rebuilt. Reset restores only this tweak's known settings. Russian
and English localization follows the system language. Disabling the tweak skips
all hook installation on the next SpringBoard start. The editor does not add a
new iOS 26 transition trajectory or claim a full visual copy.
