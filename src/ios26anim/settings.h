/* SPDX-License-Identifier: MIT -- shared iOS26Anim editor configuration. */
#import <Foundation/Foundation.h>
#include <dlfcn.h>
#include <math.h>
#ifdef IOS26_ROOTFUL
#define IOS26_PREFIX ""
#else
#define IOS26_PREFIX "/var/jb"
#endif
#define IOS26_DOMAIN "local.rajok.ios26anim"
@interface IOS26PreferencesStore : NSObject
- (instancetype)initWithIdentifier:(NSString *)identifier;
- (id)objectForKey:(NSString *)key;
- (void)setObject:(id)value forKey:(NSString *)key;
@end
typedef struct { const char *key; double initial, minimum, maximum; } IOS26Number;
static const IOS26Number IOS26Numbers[] = {
 {"OpenDuration",1.50,0.15,2.50}, {"CloseDuration",1.50,0.15,2.50},
 {"OpenResponse",0.90,0.15,1.50}, {"CloseResponse",0.90,0.15,1.50},
 {"OpenDamping",0.60,0.25,1.25}, {"CloseDamping",0.60,0.25,1.25},
 {"BlurAlpha",0.95,0.00,1.00}
};
static inline IOS26PreferencesStore *IOS26Preferences(void) {
 static IOS26PreferencesStore *preferences;
 if (!preferences) {
  Class cls=NSClassFromString(@"HBPreferences");
  if (!cls) { dlopen(IOS26_PREFIX "/Library/Frameworks/Cephei.framework/Cephei",RTLD_NOW);cls=NSClassFromString(@"HBPreferences"); }
  if(cls)preferences=[[cls alloc] initWithIdentifier:@IOS26_DOMAIN];
 }
 return preferences;
}
static inline const IOS26Number *IOS26NumberForKey(NSString *key) {
 for(unsigned i=0;i<sizeof(IOS26Numbers)/sizeof(*IOS26Numbers);i++)
  if([key isEqualToString:[NSString stringWithUTF8String:IOS26Numbers[i].key]])return &IOS26Numbers[i];
 return NULL;
}
static inline double IOS26Clamp(double value,const IOS26Number *setting) {
 if(!isfinite(value))return setting->initial;
 return fmax(setting->minimum,fmin(setting->maximum,value));
}
static inline double IOS26ReadNumber(NSString *key) {
 const IOS26Number *setting=IOS26NumberForKey(key);if(!setting)return 0;
 id value=[IOS26Preferences() objectForKey:key];
 return value&&[value respondsToSelector:@selector(doubleValue)]?IOS26Clamp([value doubleValue],setting):setting->initial;
}
static inline BOOL IOS26ReadBool(NSString *key) {
 id value=[IOS26Preferences() objectForKey:key];
 return value&&[value respondsToSelector:@selector(boolValue)]?[value boolValue]:YES;
}
