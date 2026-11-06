#!/usr/bin/env python3
"""Apply the local iOS 16 spring-settings compatibility fix to pinned upstream."""
from pathlib import Path
import sys

path = Path(sys.argv[1])
source = path.read_text()
if '%hook SBFluidBehaviorSettings\n' not in source:
    raise SystemExit('Unexpected upstream spring hook; refusing to patch')
source = source.replace('SBFluidBehaviorSettings', 'SBFFluidBehaviorSettings')
anchor = '%hook SBFFluidBehaviorSettings\n'
source = source.replace(anchor, anchor + '''// Apply the parameters while presets are constructed, including callers
// that subsequently consume cached spring coefficients instead of getters.
- (void)setResponse:(double)value {
    double response = gIsOpening ? kOpenResponse : kCloseResponse;
    static BOOL logged = NO;
    if (!logged) {
        NSLog(@"[iOS26Anim rajok.2] setResponse %.3f -> %.3f", value, response);
        logged = YES;
    }
    %orig(response);
}
- (void)setDampingRatio:(double)value {
    double damping = gIsOpening ? kOpenDamping : kCloseDamping;
    static BOOL logged = NO;
    if (!logged) {
        NSLog(@"[iOS26Anim rajok.2] setDampingRatio %.3f -> %.3f", value, damping);
        logged = YES;
    }
    %orig(damping);
}
''', 1)
source = source.replace('%ctor {', '''%ctor {
    Class spring = objc_getClass("SBFFluidBehaviorSettings");
    NSLog(@"[iOS26Anim rajok.2] spring class=%@ response=%@ setter=%@",
          spring ? @"present" : @"MISSING",
          class_getInstanceMethod(spring, @selector(response)) ? @"present" : @"MISSING",
          class_getInstanceMethod(spring, @selector(setResponse:)) ? @"present" : @"MISSING");
''', 1)
path.write_text(source)
