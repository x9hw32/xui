/* SPDX-License-Identifier: MIT
 * XUI, local iPhone8/iOS16 UI customization. Native gestures/actions.
 */
#include "settings.h"
static void (*hook_message)(Class,SEL,IMP,IMP *);
static IMP orig_set_grabber,orig_quick_layout;
static IMP orig_quick_background,orig_teachable,orig_dock,orig_fixed_footer;
static char pill_key,label_key,cc_header_key,camera_gesture_key,camera_delegate_key,camera_owner_key;
static Class camera_delegate_class;
static id current_cover;
static IMP orig_camera_layout,orig_camera_window;
static IMP orig_window_insets,orig_keyboard_padding,orig_keyboard_instance_padding,orig_keyboard_layout;
static IMP orig_scene[4],orig_hw_home,orig_hw_lock,orig_hw_volume;
static IMP orig_home_type,orig_quick_view_layout,orig_quick_button_layout;
static IMP orig_quick_color,orig_quick_style,orig_page_hidden,orig_cc_appear,orig_cc_disappear;
static int cc_hooks_installed;
static void install_cc_hooks(void);
static int is_springboard;
static BOOL cfg_enabled,cfg_home,cfg_indicator,cfg_cc,cfg_dock,cfg_apps,cfg_keyboard,cfg_flash,cfg_camera,cfg_footer,cfg_ru;
static int logfd=-1;
static id current_dock,current_quick;
static void view_tree(id,unsigned);
static void logtext(const char *s){if(logfd>=0)write(logfd,s,strlen(s));}
static BOOL yes(id obj,SEL cmd){(void)obj;(void)cmd;return 1;}
static BOOL camera_supported(id obj,SEL cmd){(void)obj;(void)cmd;return cfg_camera;}
static BOOL flash_supported(id obj,SEL cmd){(void)obj;(void)cmd;return cfg_flash;}
static unsigned long one(id obj,SEL cmd){(void)obj;(void)cmd;if(is_springboard)install_cc_hooks();return 1;}
static long home_type(id obj,SEL cmd){
 ImageInfo image={0};
 if(dladdr(__builtin_return_address(0),&image)&&image.filename&&
    ((cfg_footer&&strstr(image.filename,"/CoverSheet.framework/"))||((cfg_footer||cfg_flash||cfg_camera)&&strstr(image.filename,"/SpringBoardUI.framework/"))))return 2;
 return((long(*)(id,SEL))orig_home_type)(obj,cmd);
}
static Rect bounds(id obj){return((Rect(*)(id,SEL))objc_msgSend)(obj,sel_registerName("bounds"));}
static void frame(id obj,Rect r){if(obj)((void(*)(id,SEL,Rect))objc_msgSend)(obj,sel_registerName("setFrame:"),r);}
static BOOL has(id obj,const char *name){return obj&&class_getInstanceMethod(object_getClass(obj),sel_registerName(name));}
static void set_grabber(id obj,SEL cmd,BOOL value){(void)value;((void(*)(id,SEL,BOOL))orig_set_grabber)(obj,cmd,1);}
static Insets app_insets(Insets in,int portrait){
 if(in.top>0)in.top=portrait?20:(in.top>20?20:in.top);
 in.bottom=portrait?20:12;
 return in;
}
static Insets window_insets(id obj,SEL cmd){
 Insets in=((Insets(*)(id,SEL))orig_window_insets)(obj,cmd);
 if(!cfg_enabled)return in;
 if(is_springboard){
  ImageInfo image={0};
  /* LiquidAss tests these insets to select its full-screen dock/QA hosts.
   * Only its drawing code sees this; SpringBoard's widget grid stays native. */
  if((cfg_dock||cfg_flash||cfg_camera)&&dladdr(__builtin_return_address(0),&image)&&image.filename&&strstr(image.filename,"/liquidass.dylib"))in.bottom=20;
  return in;
 }
 const char *name=class_getName(object_getClass(obj));
 if(strstr(name,"Keyboard")||strstr(name,"TextEffects")||strstr(name,"StatusBar"))return in;
 if(!has(obj,"windowLevel")||((double(*)(id,SEL))objc_msgSend)(obj,sel_registerName("windowLevel"))!=0)return in;
 Rect b=bounds(obj);if(b.width<250||b.height<250)return in;
 if(!cfg_indicator||!cfg_apps){in.bottom=0;return in;}
 return app_insets(in,b.height>=b.width);
}
static Insets scene_insets(id obj,SEL cmd){
 const char *name=sel_getName(cmd);unsigned index=0;
 if(!strcmp(name,"safeAreaInsetsPortraitUpsideDown"))index=1;
 else if(!strcmp(name,"safeAreaInsetsLandscapeLeft"))index=2;
 else if(!strcmp(name,"safeAreaInsetsLandscapeRight"))index=3;
 Insets original=((Insets(*)(id,SEL))orig_scene[index])(obj,cmd);
 if(cfg_enabled&&cfg_indicator&&cfg_apps)return app_insets(original,index<2);
 original.bottom=0;return original;
}
static Insets keyboard_padding(id obj,SEL cmd,long orientation,id mode){
 Insets in=((Insets(*)(id,SEL,long,id))orig_keyboard_padding)(obj,cmd,orientation,mode);
 if(cfg_enabled&&cfg_indicator&&cfg_keyboard)in.bottom=(orientation==1||orientation==2)?12:8;
 return in;
}
static Insets keyboard_instance_padding(id obj,SEL cmd,long orientation,id mode){
 Insets in=((Insets(*)(id,SEL,long,id))orig_keyboard_instance_padding)(obj,cmd,orientation,mode);
 in.bottom=cfg_enabled&&cfg_indicator&&cfg_keyboard?((orientation==1||orientation==2)?12:8):0;
 return in;
}
static void reload_responder(id view,unsigned depth){
 if(!view||depth>30)return;
 if(has(view,"isFirstResponder")&&((BOOL(*)(id,SEL))objc_msgSend)(view,sel_registerName("isFirstResponder"))){
  if(has(view,"reloadInputViews"))SEND(view,"reloadInputViews");return;
 }
 id children=SEND(view,"subviews");usize count=((usize(*)(id,SEL))objc_msgSend)(children,sel_registerName("count"));
 for(usize i=0;i<count;i++)reload_responder(((id(*)(id,SEL,usize))objc_msgSend)(children,sel_registerName("objectAtIndex:"),i),depth+1);
}
static void refresh_spacing(void){
 if(is_springboard)return;
 id app=SEND(objc_getClass("UIApplication"),"sharedApplication");
 id windows=SEND(app,"windows");usize count=((usize(*)(id,SEL))objc_msgSend)(windows,sel_registerName("count"));
 for(usize i=0;i<count;i++){
  id window=((id(*)(id,SEL,usize))objc_msgSend)(windows,sel_registerName("objectAtIndex:"),i);
  if(has(window,"_sceneSettingsSafeAreaInsetsDidChange"))SEND(window,"_sceneSettingsSafeAreaInsetsDidChange");
  if(has(window,"_updateSafeAreaInsets"))SEND(window,"_updateSafeAreaInsets");
  SEND(window,"setNeedsLayout");SEND(window,"layoutIfNeeded");reload_responder(window,0);
 }
}
static void keyboard_layout(id obj,SEL cmd){
 ((void(*)(id,SEL))orig_keyboard_layout)(obj,cmd);
 if(!cfg_enabled||!cfg_indicator||!cfg_keyboard)return;
 /* Paint the allocated footer with the keyboard's own surrounding color. */
 id parent=SEND(obj,"superview");id color=parent?SEND(parent,"backgroundColor"):(id)0;
 if(color&&has(obj,"setBackgroundColor:"))((void(*)(id,SEL,id))objc_msgSend)(obj,sel_registerName("setBackgroundColor:"),color);
}
static BOOL yellow_color(id color){
 if(!has(color,"getRed:green:blue:alpha:"))return 0;
 double r=0,g=0,b=0,a=0;
 BOOL rgb=((BOOL(*)(id,SEL,double*,double*,double*,double*))objc_msgSend)(color,sel_registerName("getRed:green:blue:alpha:"),&r,&g,&b,&a);
 return rgb&&r>=.8&&g>=.65&&b<=.35&&a>.05;
}
static BOOL yellow_cgcolor(id color){
 if(!color)return 0;
 id ui=((id(*)(id,SEL,id))objc_msgSend)(objc_getClass("UIColor"),sel_registerName("colorWithCGColor:"),color);
 return yellow_color(ui);
}
static void clear_yellow_layer(id layer,unsigned depth){
 if(!layer||depth>5)return;
 const char *getters[]={"borderColor","strokeColor"};
 const char *setters[]={"setBorderColor:","setStrokeColor:"};
 for(unsigned i=0;i<2;i++)if(has(layer,getters[i])){
  if(yellow_cgcolor(SEND(layer,getters[i])))((void(*)(id,SEL,id))objc_msgSend)(layer,sel_registerName(setters[i]),0);
 }
 Rect rect=((Rect(*)(id,SEL))objc_msgSend)(layer,sel_registerName("bounds"));
 double radius=((double(*)(id,SEL))objc_msgSend)(layer,sel_registerName("cornerRadius"));
 double side=rect.width<rect.height?rect.width:rect.height;
 if(side>=40&&radius<side*.35){
  const char *fills[]={"backgroundColor","fillColor"};
  const char *setfills[]={"setBackgroundColor:","setFillColor:"};
  for(unsigned i=0;i<2;i++)if(has(layer,fills[i])&&yellow_cgcolor(SEND(layer,fills[i]))){
   ((void(*)(id,SEL,id))objc_msgSend)(layer,sel_registerName(setfills[i]),0);
   logtext("QuickActions: cleared yellow layer ");logtext(class_getName(object_getClass(layer)));logtext("\n");
  }
 }
 id list=SEND(layer,"sublayers");usize count=((usize(*)(id,SEL))objc_msgSend)(list,sel_registerName("count"));
 for(usize i=0;i<count;i++)clear_yellow_layer(((id(*)(id,SEL,usize))objc_msgSend)(list,sel_registerName("objectAtIndex:"),i),depth+1);
}
static void clear_yellow_fields(id obj,unsigned depth){
 if(!obj||depth>5)return;
 Rect r=bounds(obj);id layer=SEND(obj,"layer");
 double radius=((double(*)(id,SEL))objc_msgSend)(layer,sel_registerName("cornerRadius"));
 double side=r.width<r.height?r.width:r.height;
 /* Remove yellow rectangular field backgrounds, retaining circular native
  * material, the selected flashlight tint and LiquidAss's glass contents. */
 if(side>=40&&radius<side*.35&&yellow_color(SEND(obj,"backgroundColor"))){
  ((void(*)(id,SEL,id))objc_msgSend)(obj,sel_registerName("setBackgroundColor:"),SEND(objc_getClass("UIColor"),"clearColor"));
  logtext("QuickActions: cleared yellow field ");logtext(class_getName(object_getClass(obj)));logtext("\n");
 }
 clear_yellow_layer(layer,0);
 id children=SEND(obj,"subviews");usize n=((usize(*)(id,SEL))objc_msgSend)(children,sel_registerName("count"));
 for(usize i=0;i<n;i++)clear_yellow_fields(((id(*)(id,SEL,usize))objc_msgSend)(children,sel_registerName("objectAtIndex:"),i),depth+1);
}
static void quick_view_layout(id obj,SEL cmd){
 ((void(*)(id,SEL))orig_quick_view_layout)(obj,cmd);
 clear_yellow_fields(obj,0);
}
static BOOL quick_insets(id obj){
 if(!has(obj,"edgeInsets")||!has(obj,"setEdgeInsets:"))return 0;
 Insets in=((Insets(*)(id,SEL))objc_msgSend)(obj,sel_registerName("edgeInsets"));
 if(in.top==3&&in.left==3&&in.bottom==3&&in.right==3)return 0;
 ((void(*)(id,SEL,Insets))objc_msgSend)(obj,sel_registerName("setEdgeInsets:"),(Insets){3,3,3,3});
 return 1;
}
static void quick_button_layout(id obj,SEL cmd){
 ((void(*)(id,SEL))orig_quick_button_layout)(obj,cmd);
 if(quick_insets(obj))((void(*)(id,SEL))orig_quick_button_layout)(obj,cmd);
 clear_yellow_fields(obj,0);
}
static void quick_color(id obj,SEL cmd,id color){
 if(yellow_color(color))color=SEND(objc_getClass("UIColor"),"clearColor");
 ((void(*)(id,SEL,id))orig_quick_color)(obj,cmd,color);
}
static void quick_style(id obj,SEL cmd){
 ((void(*)(id,SEL))orig_quick_style)(obj,cmd);
 quick_insets(obj);clear_yellow_fields(obj,0);
}
static void quick_layout(id obj,SEL cmd){
 ((void(*)(id,SEL))orig_quick_layout)(obj,cmd);
 if(current_quick!=obj){SEND(obj,"retain");if(current_quick)SEND(current_quick,"release");current_quick=obj;}
 /* Keep geometry in the moving CoverSheet's own coordinate space. Window
  * conversion during an interactive transition bakes its transform into frame. */
 Rect b=bounds(obj);if(b.width<250||b.height<350)return;
 double y=b.y+b.height-114;if(y<b.y)return;
 frame(has(obj,"flashlightButton")?SEND(obj,"flashlightButton"):(id)0,(Rect){b.x+39,y,56,56});
 frame(has(obj,"cameraButton")?SEND(obj,"cameraButton"):(id)0,(Rect){b.x+b.width-95,y,56,56});
 quick_insets(has(obj,"flashlightButton")?SEND(obj,"flashlightButton"):0);
 quick_insets(has(obj,"cameraButton")?SEND(obj,"cameraButton"):0);
}
static void quick_background(id obj,SEL cmd,BOOL visible){
 (void)visible;((void(*)(id,SEL,BOOL))orig_quick_background)(obj,cmd,1);
}
static void dock_layout(id obj,SEL cmd){
 ((void(*)(id,SEL))orig_dock)(obj,cmd);current_dock=obj;
 id bg=has(obj,"backgroundView")?SEND(obj,"backgroundView"):(id)0;if(!bg)return;
 Rect b=bounds(obj);if(b.width<250||b.height<40||b.height>180)return;
 frame(bg,(Rect){12,6,b.width-24,b.height-12});
 id layer=SEND(bg,"layer");((void(*)(id,SEL,double))objc_msgSend)(layer,sel_registerName("setCornerRadius:"),28);
 /* LiquidAss remains the sole painter; only the Apple host's geometry changes. */
 id children=SEND(obj,"subviews");usize n=((usize(*)(id,SEL))objc_msgSend)(children,sel_registerName("count"));
 for(usize i=0;i<n;i++){id v=((id(*)(id,SEL,usize))objc_msgSend)(children,sel_registerName("objectAtIndex:"),i);
  if(!strcmp(class_getName(object_getClass(v)),"SBHighlightView"))((void(*)(id,SEL,BOOL))objc_msgSend)(v,sel_registerName("setHidden:"),1);
 }
}
static id native_string(const char *text){return((id(*)(id,SEL,const char*))objc_msgSend)(objc_getClass("NSString"),sel_registerName("stringWithUTF8String:"),text);}
static BOOL camera_page_visible(id controller){
 if(!controller)return 0;
 id page=has(controller,"visiblePageViewController")?SEND(controller,"visiblePageViewController"):0;
 if(page&&strstr(class_getName(object_getClass(page)),"Camera"))return 1;
 if(has(controller,"lastSettledPageIndex")&&has(controller,"_indexOfCameraPage")){
  usize page_index=((usize(*)(id,SEL))objc_msgSend)(controller,sel_registerName("lastSettledPageIndex"));
  usize camera_index=((usize(*)(id,SEL))objc_msgSend)(controller,sel_registerName("_indexOfCameraPage"));
  return camera_index<10&&page_index==camera_index;
 }
 return 0;
}
static BOOL camera_gesture_begin(id obj,SEL cmd,id gesture){
 (void)cmd;(void)gesture;
 if(!xui_option("Enabled")||!xui_option("HomeGestures"))return 0;
 if(!is_springboard)return 1;
 return camera_page_visible(objc_getAssociatedObject(obj,&camera_owner_key));
}
static BOOL camera_gesture_simultaneous(id obj,SEL cmd,id gesture,id other){
 (void)obj;(void)cmd;(void)gesture;
 const char *name=class_getName(object_getClass(other));
 if(strstr(name,"Tap")||strstr(name,"LongPress")||strstr(name,"Shutter"))return 0;
 for(id view=SEND(other,"view");view;view=SEND(view,"superview")){
  name=class_getName(object_getClass(view));
  if(strstr(name,"Shutter")||strstr(name,"CaptureButton"))return 0;
 }
 return 1;
}
static void camera_pan(id obj,SEL cmd,id gesture){
 (void)obj;(void)cmd;
 long state=((long(*)(id,SEL))objc_msgSend)(gesture,sel_registerName("state"));
 if(state!=3)return;
 id view=SEND(gesture,"view");
 Size delta=((Size(*)(id,SEL,id))objc_msgSend)(gesture,sel_registerName("translationInView:"),view);
 Size speed=((Size(*)(id,SEL,id))objc_msgSend)(gesture,sel_registerName("velocityInView:"),view);
 if(delta.height < -30||speed.height < -250)notify_post("local.rajok.xui.camera-return");
}
static void install_camera_gesture(id controller){
 if(!cfg_home||!camera_delegate_class||!has(controller,"view"))return;
 id root=SEND(controller,"view");if(!root)return;
 if(!is_springboard){
  id window=SEND(root,"window");if(!window)return;
  id owner=SEND(window,"rootViewController");
  if(owner&&has(owner,"view")){controller=owner;root=SEND(owner,"view");}
 }
 if(objc_getAssociatedObject(root,&camera_gesture_key))return;
 id delegate=SEND(SEND(camera_delegate_class,"alloc"),"init");
 objc_setAssociatedObject(delegate,&camera_owner_key,controller,0);
 objc_setAssociatedObject(root,&camera_delegate_key,delegate,1);
 id gesture=((id(*)(id,SEL,id,SEL))objc_msgSend)(SEND(objc_getClass("UIScreenEdgePanGestureRecognizer"),"alloc"),sel_registerName("initWithTarget:action:"),delegate,sel_registerName("handlePan:"));
 ((void(*)(id,SEL,usize))objc_msgSend)(gesture,sel_registerName("setEdges:"),4);
 ((void(*)(id,SEL,BOOL))objc_msgSend)(gesture,sel_registerName("setCancelsTouchesInView:"),1);
 ((void(*)(id,SEL,BOOL))objc_msgSend)(gesture,sel_registerName("setDelaysTouchesBegan:"),1);
 ((void(*)(id,SEL,id))objc_msgSend)(gesture,sel_registerName("setDelegate:"),delegate);
 ((void(*)(id,SEL,id))objc_msgSend)(root,sel_registerName("addGestureRecognizer:"),gesture);
 objc_setAssociatedObject(root,&camera_gesture_key,gesture,1);
 SEND(gesture,"release");SEND(delegate,"release");
 logtext("Camera return: bottom-edge gesture installed\n");
}
static void camera_layout(id obj,SEL cmd){
 ((void(*)(id,SEL))orig_camera_layout)(obj,cmd);
 install_camera_gesture(obj);
}
static void camera_window(id obj,SEL cmd){
 ((void(*)(id,SEL))orig_camera_window)(obj,cmd);
 if(((double(*)(id,SEL))objc_msgSend)(obj,sel_registerName("windowLevel"))==0){
  id controller=SEND(obj,"rootViewController");if(controller)install_camera_gesture(controller);
 }
}
static void return_from_camera(void){
 if(!cfg_home||!current_cover)return;
 id manager=SEND(objc_getClass("SBLockScreenManager"),"sharedInstance");
 BOOL locked=has(manager,"isUILocked")?((BOOL(*)(id,SEL))objc_msgSend)(manager,sel_registerName("isUILocked")):0;
 if(!locked&&!camera_page_visible(current_cover))return;
 if(has(current_cover,"activateMainPageWithCompletion:")){
  ((void(*)(id,SEL,id))objc_msgSend)(current_cover,sel_registerName("activateMainPageWithCompletion:"),0);
  logtext("Camera return: activated native main lock-screen page\n");
 }
}
static void teachable_layout(id obj,SEL cmd){
 ((void(*)(id,SEL))orig_teachable)(obj,cmd);
 if(current_cover!=obj){SEND(obj,"retain");if(current_cover)SEND(current_cover,"release");current_cover=obj;}
 install_camera_gesture(obj);
 if(!cfg_footer)return;
 id controller=obj;obj=SEND(controller,"view");if(!obj)return;
 Rect b=bounds(obj);if(b.width<250||b.height<350)return;
 id label=objc_getAssociatedObject(obj,&label_key);
 if(!label&&objc_getClass("SBUICallToActionLabel")){
  label=((id(*)(id,SEL,Rect))objc_msgSend)(SEND(objc_getClass("SBUICallToActionLabel"),"alloc"),sel_registerName("initWithFrame:"),(Rect){0,0,b.width-24,24});
  objc_setAssociatedObject(obj,&label_key,label,1);
  ((void(*)(id,SEL,id))objc_msgSend)(obj,sel_registerName("addSubview:"),label);SEND(label,"release");
  ((void(*)(id,SEL,BOOL))objc_msgSend)(label,sel_registerName("setUserInteractionEnabled:"),0);
 }
 if(label){
  id manager=SEND(objc_getClass("SBLockScreenManager"),"sharedInstance");
  BOOL locked=has(manager,"isUILocked")?((BOOL(*)(id,SEL))objc_msgSend)(manager,sel_registerName("isUILocked")):1;
  id text=native_string(cfg_ru?(locked?"Смахните вверх, чтобы разблокировать":"Смахните вверх, чтобы открыть"):(locked?"Swipe up to unlock":"Swipe up to open"));
  if(has(label,"setString:"))((void(*)(id,SEL,id))objc_msgSend)(label,sel_registerName("setString:"),text);
  else if(has(label,"setText:"))((void(*)(id,SEL,id))objc_msgSend)(label,sel_registerName("setText:"),text);
  BOOL passcode=has(controller,"isPasscodeLockVisible")?((BOOL(*)(id,SEL))objc_msgSend)(controller,sel_registerName("isPasscodeLockVisible")):0;
  if(has(label,"setVisible:animated:"))((void(*)(id,SEL,BOOL,BOOL))objc_msgSend)(label,sel_registerName("setVisible:animated:"),!passcode,0);
  ((void(*)(id,SEL,BOOL))objc_msgSend)(label,sel_registerName("setHidden:"),passcode);
  ((void(*)(id,SEL,double))objc_msgSend)(label,sel_registerName("setAlpha:"),1);
  frame(label,(Rect){b.x+12,b.y+b.height-60,b.width-24,24});
 }
 id pill=objc_getAssociatedObject(obj,&pill_key);
 if(!pill&&objc_getClass("CSHomeAffordanceView")){
  pill=SEND(SEND(objc_getClass("CSHomeAffordanceView"),"alloc"),"init");
  objc_setAssociatedObject(obj,&pill_key,pill,1);
  ((void(*)(id,SEL,id))objc_msgSend)(obj,sel_registerName("addSubview:"),pill);
  SEND(pill,"release");
 }
 if(pill){
  id legibility=has(obj,"legibilitySettings")?SEND(obj,"legibilitySettings"):(id)0;
  if(!legibility&&has(controller,"legibilityProvider")){id lp=SEND(controller,"legibilityProvider");if(has(lp,"legibilitySettings"))legibility=SEND(lp,"legibilitySettings");}
  if(legibility&&has(pill,"setLegibilitySettings:"))((void(*)(id,SEL,id))objc_msgSend)(pill,sel_registerName("setLegibilitySettings:"),legibility);
  if(has(pill,"setStyle:"))((void(*)(id,SEL,usize))objc_msgSend)(pill,sel_registerName("setStyle:"),2);
  ((void(*)(id,SEL,BOOL))objc_msgSend)(pill,sel_registerName("setHidden:"),0);
  ((void(*)(id,SEL,double))objc_msgSend)(pill,sel_registerName("setAlpha:"),1);
  ((void(*)(id,SEL,BOOL))objc_msgSend)(pill,sel_registerName("setUserInteractionEnabled:"),0);
  Size size={135,5};if(has(pill,"suggestedSizeForContentWidth:"))size=((Size(*)(id,SEL,double))objc_msgSend)(pill,sel_registerName("suggestedSizeForContentWidth:"),b.width);
  if(!(size.width>30&&size.width<b.width))size.width=135;if(!(size.height>1&&size.height<15))size.height=5;
  frame(pill,(Rect){b.x+(b.width-size.width)/2,b.y+b.height-9-size.height,size.width,size.height});
 }
 static int reported;if(!reported++){
  char line[220];Rect local=bounds(obj);
  snprintf(line,sizeof(line),"LOCK FOOTER local %.0fx%.0f bounds %.0fx%.0f label=%s pill=%s\n",local.width,local.height,b.width,b.height,label?class_getName(object_getClass(label)):"nil",pill?class_getName(object_getClass(pill)):"nil");logtext(line);
 }
}
static void fixed_footer_layout(id obj,SEL cmd){
 ((void(*)(id,SEL))orig_fixed_footer)(obj,cmd);
 /* Only remove the old Home-button prompt, leaving charging/status text. */
 const char *names[]={"callToActionLabel","alternateCallToActionLabel","pageControl"};
 for(unsigned i=0;i<3;i++)if(has(obj,names[i])){
  id label=SEND(obj,names[i]);
  if(label)((void(*)(id,SEL,BOOL))objc_msgSend)(label,sel_registerName("setHidden:"),1);
 }
}
static void page_hidden(id obj,SEL cmd,BOOL hidden){
 (void)hidden;((void(*)(id,SEL,BOOL))orig_page_hidden)(obj,cmd,1);
}
static id find_cc_header(id root,unsigned depth){
 if(!root||depth>6)return 0;
 const char *name=class_getName(object_getClass(root));
 if(!strcmp(name,"CCUIHeaderPocketView"))return root;
 id children=SEND(root,"subviews");usize n=((usize(*)(id,SEL))objc_msgSend)(children,sel_registerName("count"));
 for(usize i=0;i<n;i++){
  id found=find_cc_header(((id(*)(id,SEL,usize))objc_msgSend)(children,sel_registerName("objectAtIndex:"),i),depth+1);
  if(found)return found;
 }
 return 0;
}
static void cc_appear(id obj,SEL cmd,BOOL animated){
 id header=objc_getAssociatedObject(obj,&cc_header_key);
 if(header){((void(*)(id,SEL,BOOL))objc_msgSend)(header,sel_registerName("setHidden:"),0);objc_setAssociatedObject(obj,&cc_header_key,0,1);}
 ((void(*)(id,SEL,BOOL))orig_cc_appear)(obj,cmd,animated);
}
static void cc_disappear(id obj,SEL cmd,BOOL animated){
 ((void(*)(id,SEL,BOOL))orig_cc_disappear)(obj,cmd,animated);
 id header=has(obj,"overlayHeaderView")?SEND(obj,"overlayHeaderView"):0;
 if(!header)header=find_cc_header(SEND(obj,"view"),0);
 if(header&&!((BOOL(*)(id,SEL))objc_msgSend)(header,sel_registerName("isHidden"))){
  objc_setAssociatedObject(obj,&cc_header_key,header,1);
  ((void(*)(id,SEL,BOOL))objc_msgSend)(header,sel_registerName("setHidden:"),1);
  logtext("CC: header hidden at dismissal\n");
 }else logtext(header?"CC: header already hidden\n":"CC: header not found\n");
}
static id hw_home(id obj,SEL cmd,id a,long type,id c,id d){(void)type;return((id(*)(id,SEL,id,long,id,id))orig_hw_home)(obj,cmd,a,1,c,d);}
static id hw_lock(id obj,SEL cmd,id a,id b,id c,id d,id e,id f,long type,BOOL g){(void)type;return((id(*)(id,SEL,id,id,id,id,id,id,long,BOOL))orig_hw_lock)(obj,cmd,a,b,c,d,e,f,1,g);}
static id hw_volume(id obj,SEL cmd,id a,id b,long type){(void)type;return((id(*)(id,SEL,id,id,long))orig_hw_volume)(obj,cmd,a,b,1);}
static void view_tree(id obj,unsigned depth){
 if(!obj||depth>4)return;
 char line[260];Rect r=((Rect(*)(id,SEL))objc_msgSend)(obj,sel_registerName("frame"));
 snprintf(line,sizeof(line),"VIEW %u %s %.1f %.1f %.1f %.1f\n",depth,class_getName(object_getClass(obj)),r.x,r.y,r.width,r.height);logtext(line);
 id children=SEND(obj,"subviews");usize n=((usize(*)(id,SEL))objc_msgSend)(children,sel_registerName("count"));
 for(usize i=0;i<n&&i<24;i++)view_tree(((id(*)(id,SEL,usize))objc_msgSend)(children,sel_registerName("objectAtIndex:"),i),depth+1);
}
static int hook(const char *name,const char *method,int meta,char result,IMP replacement,IMP *original){
 Class cls=objc_getClass(name);SEL sel=sel_registerName(method);
 Method m=cls?(meta?class_getClassMethod(cls,sel):class_getInstanceMethod(cls,sel)):(Method)0;
 const char *encoding=m?method_getTypeEncoding(m):(const char*)0;
 logtext(name);logtext(meta?" +":" -");logtext(method);logtext(" ");
 if(!encoding||encoding[0]!=result){logtext("SKIP (missing/type mismatch)\n");return 0;}
 logtext(encoding);logtext(" HOOK\n");IMP unused=0;
 hook_message(meta?object_getClass(cls):cls,sel,replacement,original?original:&unused);return 1;
}
static void install_cc_hooks(void){
 if(!cfg_cc||cc_hooks_installed||!hook_message||!objc_getClass("CCUIModularControlCenterOverlayViewController"))return;
 cc_hooks_installed=1;
 hook("CCUIModularControlCenterOverlayViewController","viewWillAppear:",0,'v',(IMP)cc_appear,&orig_cc_appear);
 hook("CCUIModularControlCenterOverlayViewController","viewWillDisappear:",0,'v',(IMP)cc_disappear,&orig_cc_disappear);
}
__attribute__((constructor)) static void init_xui(void){
 char machine[64]={0},release[64]={0};usize size=sizeof(machine);
 if(sysctlbyname("hw.machine",machine,&size,0,0))return;
 if(strcmp(machine,"iPhone10,4")&&strcmp(machine,"iPhone10,1"))return;
 size=sizeof(release);if(sysctlbyname("kern.osrelease",release,&size,0,0)||release[0]!='2'||release[1]!='2'||release[2]!='.')return;
 if(access("/var/mobile/Library/Preferences/local.rajok.xui.disabled",0)==0)return;
 int springboard=objc_getClass("SpringBoard")!=0;is_springboard=springboard;
 cfg_enabled=xui_option("Enabled");
 if(!cfg_enabled&&springboard)return;
 cfg_home=xui_option("HomeGestures");cfg_indicator=xui_option("HomeIndicator");
 cfg_cc=cfg_home;cfg_dock=xui_option("Dock");
 cfg_apps=xui_option("AppInsets");cfg_keyboard=xui_option("KeyboardInsets");
 if(springboard)xui_publish_spacing();
 else {
  static int initial_state_token;
  unsigned long long state=0;
  if(!notify_register_check(XUI_SPACING_STATE,&initial_state_token)&&
     !notify_get_state(initial_state_token,&state)&&(state&0xffffffff00000000ULL)==XUI_SPACING_MAGIC){
   cfg_enabled=(state&1)!=0;cfg_indicator=(state&2)!=0;
   cfg_apps=(state&4)!=0;cfg_keyboard=(state&8)!=0;
  }
 }
 cfg_flash=xui_option("FlashlightButton");cfg_camera=xui_option("CameraButton");
 cfg_footer=xui_option("LockScreenFooter");cfg_ru=xui_russian();
 hook_message=dlsym((void*)-2,"MSHookMessageEx");
 if(!hook_message){
#ifdef XUI_ROOTFUL
  void *lib=dlopen("/usr/lib/libsubstrate.dylib",2);
#else
  void *lib=dlopen("/var/jb/usr/lib/libsubstrate.dylib",2);
#endif
  if(lib)hook_message=dlsym(lib,"MSHookMessageEx");
 }
 if(!hook_message)return;
 if(springboard){logfd=open("/var/mobile/Library/Logs/XUIRootless.log",1|8|512,0600);logtext("XUI 1.0.0 init\n");}
 /* Guard above restricts us to iPhone8: 375pt at 2x = 750px. UIKit forbids
  * UIScreen.mainScreen during SpringBoard's dyld initializer phase. */
 if(cfg_enabled&&cfg_home)camera_delegate_class=objc_allocateClassPair(objc_getClass("NSObject"),"XUICameraReturnDelegate",0);
 if(camera_delegate_class){
  class_addMethod(camera_delegate_class,sel_registerName("gestureRecognizerShouldBegin:"),(IMP)camera_gesture_begin,"B24@0:8@16");
  class_addMethod(camera_delegate_class,sel_registerName("gestureRecognizer:shouldRecognizeSimultaneouslyWithGestureRecognizer:"),(IMP)camera_gesture_simultaneous,"B32@0:8@16@24");
  class_addMethod(camera_delegate_class,sel_registerName("handlePan:"),(IMP)camera_pan,"v24@0:8@16");
  objc_registerClassPair(camera_delegate_class);
 }
 id bundle=SEND(objc_getClass("NSBundle"),"mainBundle"),identifier=SEND(bundle,"bundleIdentifier");
 const char *bundle_id=identifier?((const char*(*)(id,SEL))objc_msgSend)(identifier,sel_registerName("UTF8String")):"";
 if(cfg_enabled&&cfg_home&&!strcmp(bundle_id,"com.apple.camera")){
  hook("CAMViewfinderViewController","viewDidLayoutSubviews",0,'v',(IMP)camera_layout,&orig_camera_layout);
  hook("UIWindow","makeKeyAndVisible",0,'v',(IMP)camera_window,&orig_camera_window);
 }
 hook("UIWindow","safeAreaInsets",0,'{',(IMP)window_insets,&orig_window_insets);
 const char *scenes[]={"safeAreaInsetsPortrait","safeAreaInsetsPortraitUpsideDown","safeAreaInsetsLandscapeLeft","safeAreaInsetsLandscapeRight"};
 if(!springboard)for(unsigned i=0;i<4;i++)hook("UIApplicationSceneSettings",scenes[i],0,'{',(IMP)scene_insets,&orig_scene[i]);
 hook("UIKeyboardImpl","deviceSpecificPaddingForInterfaceOrientation:inputMode:",1,'{',(IMP)keyboard_padding,&orig_keyboard_padding);
 hook("UIKeyboardImpl","deviceSpecificPaddingForInterfaceOrientation:inputMode:",0,'{',(IMP)keyboard_instance_padding,&orig_keyboard_instance_padding);
 hook("UIKeyboardDockView","layoutSubviews",0,'v',(IMP)keyboard_layout,&orig_keyboard_layout);
 if(!springboard){
  static int spacing_token;
  notify_register_dispatch(XUI_SPACING_STATE,&spacing_token,&_dispatch_main_q,^(int token){
   unsigned long long state=0;
   if(!notify_get_state(token,&state)&&(state&0xffffffff00000000ULL)==XUI_SPACING_MAGIC){
    cfg_enabled=(state&1)!=0;cfg_indicator=(state&2)!=0;
    cfg_apps=(state&4)!=0;cfg_keyboard=(state&8)!=0;
    refresh_spacing();
   }
  });
  return;
 }
 if(cfg_footer||cfg_flash||cfg_camera)hook("BSPlatform","homeButtonType",0,'q',(IMP)home_type,&orig_home_type);
 hook("SBHomeHardwareButton","initWithScreenshotGestureRecognizer:homeButtonType:buttonActions:gestureRecognizerConfiguration:",0,'@',(IMP)hw_home,&orig_hw_home);
 hook("SBLockHardwareButton","initWithScreenshotGestureRecognizer:shutdownGestureRecognizer:proximitySensorManager:homeHardwareButton:volumeHardwareButton:buttonActions:homeButtonType:createGestures:",0,'@',(IMP)hw_lock,&orig_hw_lock);
 hook("SBVolumeHardwareButton","initWithScreenshotGestureRecognizer:shutdownGestureRecognizer:homeButtonType:",0,'@',(IMP)hw_volume,&orig_hw_volume);
 if(cfg_home)hook("SBHomeGestureSettings","isHomeGestureEnabled",0,'B',(IMP)yes,0);
 if(cfg_cc)hook("CCSControlCenterDefaults","_defaultPresentationGesture",0,'Q',(IMP)one,0);
 if(cfg_cc)hook("SBControlCenterController","presentingEdge",0,'Q',(IMP)one,0);
 if(cfg_indicator)hook("SBFHomeGrabberSettings","isEnabled",0,'B',(IMP)yes,0);
 if(cfg_indicator)hook("SBFHomeGrabberSettings","setEnabled:",0,'v',(IMP)set_grabber,&orig_set_grabber);
 if(cfg_flash||cfg_camera){
 hook("CSQuickActionsViewController","deviceSupportsButtons",1,'B',(IMP)yes,0);
 hook("CSQuickActionsViewController","hasCamera",0,'B',(IMP)camera_supported,0);
 hook("CSQuickActionsViewController","hasFlashlight",0,'B',(IMP)flash_supported,0);
 hook("CSQuickActionsView","wantsQuickActions",0,'B',(IMP)yes,0);
 hook("CSQuickActionsView","_prototypingAllowsButtons",0,'B',(IMP)yes,0);
 hook("CSQuickActionsView","_layoutQuickActionButtons",0,'v',(IMP)quick_layout,&orig_quick_layout);
 hook("CSQuickActionsView","layoutSubviews",0,'v',(IMP)quick_view_layout,&orig_quick_view_layout);
 hook("CSQuickActionsButton","layoutSubviews",0,'v',(IMP)quick_button_layout,&orig_quick_button_layout);
 hook("CSQuickActionsButton","setBackgroundColor:",0,'v',(IMP)quick_color,&orig_quick_color);
 hook("CSQuickActionsButton","_updateStyle",0,'v',(IMP)quick_style,&orig_quick_style);
 hook("CSQuickActionsView","_setButtonBackgroundVisible:",0,'v',(IMP)quick_background,&orig_quick_background);
 }
 if(cfg_footer||cfg_home)hook("CSCoverSheetViewController","viewDidLayoutSubviews",0,'v',(IMP)teachable_layout,&orig_teachable);
 if(cfg_footer)hook("CSFixedFooterView","layoutSubviews",0,'v',(IMP)fixed_footer_layout,&orig_fixed_footer);
 if(cfg_footer)hook("CSPageControl","setHidden:",0,'v',(IMP)page_hidden,&orig_page_hidden);
 install_cc_hooks();
 if(cfg_dock)hook("SBDockView","layoutSubviews",0,'v',(IMP)dock_layout,&orig_dock);

 static int camera_notify_token;
 notify_register_dispatch("local.rajok.xui.camera-return",&camera_notify_token,&_dispatch_main_q,^(int token){(void)token;return_from_camera();});
 static int notify_token;
 notify_register_dispatch("local.rajok.xui.capture",&notify_token,&_dispatch_main_q,^(int token){(void)token;logtext("QUICK ACTION GEOMETRY\n");view_tree(current_quick,0);view_tree(current_dock,0);
 });

 logtext("XUI 1.0.0 init finished\n");
}
