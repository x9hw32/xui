/* SPDX-License-Identifier: MIT -- native Settings preference pane. */
#include "settings.h"
static char specs_key;
static id bundle;
static id localized(const char *key){
 if(!bundle){
  ImageInfo image={0};id path=xui_string(XUI_PREFS_BUNDLE);
  if(dladdr((const void*)localized,&image)&&image.filename)path=SEND(xui_string(image.filename),"stringByDeletingLastPathComponent");
  bundle=((id(*)(id,SEL,id))objc_msgSend)(objc_getClass("NSBundle"),sel_registerName("bundleWithPath:"),path);SEND(bundle,"retain");
 }
 const char *lang=xui_russian()?"ru":"en";
 id path=((id(*)(id,SEL,id,id))objc_msgSend)(bundle,sel_registerName("pathForResource:ofType:"),xui_string(lang),xui_string("lproj"));
 id language=path?((id(*)(id,SEL,id))objc_msgSend)(objc_getClass("NSBundle"),sel_registerName("bundleWithPath:"),path):bundle;
 return((id(*)(id,SEL,id,id,id))objc_msgSend)(language,sel_registerName("localizedStringForKey:value:table:"),xui_string(key),xui_string(key),xui_string("Localizable"));
}
static void property(id spec,const char *key,id value){((void(*)(id,SEL,id,id))objc_msgSend)(spec,sel_registerName("setProperty:forKey:"),value,xui_string(key));}
static void append(id list,id value){((void(*)(id,SEL,id))objc_msgSend)(list,sel_registerName("addObject:"),value);}
static id specifier(id owner,const char *label,const char *key,long cell){
 id spec=((id(*)(id,SEL,id,id,SEL,SEL,Class,long,Class))objc_msgSend)(objc_getClass("PSSpecifier"),sel_registerName("preferenceSpecifierNamed:target:set:get:detail:cell:edit:"),localized(label),owner,key?sel_registerName("setPreferenceValue:specifier:"):0,key?sel_registerName("readPreferenceValue:"):0,cell==2?objc_getClass("PSListItemsController"):0,cell,0);
 if(key){property(spec,"key",xui_string(key));property(spec,"defaults",xui_string(XUI_DOMAIN));}
 return spec;
}
static void group(id list,id owner,const char *title,const char *footer){
 id s=specifier(owner,title,0,0);if(footer)property(s,"footerText",localized(footer));append(list,s);
}
static id read_value(id obj,SEL cmd,id spec){
 (void)obj;(void)cmd;
 id key=((id(*)(id,SEL,id))objc_msgSend)(spec,sel_registerName("propertyForKey:"),xui_string("key"));
 const char *name=((const char*(*)(id,SEL))objc_msgSend)(key,sel_registerName("UTF8String"));
 if((!strcmp(name,"AppInsets")||!strcmp(name,"KeyboardInsets"))&&!xui_option("HomeIndicator"))
  return((id(*)(id,SEL,BOOL))objc_msgSend)(objc_getClass("NSNumber"),sel_registerName("numberWithBool:"),0);
 id value=((id(*)(id,SEL,id))objc_msgSend)(xui_preferences(),sel_registerName("objectForKey:"),key);
 if(value)return value;
 if(!strcmp(name,"Language"))return xui_string("system");
 return((id(*)(id,SEL,BOOL))objc_msgSend)(objc_getClass("NSNumber"),sel_registerName("numberWithBool:"),xui_option(name));
}
static void write_value(id obj,SEL cmd,id value,id spec){
 (void)cmd;
 id key=((id(*)(id,SEL,id))objc_msgSend)(spec,sel_registerName("propertyForKey:"),xui_string("key"));
 ((void(*)(id,SEL,id,id))objc_msgSend)(xui_preferences(),sel_registerName("setObject:forKey:"),value,key);
 const char *changed=((const char*(*)(id,SEL))objc_msgSend)(key,sel_registerName("UTF8String"));
 /* Persist dependent switches as well as refreshing their displayed cells. */
 if(!strcmp(changed,"HomeIndicator")){
  const char *dependencies[]={"AppInsets","KeyboardInsets"};
  for(unsigned i=0;i<2;i++)
   ((void(*)(id,SEL,id,id))objc_msgSend)(xui_preferences(),sel_registerName("setObject:forKey:"),value,xui_string(dependencies[i]));
 }
 xui_publish_spacing();
 notify_post(XUI_DOMAIN "/ReloadPrefs");
 if(!strcmp(changed,"Language")||!strcmp(changed,"HomeIndicator")){
  objc_setAssociatedObject(obj,&specs_key,0,1);
  SEND(obj,"reloadSpecifiers");
 }
}
static void apply_settings(id obj,SEL cmd){
 (void)obj;(void)cmd;
 char *args[]={XUI_PREFIX "/usr/bin/killall","-TERM","Camera",0};
 char *env[]={"PATH=/usr/bin:/bin:/usr/sbin:/sbin:" XUI_PREFIX "/usr/bin",0};
 int pid=0;if(!posix_spawn(&pid,args[0],0,0,args,env))waitpid(pid,0,0);
 ((void(*)(id,SEL))objc_msgSend)(objc_getClass("HBRespringController"),sel_registerName("respring"));
}
static id specifiers(id obj,SEL cmd){
 (void)cmd;id cached=objc_getAssociatedObject(obj,&specs_key);if(cached)return cached;
 id list=SEND(objc_getClass("NSMutableArray"),"array");
 ((void(*)(id,SEL,id))objc_msgSend)(obj,sel_registerName("setTitle:"),xui_string("XUI"));
 group(list,obj,"GENERAL","APPLY_FOOTER");
 append(list,specifier(obj,"ENABLED","Enabled",6));
 id language=specifier(obj,"LANGUAGE","Language",2);
 id values=SEND(objc_getClass("NSMutableArray"),"array"),titles=SEND(objc_getClass("NSMutableArray"),"array");
 append(values,xui_string("system"));append(values,xui_string("ru"));append(values,xui_string("en"));
 append(titles,localized("SYSTEM_LANGUAGE"));append(titles,xui_string("Русский"));append(titles,xui_string("English"));
 ((void(*)(id,SEL,id,id))objc_msgSend)(language,sel_registerName("setValues:titles:"),values,titles);
 append(list,language);
 group(list,obj,"NAVIGATION","NAVIGATION_FOOTER");
 append(list,specifier(obj,"HOME_GESTURES","HomeGestures",6));
 append(list,specifier(obj,"HOME_INDICATOR","HomeIndicator",6));
 group(list,obj,"APPEARANCE","STOCK_STATUS_FOOTER");
 append(list,specifier(obj,"DOCK","Dock",6));
 id app_insets=specifier(obj,"APP_INSETS","AppInsets",6);
 id keyboard_insets=specifier(obj,"KEYBOARD_INSETS","KeyboardInsets",6);
 id spacing_enabled=((id(*)(id,SEL,BOOL))objc_msgSend)(objc_getClass("NSNumber"),sel_registerName("numberWithBool:"),xui_option("HomeIndicator"));
 property(app_insets,"enabled",spacing_enabled);property(keyboard_insets,"enabled",spacing_enabled);
 append(list,app_insets);append(list,keyboard_insets);
 group(list,obj,"LOCK_SCREEN","LOCK_FOOTER");
 append(list,specifier(obj,"FLASHLIGHT","FlashlightButton",6));
 append(list,specifier(obj,"CAMERA","CameraButton",6));
 append(list,specifier(obj,"LOCK_INDICATOR","LockScreenFooter",6));
 group(list,obj,"APPLY_GROUP","AUTHOR");
 id action=specifier(obj,"APPLY",0,13);
 ((void(*)(id,SEL,SEL))objc_msgSend)(action,sel_registerName("setButtonAction:"),sel_registerName("applySettings"));
 append(list,action);
 objc_setAssociatedObject(obj,&specs_key,list,1);
 ((void(*)(id,SEL,id))objc_msgSend)(obj,sel_registerName("setSpecifiers:"),list);
 return list;
}
__attribute__((constructor)) static void init_preferences(void){
 xui_preferences();
 Class base=objc_getClass("PSListController");if(!base)return;
 Class cls=objc_allocateClassPair(base,"XUIRootlessListController",0);if(!cls)return;
 class_addMethod(cls,sel_registerName("specifiers"),(IMP)specifiers,"@16@0:8");
 class_addMethod(cls,sel_registerName("readPreferenceValue:"),(IMP)read_value,"@24@0:8@16");
 class_addMethod(cls,sel_registerName("setPreferenceValue:specifier:"),(IMP)write_value,"v32@0:8@16@24");
 class_addMethod(cls,sel_registerName("applySettings"),(IMP)apply_settings,"v16@0:8");
 objc_registerClassPair(cls);
}
