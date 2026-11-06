/* SPDX-License-Identifier: MIT -- native Preferences editor. */
#import <UIKit/UIKit.h>
#import <objc/message.h>
#import <notify.h>
#import "settings.h"
@interface PSSpecifier : NSObject
@property(copy) NSString *name;
- (id)propertyForKey:(NSString *)key;
- (void)setProperty:(id)value forKey:(NSString *)key;
@end
@interface PSListController : UIViewController
- (NSArray *)loadSpecifiersFromPlistName:(NSString *)name target:(id)target;
- (void)reloadSpecifiers;
@end
@interface IOS26AnimListController : PSListController
@property(strong) NSArray *editorSpecifiers;
@end
@implementation IOS26AnimListController
- (NSArray *)specifiers {
 if(!self.editorSpecifiers){
  self.title=@"iOS26Anim";
  NSArray *specifiers=[self loadSpecifiersFromPlistName:@"Root" target:self];
  NSBundle *bundle=[NSBundle bundleForClass:self.class];
  for(PSSpecifier *spec in specifiers){
   if(spec.name.length)spec.name=[bundle localizedStringForKey:spec.name value:spec.name table:@"Root"];
   NSString *footer=[spec propertyForKey:@"footerText"];
   if(footer.length)[spec setProperty:[bundle localizedStringForKey:footer value:footer table:@"Root"] forKey:@"footerText"];
  }
  self.editorSpecifiers=specifiers;
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
