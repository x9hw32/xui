#!/usr/bin/env python3
"""Wire the patched upstream animation parameters to persistent editor values."""
from pathlib import Path
import sys
path=Path(sys.argv[1]);source=path.read_text()
source=source.replace('#import <objc/runtime.h>', '#import <objc/runtime.h>\n#import "settings.h"',1)
keys={'kOpenResponse':'OpenResponse','kCloseResponse':'CloseResponse','kOpenDamping':'OpenDamping','kCloseDamping':'CloseDamping','kOpenDuration':'OpenDuration','kCloseDuration':'CloseDuration','kGlassBlurAlpha':'BlurAlpha'}
for variable in keys:
 marker='static const double '+variable
 if marker not in source:raise SystemExit('Missing upstream constant: '+variable)
 source=source.replace(marker,'static double '+variable,1)
config='static BOOL kIOS26Enabled=YES, kIOS26BlurEnabled=YES;\nstatic void IOS26ReadConfiguration(void) {\n kIOS26Enabled=IOS26ReadBool(@"Enabled");\n kIOS26BlurEnabled=IOS26ReadBool(@"BlurEnabled");\n'
for variable,key in keys.items():config+=f' {variable}=IOS26ReadNumber(@"{key}");\n'
config+='}\n\n'
source=source.replace('// Forward decls for private classes / structs',config+'// Forward decls for private classes / structs',1)
source=source.replace('static void presentGlassOverlay(void) {','static void presentGlassOverlay(void) {\n    if(!kIOS26BlurEnabled || kGlassBlurAlpha<=0.0)return;',1)
source=source.replace('%ctor {','''%ctor {
    IOS26ReadConfiguration();
    if(!kIOS26Enabled){NSLog(@"[iOS26Anim 0.2.0] disabled in Settings");return;}
    %init;
''',1)
source=source.replace('[iOS26Anim rajok.2]','[iOS26Anim 0.2.0]')
path.write_text(source)
