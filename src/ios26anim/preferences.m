/* SPDX-License-Identifier: MIT -- native Preferences editor. */
#import <UIKit/UIKit.h>
#import <objc/message.h>
#import <notify.h>
#import "settings.h"
@interface PSSpecifier : NSObject
@property(copy) NSString *name;
- (id)propertyForKey:(NSString *)key;
- (void)setProperty:(id)value forKey:(NSString *)key;
+ (instancetype)preferenceSpecifierNamed:(NSString *)name target:(id)target set:(SEL)set get:(SEL)get detail:(Class)detail cell:(NSInteger)cell edit:(Class)edit;
- (void)setButtonAction:(SEL)action;
@end
@interface PSListController : UIViewController
- (void)setSpecifiers:(NSArray *)specifiers;
- (void)reloadSpecifiers;
@end
@interface IOS26AnimListController : PSListController
@property(strong) NSArray *editorSpecifiers;
@end
/* Resolve our own loaded image, including rootless relocated bundle paths. */
static NSBundle *IOS26EditorBundle(void) {
 static NSBundle *bundle;
 if(!bundle){
  Dl_info image={0};
  NSString *path=@(IOS26_PREFIX "/Library/PreferenceBundles/IOS26AnimPrefs.bundle");
  if(dladdr((const void *)IOS26EditorBundle,&image)&&image.dli_fname)
   path=[@(image.dli_fname) stringByDeletingLastPathComponent];
  bundle=[NSBundle bundleWithPath:path];
 }
 return bundle;
}
static NSString *IOS26Text(NSString *key) {
 static NSDictionary *fallback;
 if(!fallback)fallback=@{
  @"GENERAL":@"General",@"ENABLED":@"Enable iOS26Anim",
  @"APPLY_FOOTER":@"Changes take effect after Apply and respring.",
  @"OPENING":@"Opening an app",@"CLOSING":@"Closing an app",
  @"DURATION":@"Duration (seconds)",@"RESPONSE":@"Spring response (seconds)",
  @"DAMPING":@"Damping ratio",@"SPRING_FOOTER":@"Lower response is faster. Lower damping adds bounce.",
  @"BLUR":@"Transition blur",@"BLUR_ENABLED":@"Enable blur overlay",@"BLUR_ALPHA":@"Blur opacity",
  @"BLUR_FOOTER":@"Controls the temporary blur layer during transitions.",
  @"ACTIONS":@"Apply changes",@"RESET":@"Reset to defaults",@"APPLY":@"Apply and respring",
  @"LIMITS_FOOTER":@"Tunes supported iOS 16 transitions."
 };
 return [IOS26EditorBundle() localizedStringForKey:key value:fallback[key]?:key table:@"Root"]?:fallback[key]?:key;
}
static PSSpecifier *IOS26Row(id target,NSString *label,NSString *key,NSInteger cell) {
 PSSpecifier *specifier=[PSSpecifier preferenceSpecifierNamed:IOS26Text(label) target:target
  set:key?@selector(setPreferenceValue:specifier:):NULL
  get:key?@selector(readPreferenceValue:):NULL detail:Nil cell:cell edit:Nil];
 if(key){[specifier setProperty:key forKey:@"key"];[specifier setProperty:@IOS26_DOMAIN forKey:@"defaults"];}
 return specifier;
}
static void IOS26Group(NSMutableArray *rows,id target,NSString *title,NSString *footer) {
 PSSpecifier *group=IOS26Row(target,title,nil,0);
 [group setProperty:IOS26Text(footer) forKey:@"footerText"];
 [rows addObject:group];
}
static void IOS26Switch(NSMutableArray *rows,id target,NSString *label,NSString *key) {
 PSSpecifier *specifier=IOS26Row(target,label,key,6);
 [specifier setProperty:@YES forKey:@"default"];
 [rows addObject:specifier];
}
static void IOS26Slider(NSMutableArray *rows,id target,NSString *label,NSString *key) {
 const IOS26Number *setting=IOS26NumberForKey(key);
 PSSpecifier *specifier=IOS26Row(target,label,key,5);
 [specifier setProperty:@(setting->initial) forKey:@"default"];
 [specifier setProperty:@(setting->minimum) forKey:@"min"];
 [specifier setProperty:@(setting->maximum) forKey:@"max"];
 [specifier setProperty:@YES forKey:@"showValue"];
 [specifier setProperty:@NO forKey:@"isSegmented"];
 [rows addObject:specifier];
}
@implementation IOS26AnimListController
- (NSArray *)specifiers {
 if(!self.editorSpecifiers){
  self.title=@"iOS26Anim";
  NSMutableArray *rows=[NSMutableArray array];
  IOS26Group(rows,self,@"GENERAL",@"APPLY_FOOTER");
  IOS26Switch(rows,self,@"ENABLED",@"Enabled");
  IOS26Group(rows,self,@"OPENING",@"SPRING_FOOTER");
  IOS26Slider(rows,self,@"DURATION",@"OpenDuration");
  IOS26Slider(rows,self,@"RESPONSE",@"OpenResponse");
  IOS26Slider(rows,self,@"DAMPING",@"OpenDamping");
  IOS26Group(rows,self,@"CLOSING",@"SPRING_FOOTER");
  IOS26Slider(rows,self,@"DURATION",@"CloseDuration");
  IOS26Slider(rows,self,@"RESPONSE",@"CloseResponse");
  IOS26Slider(rows,self,@"DAMPING",@"CloseDamping");
  IOS26Group(rows,self,@"BLUR",@"BLUR_FOOTER");
  IOS26Switch(rows,self,@"BLUR_ENABLED",@"BlurEnabled");
  IOS26Slider(rows,self,@"BLUR_ALPHA",@"BlurAlpha");
  IOS26Group(rows,self,@"ACTIONS",@"LIMITS_FOOTER");
  PSSpecifier *reset=IOS26Row(self,@"RESET",nil,13);
  [reset setButtonAction:@selector(resetDefaults)];[rows addObject:reset];
  PSSpecifier *apply=IOS26Row(self,@"APPLY",nil,13);
  [apply setButtonAction:@selector(applySettings)];[rows addObject:apply];
  self.editorSpecifiers=rows;
  [self setSpecifiers:rows];
 }
 return self.editorSpecifiers;
}
- (id)readPreferenceValue:(PSSpecifier *)specifier {
 NSString *key=[specifier propertyForKey:@"key"];
 return IOS26NumberForKey(key)?@(IOS26ReadNumber(key)):@(IOS26ReadBool(key));
}
- (void)setPreferenceValue:(id)value specifier:(PSSpecifier *)specifier {
 NSString *key=[specifier propertyForKey:@"key"];
 const IOS26Number *setting=IOS26NumberForKey(key);
 if(setting){if(![value respondsToSelector:@selector(doubleValue)])return;value=@(IOS26Clamp([value doubleValue],setting));}
 else {if(![key isEqualToString:@"Enabled"]&&![key isEqualToString:@"BlurEnabled"])return;value=@([value boolValue]);}
 [IOS26Preferences() setObject:value forKey:key];
 notify_post(IOS26_DOMAIN "/ReloadPrefs");
}
- (void)resetDefaults {
 for(unsigned i=0;i<sizeof(IOS26Numbers)/sizeof(*IOS26Numbers);i++)
  [IOS26Preferences() setObject:@(IOS26Numbers[i].initial) forKey:[NSString stringWithUTF8String:IOS26Numbers[i].key]];
 [IOS26Preferences() setObject:@YES forKey:@"Enabled"];
 [IOS26Preferences() setObject:@YES forKey:@"BlurEnabled"];
 notify_post(IOS26_DOMAIN "/ReloadPrefs");
 self.editorSpecifiers=nil;
 [self reloadSpecifiers];
}
- (void)applySettings {
 IOS26Preferences();
 Class controller=NSClassFromString(@"HBRespringController");
 if([controller respondsToSelector:@selector(respring)])
  ((void(*)(id,SEL))objc_msgSend)(controller,@selector(respring));
}
@end
