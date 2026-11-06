/* SPDX-License-Identifier: MIT -- shared preference keys and bootstrap paths. */
#include "runtime.h"
#ifdef XUI_ROOTFUL
#define XUI_PREFIX ""
#else
#define XUI_PREFIX "/var/jb"
#endif
#define XUI_DOMAIN "local.rajok.xui"
#define XUI_PREFS_BUNDLE XUI_PREFIX "/Library/PreferenceBundles/XUIRootlessPrefs.bundle"
static id xui_string(const char *s){return((id(*)(id,SEL,const char*))objc_msgSend)(objc_getClass("NSString"),sel_registerName("stringWithUTF8String:"),s);}
static id xui_preferences(void){
 static id prefs;
 if(!prefs){
  if(!objc_getClass("HBPreferences"))dlopen(XUI_PREFIX "/Library/Frameworks/Cephei.framework/Cephei",2);
  Class cls=objc_getClass("HBPreferences");
  if(cls)prefs=((id(*)(id,SEL,id))objc_msgSend)(SEND(cls,"alloc"),sel_registerName("initWithIdentifier:"),xui_string(XUI_DOMAIN));
 }
 return prefs;
}
static id xui_value(const char *key){return((id(*)(id,SEL,id))objc_msgSend)(xui_preferences(),sel_registerName("objectForKey:"),xui_string(key));}
static BOOL xui_option(const char *key){id value=xui_value(key);return value?((BOOL(*)(id,SEL))objc_msgSend)(value,sel_registerName("boolValue")):1;}
static BOOL xui_russian(void){
 id language=xui_value("Language");
 const char *code=language?((const char*(*)(id,SEL))objc_msgSend)(language,sel_registerName("UTF8String")):"system";
 if(!strcmp(code,"ru"))return 1;
 if(!strcmp(code,"en"))return 0;
 id languages=SEND(objc_getClass("NSLocale"),"preferredLanguages");
 if(!((usize(*)(id,SEL))objc_msgSend)(languages,sel_registerName("count")))return 0;
 id first=((id(*)(id,SEL,usize))objc_msgSend)(languages,sel_registerName("objectAtIndex:"),0);
 code=((const char*(*)(id,SEL))objc_msgSend)(first,sel_registerName("UTF8String"));
 return code&&strstr(code,"ru")==code;
}

/* Share only four UI booleans with sandboxed UIKit clients. Preferences remain
 * authoritative in Settings/SpringBoard; no private data enters this channel. */
#define XUI_SPACING_STATE XUI_DOMAIN "/SpacingState"
#define XUI_SPACING_MAGIC (0x585549ULL<<32)
static unsigned long long xui_spacing_state(void){
 return XUI_SPACING_MAGIC | (unsigned)xui_option("Enabled") |
  ((unsigned)xui_option("HomeIndicator")<<1) |
  ((unsigned)xui_option("AppInsets")<<2) |
  ((unsigned)xui_option("KeyboardInsets")<<3);
}
static void xui_publish_spacing(void){
 static int token=-1;
 if(token<0&&notify_register_check(XUI_SPACING_STATE,&token))return;
 if(!notify_set_state(token,xui_spacing_state()))notify_post(XUI_SPACING_STATE);
}
