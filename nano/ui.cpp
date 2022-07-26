/*
 * Nano Library
 *
 * Copyright (C) 2022, Meta-Sonic
 * All rights reserved.
 *
 * Proprietary and confidential.
 * Any unauthorized copying, alteration, distribution, transmission, performance,
 * display or other use of this material is strictly prohibited.
 *
 * Written by Alexandre Arsenault <alx.arsenault@gmail.com>
 */

#include <nano/ui.h>
#include <nano/objc.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <dispatch/dispatch.h>

extern "C" {
extern CFStringRef NSViewFrameDidChangeNotification;
}

NANO_CLANG_DIAGNOSTIC_PUSH()
NANO_CLANG_DIAGNOSTIC(warning, "-Weverything")
NANO_CLANG_DIAGNOSTIC(ignored, "-Wc++98-compat")
NANO_CLANG_DIAGNOSTIC(ignored, "-Wc++98-compat-bind-to-temporary-copy")

namespace nano {

///
///
///

NANO_INLINE_CXPR objc::ns_uint_t uiNSTrackingMouseEnteredAndExited = 0x01;
NANO_INLINE_CXPR objc::ns_uint_t uiNSTrackingActiveInKeyWindow = 0x20;
NANO_INLINE_CXPR objc::ns_uint_t uiNSTrackingMouseMoved = 0x02;
NANO_INLINE_CXPR objc::ns_uint_t uiNSTrackingInVisibleRect = 0x200;

namespace {
  inline nano::point<int> get_location_in_view(native_event_handle handle, nano::view* view) {
    CGPoint locInWindow = objc::call<CGPoint>(handle, "locationInWindow");
    return objc::call<CGPoint, CGPoint, objc::obj_t*>(reinterpret_cast<objc::obj_t*>(view->get_native_handle()),
        "convertPoint:fromView:", static_cast<CGPoint>(locInWindow), nullptr);
  }

  inline event_modifiers get_event_modifiers_from_cg_event(CGEventRef evt) {
    event_modifiers mods = event_modifiers::none;
    CGEventFlags flags = CGEventGetFlags(evt);

    if (flags & kCGEventFlagMaskShift) {
      mods = mods | event_modifiers::shift;
    }

    if (flags & kCGEventFlagMaskControl) {
      mods = mods | event_modifiers::control;
    }

    if (flags & kCGEventFlagMaskAlternate) {
      mods = mods | event_modifiers::alt;
    }

    if (flags & kCGEventFlagMaskCommand) {
      mods = mods | event_modifiers::command;
    }

    if (flags & kCGEventFlagMaskSecondaryFn) {
      mods = mods | event_modifiers::function;
    }

    std::int64_t btnNumber = CGEventGetIntegerValueField(evt, kCGMouseEventButtonNumber);
    switch (btnNumber) {
    case kCGMouseButtonLeft:
      mods = mods | event_modifiers::left_mouse_down;
      break;

    case kCGMouseButtonRight:
      mods = mods | event_modifiers::right_mouse_down;
      break;

    case kCGMouseButtonCenter:
      mods = mods | event_modifiers::middle_mouse_down;
      break;
    }

    return mods;
  }

  inline event_type get_event_type_from_cg_event(CGEventRef evt) {
    NANO_CLANG_PUSH_WARNING("-Wswitch-enum")

    switch (CGEventGetType(evt)) {
    case kCGEventLeftMouseDown:
      return event_type::left_mouse_down;

    case kCGEventLeftMouseUp:
      return event_type::left_mouse_up;

    case kCGEventRightMouseDown:
      return event_type::right_mouse_down;

    case kCGEventRightMouseUp:
      return event_type::right_mouse_up;

    case kCGEventMouseMoved:
      return event_type::mouse_moved;

    case kCGEventLeftMouseDragged:
      return event_type::left_mouse_dragged;

    case kCGEventRightMouseDragged:
      return event_type::right_mouse_dragged;

    case kCGEventKeyDown:
      return event_type::key_down;

    case kCGEventKeyUp:
      return event_type::key_up;

    case kCGEventFlagsChanged:
      return event_type::key_flags_changed;

    case kCGEventScrollWheel:
      return event_type::scroll_wheel;

    case kCGEventTabletPointer:
      return event_type::tablet_pointer;

    case kCGEventTabletProximity:
      return event_type::tablet_proximity;

    case kCGEventOtherMouseDown:
      return event_type::other_mouse_down;

    case kCGEventOtherMouseUp:
      return event_type::other_mouse_up;

    case kCGEventOtherMouseDragged:
      return event_type::other_mouse_dragged;

    default:
      return event_type::none;
    }

    NANO_CLANG_POP_WARNING()

    NANO_CLANG_PUSH_WARNING("-Wunreachable-code-return")
    return event_type::none;
    NANO_CLANG_POP_WARNING()
  }
} // namespace.

static nano::point<float> s_click_position = { 0.0f, 0.0f };

event::event(native_event_handle handle, nano::view* view)
    : event() {
  m_native_handle = handle;
  m_view = view;

  CGEventRef evt = objc::call<CGEventRef>(m_native_handle, "CGEvent");
  m_type = get_event_type_from_cg_event(evt);

  // For non-mouse events the return value of locationInWindow is undefined.
  if (nano::is_mouse_event(m_type)) {
    m_position = get_location_in_view(handle, view);

    if (m_type == event_type::left_mouse_down || m_type == event_type::right_mouse_down
        || m_type == event_type::other_mouse_down) {
      m_click_position = m_position;
      s_click_position = m_position;
    }
    else {
      m_click_position = s_click_position;
    }
  }

  // This property is only valid for scroll wheel, mouse-move, mouse-drag, and
  // swipe events. For swipe events, a nonzero value represents a horizontal
  // swipe: -1 for swipe right and 1 for swipe left.
  if (nano::is_scroll_or_drag_event(m_type)) {
    m_wheel_delta = nano::point<CGFloat>(
        objc::call<CGFloat>(m_native_handle, "deltaX"), objc::call<CGFloat>(m_native_handle, "deltaY"));
  }

  if (nano::is_click_event(m_type)) {
    m_click_count = static_cast<int>(CGEventGetIntegerValueField(evt, kCGMouseEventClickState));
  }

  m_event_modifiers = get_event_modifiers_from_cg_event(evt);
  m_timestamp = CGEventGetTimestamp(evt);
}

nano::point<float> event::get_screen_position() const noexcept {
  CGEventRef evt = objc::call<CGEventRef>(m_native_handle, "CGEvent");
  return CGEventGetLocation(evt);
}

const nano::point<float> event::get_window_position() const noexcept {
  nano::rect<float> frame = objc::call<CGRect>(get_native_window(), "contentLayoutRect");
  nano::point<float> pos = objc::call<CGPoint>(m_native_handle, "locationInWindow");
  pos.y = frame.height - pos.y;
  return pos;
}

// std::uint64_t event::get_timestamp() const noexcept
//{
//     CGEventRef evt = nano::call<CGEventRef>((id)m_native_handle, "CGEvent");
//     return CGEventGetTimestamp(evt);
// }

native_window_handle event::get_native_window() const noexcept {
  return reinterpret_cast<native_window_handle>(objc::call<objc::obj_t*>(m_native_handle, "window"));
}

const nano::point<float> event::get_bounds_position() const noexcept {
  return m_position - m_view->get_bounds().position;
}

std::u16string event::get_key() const noexcept {
  CGEventRef evt = objc::call<CGEventRef>(m_native_handle, "CGEvent");
  UniCharCount length = 0;
  std::u16string str(20, 0);
  CGEventKeyboardGetUnicodeString(evt, 20, &length, reinterpret_cast<UniChar*>(str.data()));

  return str;
}
// CGEventKeyboardGetUnicodeString(CGEventRef event, UniCharCount maxStringLength, UniCharCount *actualStringLength,
// UniChar *unicodeString);

class window_object {
public:
  static constexpr const char* className = "NanoWindowObject";
  static constexpr const char* baseName = "NSObject";
  static constexpr const char* valueName = "owner";

  window_object(nano::view* view, window_flags flags)
      : m_view(view) {

    m_obj = classObject.create_instance();
    objc::call(m_obj, "init");
    ClassObject::set_pointer(m_obj, this);

    //        constexpr unsigned int uiNSWindowStyleMaskBorderless = 0;
    constexpr objc::ns_uint_t uiNSWindowStyleMaskTitled = 1 << 0;
    constexpr objc::ns_uint_t uiNSWindowStyleMaskClosable = 1 << 1;
    constexpr objc::ns_uint_t uiNSWindowStyleMaskMiniaturizable = 1 << 2;
    constexpr objc::ns_uint_t uiNSWindowStyleMaskResizable = 1 << 3;
    constexpr objc::ns_uint_t uiNSWindowStyleMaskUtilityWindow = 1 << 4;
    constexpr objc::ns_uint_t uiNSWindowStyleMaskFullSizeContentView = 1 << 15;

    bool isPanel = (flags & window_flags::panel) != 0;
    objc::ns_uint_t nsFlags = 0;

    if ((flags & window_flags::titled) != 0) {
      nsFlags = nsFlags | uiNSWindowStyleMaskTitled;
    }

    if ((flags & window_flags::closable) != 0) {
      nsFlags = nsFlags | uiNSWindowStyleMaskClosable;
    }

    if ((flags & window_flags::minimizable) != 0) {
      nsFlags = nsFlags | uiNSWindowStyleMaskMiniaturizable;
    }

    if ((flags & window_flags::resizable) != 0) {
      nsFlags = nsFlags | uiNSWindowStyleMaskResizable;
    }

    if ((flags & window_flags::full_size_content_view) != 0) {
      nsFlags = nsFlags | uiNSWindowStyleMaskFullSizeContentView;
    }

    if (isPanel) {
      nsFlags = nsFlags | uiNSWindowStyleMaskUtilityWindow;
    }

    const char* windowClassName = isPanel ? "NSPanel" : "NSWindow";
    m_window = objc::create_object<void, CGRect, objc::ns_uint_t, objc::ns_uint_t, bool>(windowClassName,
        "initWithContentRect:styleMask:backing:defer:", CGRect{ { 0, 0 }, { 300, 300 } }, nsFlags, 2UL, true);
    //    m_window = create_objc_object_ref<void, CGRect, objc::ns_uint_t, objc::ns_uint_t, bool>(windowClassName,
    //        "initWithContentRect:styleMask:backing:defer:", CGRect{ { 0, 0 }, { 300, 300 } }, nsFlags, 2UL, YES);

    objc::icall(m_window, "setDelegate:", m_obj);
    objc::icall(m_window, "setContentView:", m_view->get_native_handle());
    objc::call<void, bool>(m_window, "setReleasedWhenClosed:", false);

    if ((flags & window_flags::full_size_content_view) != 0) {
      objc::call<void, bool>(m_window, "setTitlebarAppearsTransparent:", true);
    }

    objc::icall(m_window, "makeKeyAndOrderFront:");
    objc::call(m_window, "center");

    create_menu();
    //
  }

  ~window_object() {
    std::cout << "WindpwComponen :: ~Native" << std::endl;

    if (m_window) {

      objc::icall(m_window, "setDelegate:");
    }

    if (m_obj) {
      ClassObject::set_pointer(m_obj, nullptr);
      objc::reset(m_obj);
    }

    if (m_window) {
      objc::reset(m_window);
    }
  }

  native_window_handle get_native_handle() const { return reinterpret_cast<native_window_handle>(m_window); }

  void close() { objc::call(m_window, "close"); }

  void set_shadow(bool visible) { objc::call<void, bool>(m_window, "setHasShadow:", visible); }

  void set_title(std::string_view title) {
    CFStringRef str = CFStringCreateWithBytes(kCFAllocatorDefault, reinterpret_cast<const UInt8*>(title.data()),
        static_cast<CFIndex>(title.size()), kCFStringEncodingUTF8, false);

    objc::call<void, CFStringRef>(m_window, "setTitle:", str);

    CFRelease(str);
  }

  void set_frame(const nano::rect<int>& rect) {
    objc::call<void, CGRect, bool>(m_window, "setFrame:display:", rect.convert<CGRect>(), true);
  }

  nano::rect<int> get_frame() const { return objc::call<CGRect>(m_window, "frame"); }

  void center() { objc::call(m_window, "center"); }

  void set_background_color(const nano::color& c) {

    cf::unique_ptr<CGColorRef> color
        = CGColorCreateGenericRGB(c.red<CGFloat>(), c.green<CGFloat>(), c.blue<CGFloat>(), c.alpha<CGFloat>());

    objc::obj_t* nsColor = objc::call_meta<objc::obj_t*, CGColorRef>("NSColor", "colorWithCGColor:", color);
    objc::icall(m_window, "setBackgroundColor:", nsColor);
  }

  //    void set_background_color(nano::system_color scolor) { SetBackgroundColor(GetSystemColor(scolor)); }

  void set_flags(window_flags flags) {
    //        constexpr unsigned int uiNSWindowStyleMaskBorderless = 0;
    constexpr objc::ns_uint_t uiNSWindowStyleMaskTitled = 1 << 0;
    constexpr objc::ns_uint_t uiNSWindowStyleMaskClosable = 1 << 1;
    constexpr objc::ns_uint_t uiNSWindowStyleMaskMiniaturizable = 1 << 2;
    constexpr objc::ns_uint_t uiNSWindowStyleMaskResizable = 1 << 3;

    objc::ns_uint_t nsFlags = 0;

    if ((flags & window_flags::titled) != 0) {
      nsFlags = nsFlags | uiNSWindowStyleMaskTitled;
    }

    if ((flags & window_flags::closable) != 0) {
      nsFlags = nsFlags | uiNSWindowStyleMaskClosable;
    }

    if ((flags & window_flags::minimizable) != 0) {
      nsFlags = nsFlags | uiNSWindowStyleMaskMiniaturizable;
    }

    if ((flags & window_flags::resizable) != 0) {
      nsFlags = nsFlags | uiNSWindowStyleMaskResizable;
    }

    //        nsFlags = nsFlags | uiNSWindowStyleMaskUnifiedTitleAndToolbar;

    objc::call<void, objc::ns_uint_t>(m_window, "setStyleMask:", nsFlags);

    //        - (void)setDocumentEdited:(bool)dirtyFlag;
  }

  void set_document_edited(bool dirty) {
    objc::call<void, bool>(m_window, "setDocumentEdited:", static_cast<bool>(dirty));
  }

  void set_delegate(window_proxy::delegate* d) { m_window_delegate = d; }

  inline objc::obj_t* create_menu_item(const char* title, const char* keyEquivalent, long tag) {
    static objc::selector_t* initItemWithTitleSelector = objc::get_selector("initWithTitle:action:keyEquivalent:");
    static objc::selector_t* menuActionSelector = objc::get_selector("menuAction:");

    objc::obj_t* item = objc::create_class_instance("NSMenuItem");
    objc::call<void, CFStringRef, objc::selector_t*, CFStringRef>(item, initItemWithTitleSelector,
        cf::create_string(title), menuActionSelector, cf::create_string(keyEquivalent));

    objc::call<void, long>(item, "setTag:", tag);
    objc::icall(item, "setTarget:", m_obj);

    return item;
  }

  inline objc::obj_t* create_menu(const char* title) {

    return objc::create_object<void, CFStringRef>("NSMenu", "initWithTitle:", cf::create_string(title));
  }

  inline void add_menu_item(objc::obj_t* menu, objc::obj_t* item, bool release_item = false) {
    static objc::selector_t* addItemSelector = objc::get_selector("addItem:");
    objc::icall(menu, addItemSelector, item);

    if (release_item) {
      objc::release(item);
    }
  }

  inline void set_submenu(objc::obj_t* item, objc::obj_t* submenu, bool release_submenu = false) {
    static objc::selector_t* selector = objc::get_selector("setSubmenu:");
    objc::icall(item, selector, submenu);

    if (release_submenu) {
      objc::release(submenu);
    }
  }

  void create_menu() {

    objc::obj_t* mainMenu = create_menu("Title");

    {
      objc::obj_t* aboutMenu = create_menu("AppName");
      add_menu_item(aboutMenu, create_menu_item("About AppName", "", 1111), true);
      add_menu_item(aboutMenu, create_menu_item("Quit AppName", "q", 128932), true);

      objc::obj_t* aboutMenuItem = create_menu_item("UI", "", 0);
      set_submenu(aboutMenuItem, aboutMenu, true);
      add_menu_item(mainMenu, aboutMenuItem, true);
    }

    {
      objc::obj_t* fileMenu = create_menu("File");
      add_menu_item(fileMenu, create_menu_item("New", "", 232), true);

      objc::obj_t* fileMenuItem = create_menu_item("UI", "", 0);
      set_submenu(fileMenuItem, fileMenu, true);
      add_menu_item(mainMenu, fileMenuItem, true);
    }

    objc_object* sharedApplication = objc::get_class_property("NSApplication", "sharedApplication");
    objc::call<void, objc_object*>(sharedApplication, "setMainMenu:", mainMenu);
    objc::release(mainMenu);
  }

  nano::view* m_view;
  objc::obj_t* m_window = nullptr;
  objc::obj_t* m_obj;
  window_proxy::delegate* m_window_delegate = nullptr;

  void window_did_miniaturize([[maybe_unused]] objc_object* notification) {

    if (m_window_delegate) {
      m_window_delegate->window_did_miniaturize(m_view);
    }
    // if (m_comp->m_listeners.Size()) {
    // m_comp->m_listeners.Call(&Listener::window_did_miniaturize, m_comp);
    // }
  }

  void window_did_deminiaturize([[maybe_unused]] objc_object* notification) {
    std::cout << "window_did_deminiaturize" << std::endl;
    if (m_window_delegate) {
      m_window_delegate->window_did_deminiaturize(m_view);
    }
    // if (m_comp->m_listeners.Size()) {
    // m_comp->m_listeners.Call(&Listener::window_did_deminiaturize, m_comp);
    // }
  }

  void window_did_enter_full_screen([[maybe_unused]] objc_object* notification) {
    std::cout << "window_did_enter_full_screen" << std::endl;
    if (m_window_delegate) {
      m_window_delegate->window_did_enter_full_screen(m_view);
    }
    // if (m_comp->m_listeners.Size()) {
    // m_comp->m_listeners.Call(&Listener::window_did_enter_full_screen, m_comp);
    // }
  }

  void window_did_exit_full_screen([[maybe_unused]] objc_object* notification) {
    std::cout << "window_did_exit_full_screen" << std::endl;
    if (m_window_delegate) {
      m_window_delegate->window_did_exit_full_screen(m_view);
    }
  }

  void window_did_become_key([[maybe_unused]] objc_object* notification) {
    std::cout << "window_did_become_key" << std::endl;
    if (m_window_delegate) {
      m_window_delegate->window_did_become_key(m_view);
    }
  }

  void window_did_resign_key([[maybe_unused]] objc_object* notification) {
    std::cout << "window_did_resign_key" << std::endl;
    if (m_window_delegate) {
      m_window_delegate->window_did_resign_key(m_view);
    }
  }
  void window_did_change_screen([[maybe_unused]] objc_object* notification) {
    std::cout << "windowDidChangeScreen" << std::endl;
    if (m_window_delegate) {
      m_window_delegate->window_did_change_screen(m_view);
    }
  }

  void window_will_close([[maybe_unused]] objc_object* notification) {
    std::cout << "window_will_close" << std::endl;
    if (m_window_delegate) {
      m_window_delegate->window_will_close(m_view);
    }
  }

  bool window_should_close([[maybe_unused]] objc_object* sender) {
    std::cout << "dddd window_should_close" << std::endl;

    if (m_window_delegate) {
      return m_window_delegate->window_should_close(m_view);
    }

    return true;
  }

  void on_menu_action(objc::obj_t* sender) {
    long tag = objc::call<long>(sender, "tag");

    if (tag == 128932) {
      nano::application::quit();
      return;
    }
    //        NSLog(@"About %@ %@ %d", sender, [sender title], (int)[sender
    //        tag]);
    //        [[NSApplication sharedApplication] terminate:self];
    std::cout << "on_menu_action " << tag << std::endl;
  }

  void on_dealloc() { std::cout << "WindowComponent->on_dealloc" << std::endl; }

  class ClassObject : public objc::class_descriptor<window_object> {
  public:
    using ClassType = window_object;

    ClassObject()
        : class_descriptor("WindowComponentClassObject") {

      //            register_class();
      if (!add_protocol("NSWindowDelegate", true)) {
        //                    std::cerr << "ERROR AddProtocol
        //                    NSApplicationDelegate" << std::endl;
        return;
      }

      add_method<Dealloc>("dealloc", "v@:");

      add_notification_method<&window_object::window_did_miniaturize>("windowDidMiniaturize:");
      add_notification_method<&window_object::window_did_deminiaturize>("windowDidDeminiaturize:");

      add_notification_method<&window_object::window_did_enter_full_screen>("windowDidEnterFullScreen:");
      add_notification_method<&window_object::window_did_exit_full_screen>("windowDidExitFullScreen:");

      add_notification_method<&window_object::window_did_become_key>("windowDidBecomeKey:");
      add_notification_method<&window_object::window_did_resign_key>("windowDidResignKey:");

      add_notification_method<&window_object::window_did_change_screen>("windowDidChangeScreen:");

      add_notification_method<&window_object::window_will_close>("windowWillClose:");

      add_method<window_should_close>("windowShouldClose:", "c@:");

      add_notification_method<&window_object::on_menu_action>("menuAction:");

      register_class();
    }

    static void Dealloc(objc::obj_t* self, objc::selector_t*) {
      std::cout << "WindowComponent->Dealloc" << std::endl;

      if (auto* p = get_pointer(self)) {
        p->on_dealloc();
      }

      send_superclass_message<void>(self, "dealloc");
    }

    static bool window_should_close(objc::obj_t* self, objc::selector_t*, objc::obj_t* sender) {
      std::cout << "window_should_close" << std::endl;
      if (auto* p = get_pointer(self)) {
        return p->window_should_close(sender);
      }

      return true;
    }

    static void set_pointer(objc::obj_t* self, ClassType* owner) {
      objc::set_ivar_pointer(self, ClassType::valueName, owner);
    }

    static ClassType* get_pointer(objc::obj_t* self) {
      return objc::get_ivar_pointer<ClassType*>(self, ClassType::valueName);
    }
  };

  static ClassObject classObject;
};

NANO_CLANG_DIAGNOSTIC_PUSH()
NANO_CLANG_DIAGNOSTIC(ignored, "-Wexit-time-destructors")
NANO_CLANG_DIAGNOSTIC(ignored, "-Wglobal-constructors")
window_object::ClassObject window_object::classObject{};
NANO_CLANG_DIAGNOSTIC_POP()

class view::pimpl {
public:
  static constexpr const char* className = "CrazyView";
  static constexpr const char* baseName = "NSView";
  static constexpr const char* valueName = "owner";

  void add_web_view(view* view) {
    // WKWebViewConfiguration* conf = [[WKWebViewConfiguration alloc] init];
    // m_web_view = [[WKWebView alloc] initWithFrame:(CGRect)get_view_rect().with_position({0, 0}).reduced({10, 10})
    // configuration:conf]; [conf release];

    //[(NSView*)handle addSubview:m_web_view];

    // NSURL* url = [NSURL URLWithString:(NSString*)CFSTR("https://www.google.com")];
    // NSURLRequest* req =  [NSURLRequest requestWithURL:url];
    //[m_web_view loadRequest:req];

    objc::obj_t* conf = objc::create_class_instance("WKWebViewConfiguration");
    objc::call<void>(conf, objc::get_selector("init"));

    objc::obj_t* web_view = objc::create_class_instance("WKWebView");
    objc::call<void, CGRect, objc::obj_t*>(web_view, objc::get_selector("initWithFrame:configuration:"),
        static_cast<CGRect>(nano::rect<float>(0, 0, 200, 200)), conf);

    objc::release(conf);

    objc::icall(view->get_native_handle(), "addSubview:", web_view);

    CFStringRef url_str
        = CFStringCreateWithCString(kCFAllocatorDefault, "https://www.google.com", kCFStringEncodingUTF8);
    CFURLRef url = CFURLCreateWithString(kCFAllocatorDefault, url_str, nullptr);

    objc::obj_t* request = objc::create_class_instance("NSURLRequest");
    objc::call<void, CFURLRef>(request, objc::get_selector("initWithURL:"), url);

    [[maybe_unused]] objc::obj_t* wk_navigation
        = objc::call<objc::obj_t*, objc::obj_t*>(web_view, objc::get_selector("loadRequest:"), request);

    // NSURL* url = [NSURL URLWithString:(NSString*)CFSTR("https://www.google.com")];
    // NSURLRequest* req =  [NSURLRequest requestWithURL:url];
    //[m_web_view loadRequest:req];

    //        id m_web_view = [[WKWebView alloc] initWithFrame:(CGRect)get_view_rect().with_position({0,
    //        0}).reduced({10, 10}) configuration:conf];
  }

  pimpl(view* view, const nano::rect<int>& rect)
      : m_view(view) {

    m_obj = classObject.create_instance();
    ClassObject::set_pointer(m_obj, this);

    objc::call<void, CGRect>(m_obj, "initWithFrame:", rect.convert<CGRect>());

    objc::call<void, bool>(m_obj, "setPostsFrameChangedNotifications:", TRUE);

    objc::call<void, objc_object*, objc_selector*, CFStringRef, objc_object*>( //
        objc::get_class_property("NSNotificationCenter", "defaultCenter"), //
        "addObserver:selector:name:object:", //
        m_obj, //
        objc::get_selector("frameChanged:"), //
        NSViewFrameDidChangeNotification, m_obj);
  }

  //
  ~pimpl() {

    std::cout << "native_view::~pimpl " << std::endl;

    //    std::cout << "native_view::~Native " << m_view->get_id() << std::endl;
    if (!m_obj) {
      return;
    }

    if (m_trackingArea) {
      objc::icall(m_obj, "removeTrackingArea:", m_trackingArea);
      objc::reset(m_trackingArea);
    }

    // objc::ns_uint_t count = CFGetRetainCount(m_obj);
    // std::cout << "native_view::~Native::release " << count << std::endl;
    ClassObject::set_pointer(m_obj, nullptr);
    objc::reset(m_obj);

    std::cout << "native_view::~Native-->Donw " << std::endl;
  }

  void init(view* parent) {
    m_parent = parent;

    objc::icall(parent->get_native_handle(), "addSubview:", m_obj);

    parent->m_pimpl->m_children.push_back(m_view);
    //    parent->on_did_add_subview(m_view);

    //    if (responder* d = parent) {
    parent->on_did_add_subview(m_view);
    //    }

    add_tracking_area(nano::uiNSTrackingMouseEnteredAndExited //
        | nano::uiNSTrackingActiveInKeyWindow //
        | nano::uiNSTrackingInVisibleRect);
  }

  void init(window_flags flags) {
    m_win = std::unique_ptr<window_object>(new window_object(m_view, flags));

    add_tracking_area(nano::uiNSTrackingMouseEnteredAndExited //
        | nano::uiNSTrackingActiveInKeyWindow //
        | nano::uiNSTrackingMouseMoved //
        | nano::uiNSTrackingInVisibleRect);
  }

  void init(native_view_handle parent) {
    objc::icall(parent, "addSubview:", m_obj);

    add_tracking_area(nano::uiNSTrackingMouseEnteredAndExited //
        | nano::uiNSTrackingActiveInKeyWindow //
        | nano::uiNSTrackingMouseMoved //
        | nano::uiNSTrackingInVisibleRect);
  }

  inline objc::obj_t* get_window() const { return objc::call<objc::obj_t*>(m_obj, "window"); }

  void add_tracking_area(objc::ns_uint_t opts) {
    m_trackingArea = objc::create_object<void, CGRect, objc::ns_uint_t, objc::obj_t*, objc::obj_t*>(
        "NSTrackingArea", "initWithRect:options:owner:userInfo:", CGRectZero, opts, m_obj, nullptr);

    objc::icall(m_obj, "addTrackingArea:", m_trackingArea);
  }

  void initialize() {}

  native_view_handle get_native_handle() const { return reinterpret_cast<native_view_handle>(m_obj); }

  void set_hidden(bool hidden) { objc::call<void, bool>(m_obj, "setHidden:", hidden); }

  bool is_hidden() const { return objc::call<bool>(m_obj, "isHidden"); }

  void set_frame(const nano::rect<int>& rect) { objc::call<void, CGRect>(m_obj, "setFrame:", rect.convert<CGRect>()); }

  void set_frame_position(const nano::point<int>& pos) {
    objc::call<void, CGPoint>(m_obj, "setFrameOrigin:", static_cast<CGPoint>(pos));
  }

  void set_frame_size(const nano::size<int>& size) {
    objc::call<void, CGSize>(m_obj, "setFrameSize:", static_cast<CGSize>(size));
  }

  //  void set_bounds(const nano::rect<int>& rect) {
  //    nano::call<void, CGRect>(m_obj, "setBounds:", rect.convert<CGRect>());
  //  }

  nano::rect<int> get_frame() const { return objc::call<CGRect>(m_obj, "frame"); }

  nano::point<int> get_window_position() const {
    if (objc::obj_t* window = get_window()) {
      return convert_to_view(nano::point<int>(0, 0), nullptr, true);
    }

    return get_frame().origin;
  }

  nano::point<int> get_screen_position() const {
    if (objc::obj_t* window = get_window()) {
      objc::obj_t* screen = objc::call<objc::obj_t*>(window, "screen");

      nano::point<int> screenPos = objc::call<CGPoint, CGPoint>(
          window, "convertPointToScreen:", static_cast<CGPoint>(convert_to_view(get_frame().position, nullptr, false)));

      return screenPos.with_y(static_cast<int>(objc::call<CGRect>(screen, "frame").size.height) - screenPos.y);
    }

    return get_frame().origin;
  }

  nano::rect<int> get_bounds() const { return nano::rect<int>(objc::call<CGRect>(m_obj, "bounds")); }

  nano::rect<int> get_visible_rect() const { return objc::call<CGRect>(m_obj, "visibleRect"); }

  nano::point<int> convert_from_view(const nano::point<int>& point, nano::view* view) const {
    return objc::call<CGPoint, CGPoint, objc::obj_t*>(m_obj, "convertPoint:fromView:", static_cast<CGPoint>(point),
        reinterpret_cast<objc::obj_t*>(view ? view->get_native_handle() : nullptr));
  }

  nano::point<int> convert_to_view(const nano::point<int>& point, nano::view* view, bool flip = true) const {
    if (view || !flip) {
      return objc::call<CGPoint, CGPoint, objc::obj_t*>(m_obj, "convertPoint:toView:", static_cast<CGPoint>(point),
          reinterpret_cast<objc::obj_t*>(view ? view->get_native_handle() : nullptr));
    }

    if (objc::obj_t* window = get_window()) {
      nano::rect<int> windowFrame = objc::call<CGRect>(window, "frame");
      nano::rect<int> frame = get_frame();
      frame.position = objc::call<CGPoint, CGPoint, objc::obj_t*>(
          m_obj, "convertPoint:toView:", static_cast<CGPoint>(frame.position), nullptr);
      return frame.with_y(windowFrame.height - frame.y).position;
    }

    return point;
  }

  bool is_dirty_rect(const nano::rect<int>& rect) const {
    return objc::call<bool, CGRect>(m_obj, "needsToDrawRect:", rect.convert<CGRect>());
  }

  void unfocus() {
    if (objc::obj_t* window = get_window()) {
      objc::call<bool, objc::obj_t*>(window, "makeFirstResponder:", nullptr);
    }
  }

  void focus() {
    if (objc::obj_t* window = get_window()) {
      objc::call<bool, objc::obj_t*>(window, "makeFirstResponder:", m_obj);
    }
  }

  bool is_focused() const {
    objc::obj_t* window = get_window();
    return window && objc::call<objc::obj_t*>(window, "firstResponder") == m_obj;
  }

  void redraw() { objc::call<void, bool>(m_obj, "setNeedsDisplay:", true); }

  void redraw(const nano::rect<int>& rect) {
    objc::call<void, CGRect>(m_obj, "setNeedsDisplayInRect:", rect.convert<CGRect>());
  }

  void on_resize([[maybe_unused]] objc::obj_t* evt) { m_view->on_frame_changed(); }

  inline event create_event(objc::obj_t* evt) { return event(reinterpret_cast<native_event_handle>(evt), m_view); }

#define NANO_IMPL_EVENT(MemberMethod, Method)                                                                          \
  void MemberMethod(objc::obj_t* e) { m_view->Method(create_event(e)); }

  NANO_IMPL_EVENT(on_mouse_down, on_mouse_down)
  NANO_IMPL_EVENT(on_mouse_up, on_mouse_up)
  NANO_IMPL_EVENT(on_mouse_dragged, on_mouse_dragged)
  NANO_IMPL_EVENT(on_right_mouse_down, on_right_mouse_down)
  NANO_IMPL_EVENT(on_right_mouse_up, on_right_mouse_up)
  NANO_IMPL_EVENT(on_right_mouse_dragged, on_right_mouse_dragged)
  NANO_IMPL_EVENT(on_other_mouse_down, on_other_mouse_down)
  NANO_IMPL_EVENT(on_other_mouse_up, on_other_mouse_up)
  NANO_IMPL_EVENT(on_other_mouse_dragged, on_other_mouse_dragged)
  NANO_IMPL_EVENT(on_mouse_entered, on_mouse_entered)
  NANO_IMPL_EVENT(on_mouse_exited, on_mouse_exited)
  NANO_IMPL_EVENT(on_scroll_wheel, on_scroll_wheel)
  NANO_IMPL_EVENT(on_key_down, on_key_down)
  NANO_IMPL_EVENT(on_key_up, on_key_up)
  NANO_IMPL_EVENT(on_key_flags_changed, on_key_flags_changed)
#undef NANO_IMPL_EVENT

  void on_mouse_moved(objc::obj_t* evt) {

    if (objc::obj_t* subview
        = objc::call<objc::obj_t*, CGPoint>(m_obj, "hitTest:", objc::call<CGPoint>(evt, "locationInWindow"))) {
      if (auto* p = classObject.get_pointer(subview)) {
        p->m_view->on_mouse_moved(event(reinterpret_cast<native_event_handle>(evt), p->m_view));
        return;
      }
    }

    m_view->on_mouse_moved(create_event(evt));
  }

  void on_will_remove_subview([[maybe_unused]] objc::obj_t* v) {
    std::cout << "native_view::Native->on_will_remove_subview" << std::endl;
  }

  void on_will_draw() { m_view->on_will_draw(); }

  void on_draw(nano::rect<float> rect) {
    objc::obj_t* nsContext = objc::get_class_property("NSGraphicsContext", "currentContext");
    nano::graphic_context gc(
        reinterpret_cast<nano::graphic_context::handle>(objc::call<CGContextRef>(nsContext, "CGContext")));
    m_view->on_draw(gc, rect);
  }

  void on_did_hide() { m_view->on_hide(); }

  void on_did_unhide() { m_view->on_show(); }

  void on_dealloc() { std::cout << "view::~pimpl->on_dealloc " << std::endl; }

  void on_update_tracking_areas() { classObject.send_superclass_message<void>(m_obj, "updateTrackingAreas"); }

  bool become_first_responder() {
    m_view->on_focus();
    return true;
  }

  bool resign_first_responder() {
    m_view->on_unfocus();
    return true;
  }

  bool is_flipped() { return true; }

  //
  view* m_view;
  objc::obj_t* m_obj;
  objc::obj_t* m_trackingArea = nullptr;
  std::unique_ptr<window_object> m_win;
  view* m_parent = nullptr;
  std::vector<view*> m_children;

private:
  class ClassObject : public objc::class_descriptor<pimpl> {
  public:
    using ClassType = pimpl;

    ClassObject()
        : class_descriptor("UIViewClassObject") {

      add_method<dealloc>("dealloc", "v@:");

      //      add_method<method_ptr<bool>>(
      //          "isFlipped", [](id, objc::selector_t*) -> bool { return true; }, "c@:");

      add_method<&ClassType::is_flipped>("isFlipped", "c@:");

      add_method<&ClassType::become_first_responder>("becomeFirstResponder", "c@:");
      add_method<&ClassType::resign_first_responder>("resignFirstResponder", "c@:");

      add_notification_method<&ClassType::on_mouse_down>("mouseDown:");
      add_notification_method<&ClassType::on_mouse_up>("mouseUp:");
      add_notification_method<&ClassType::on_mouse_dragged>("mouseDragged:");

      add_notification_method<&ClassType::on_right_mouse_down>("rightMouseDown:");
      add_notification_method<&ClassType::on_right_mouse_up>("rightMouseUp:");
      add_notification_method<&ClassType::on_right_mouse_dragged>("rightMouseDragged:");

      add_notification_method<&ClassType::on_other_mouse_down>("otherMouseDown:");
      add_notification_method<&ClassType::on_other_mouse_up>("otherMouseUp:");
      add_notification_method<&ClassType::on_other_mouse_dragged>("otherMouseDragged:");

      add_notification_method<&ClassType::on_mouse_moved>("mouseMoved:");
      add_notification_method<&ClassType::on_mouse_entered>("mouseEntered:");
      add_notification_method<&ClassType::on_mouse_exited>("mouseExited:");

      add_notification_method<&ClassType::on_scroll_wheel>("scrollWheel:");

      add_notification_method<&ClassType::on_key_down>("keyDown:");
      add_notification_method<&ClassType::on_key_up>("keyUp:");
      add_notification_method<&ClassType::on_key_flags_changed>("flagsChanged:");

      add_notification_method<&ClassType::on_will_remove_subview>("willRemoveSubview:");

      add_notification_method<&ClassType::on_resize>("frameChanged:");

      add_method<view_will_draw>("viewWillDraw", "v@:");

      add_method<&ClassType::on_did_hide>("viewDidHide", "v@:");
      add_method<&ClassType::on_did_unhide>("viewDidUnhide", "v@:");
      add_method<&ClassType::on_update_tracking_areas>("updateTrackingAreas", "v@:");

      if (!add_method<draw_rect>("drawRect:", "v@:{CGRect={CGPoint=dd}{CGSize=dd}}")) {
        std::cout << "ERROR" << std::endl;
        return;
      }

      //      if (!add_pointer<ClassType>(ClassType::valueName, ClassType::className)) {
      //        std::cout << "ERROR" << std::endl;
      //        return;
      //      }

      register_class();
    }

    static void draw_rect(objc::obj_t* self, objc::selector_t*, CGRect r) {
      if (auto* p = get_pointer(self)) {
        p->on_draw(r);
      }
    }

    static void view_will_draw(objc::obj_t* self, objc::selector_t*) {
      if (auto* p = get_pointer(self)) {
        p->on_will_draw();
      }

      send_superclass_message<void>(self, "viewWillDraw");
    }

    static void dealloc(objc::obj_t* self, objc::selector_t*) {
      std::cout << "view..dealloc " << self << std::endl;

      if (auto* p = get_pointer(self)) {
        p->on_dealloc();
      }

      send_superclass_message<void>(self, "dealloc");
    }

    static void set_pointer(objc::obj_t* self, ClassType* owner) {
      objc::set_ivar_pointer(self, ClassType::valueName, owner);
    }

    static ClassType* get_pointer(objc::obj_t* self) {
      return objc::get_ivar_pointer<ClassType*>(self, ClassType::valueName);
    }
  };

  static ClassObject classObject;
};

NANO_CLANG_DIAGNOSTIC_PUSH()
NANO_CLANG_DIAGNOSTIC(ignored, "-Wexit-time-destructors")
NANO_CLANG_DIAGNOSTIC(ignored, "-Wglobal-constructors")
view::pimpl::ClassObject view::pimpl::classObject{};
NANO_CLANG_DIAGNOSTIC_POP()

view::view(window_flags flags) {
  m_pimpl = std::unique_ptr<pimpl>(new pimpl(this, { 0, 0, 50, 50 }));
  m_pimpl->init(flags);
}

view::view(view* parent, const nano::rect<int>& rect) {
  m_pimpl = std::unique_ptr<pimpl>(new pimpl(this, rect));
  m_pimpl->init(parent);
}

view::view(native_view_handle parent, const nano::rect<int>& rect, view_flags flags) {
  m_pimpl = std::unique_ptr<pimpl>(new pimpl(this, rect));
  m_pimpl->init(parent);

  if (nano::has_flag(view_flags::auto_resize, flags)) {
    constexpr objc::ns_uint_t uiNSViewWidthSizable = 2;
    constexpr objc::ns_uint_t uiNSViewHeightSizable = 16;
    objc::call<void, objc::ns_uint_t>(
        m_pimpl->m_obj, "setAutoresizingMask:", uiNSViewWidthSizable | uiNSViewHeightSizable);
  }
}

view::~view() {
  auto& children = m_pimpl->m_children;

  if (!children.empty()) {
    NANO_ERROR("WRONG");
  }

  // TODO: Check this.
  // The view is also released; if you plan to reuse it, be sure to retain it
  // before sending this message and to release it as appropriate when adding
  // it as a subview of another NSView. Calling this method removes any
  // constraints that refer to the view you are removing, or that refer to any
  // view in the subtree of the view you are removing.
  if (m_pimpl->m_parent) {

    auto& vec = m_pimpl->m_parent->m_pimpl->m_children;

    auto it = std::find(vec.begin(), vec.end(), this);
    if (it == vec.end()) {
      NANO_ERROR("WRONG");
    }
    else {
      vec.erase(it);
    }

    objc::call(get_native_handle(), "removeFromSuperview");

    //    if (responder* d = m_pimpl->m_parent->m_pimpl->m_responder) {
    m_pimpl->m_parent->on_did_remove_subview(this);
    //    }
  }
  else {
    objc::call(get_native_handle(), "removeFromSuperview");
  }
}

void view::initialize() { m_pimpl->initialize(); }

void view::set_auto_resize() {

  constexpr objc::ns_uint_t uiNSViewWidthSizable = 2;
  constexpr objc::ns_uint_t uiNSViewHeightSizable = 16;
  objc::call<void, objc::ns_uint_t>(
      m_pimpl->m_obj, "setAutoresizingMask:", uiNSViewWidthSizable | uiNSViewHeightSizable);
}

native_view_handle view::get_native_handle() const { return m_pimpl->get_native_handle(); }

bool view::is_window() const { return m_pimpl->m_win != nullptr; }

void window_proxy::set_window_frame(const nano::rect<int>& rect) {
  NANO_ASSERT(m_view.is_window(), "Not a window");

  if (m_view.is_window()) {
    m_view.m_pimpl->m_win->set_frame(rect);
  }
}

native_window_handle window_proxy::get_native_handle() const {
  return m_view.is_window() ? m_view.m_pimpl->m_win->get_native_handle() : nullptr;
}

void window_proxy::set_title(std::string_view title) {
  NANO_ASSERT(m_view.is_window(), "Not a window");
  if (m_view.is_window()) {
    m_view.m_pimpl->m_win->set_title(title);
  }
}

nano::rect<int> window_proxy::get_frame() const {
  NANO_ASSERT(m_view.is_window(), "Not a window");
  return m_view.is_window() ? m_view.m_pimpl->m_win->get_frame() : nano::rect<int>(0, 0, 0, 0);
}

void window_proxy::set_flags(window_flags flags) {
  NANO_ASSERT(m_view.is_window(), "Not a window");
  if (m_view.is_window()) {
    m_view.m_pimpl->m_win->set_flags(flags);
  }
}

void window_proxy::set_document_edited(bool dirty) {
  NANO_ASSERT(m_view.is_window(), "Not a window");
  if (m_view.is_window()) {
    m_view.m_pimpl->m_win->set_document_edited(dirty);
  }
}

void window_proxy::close() {
  NANO_ASSERT(m_view.is_window(), "Not a window");
  if (m_view.is_window()) {
    m_view.m_pimpl->m_win->close();
  }
}

void window_proxy::center() {
  NANO_ASSERT(m_view.is_window(), "Not a window");
  if (m_view.is_window()) {
    m_view.m_pimpl->m_win->center();
  }
}

void window_proxy::set_shadow(bool visible) {
  NANO_ASSERT(m_view.is_window(), "Not a window");
  if (m_view.is_window()) {
    m_view.m_pimpl->m_win->set_shadow(visible);
  }
}

void window_proxy::set_window_delegate(delegate* d) {
  NANO_ASSERT(m_view.is_window(), "Not a window");
  if (m_view.is_window()) {
    m_view.m_pimpl->m_win->set_delegate(d);
  }
}

nano::rect<int> view::get_frame() const { return m_pimpl->get_frame(); }

nano::point<int> view::get_position_in_window() const { return m_pimpl->get_window_position(); }

nano::point<int> view::get_position_in_screen() const { return m_pimpl->get_screen_position(); }

void view::set_frame(const nano::rect<int>& rect) { m_pimpl->set_frame(rect); }

void view::set_frame_position(const nano::point<int>& pos) { m_pimpl->set_frame_position(pos); }

void view::set_frame_size(const nano::size<int>& size) { m_pimpl->set_frame_size(size); }

nano::rect<int> view::get_bounds() const { return m_pimpl->get_bounds(); }

nano::rect<int> view::get_visible_rect() const { return m_pimpl->get_visible_rect(); }

nano::point<int> view::convert_from_view(const nano::point<int>& point, nano::view* view) const {
  return m_pimpl->convert_from_view(point, view);
}

nano::point<int> view::convert_to_view(const nano::point<int>& point, nano::view* view) const {
  return m_pimpl->convert_to_view(point, view, false);
}

void view::set_hidden(bool hidden) { m_pimpl->set_hidden(hidden); }

bool view::is_hidden() const { return m_pimpl->is_hidden(); }

void view::focus() { m_pimpl->focus(); }

void view::unfocus() { m_pimpl->unfocus(); }

bool view::is_focused() const { return m_pimpl->is_focused(); }

bool view::is_dirty_rect(const nano::rect<int>& rect) const { return m_pimpl->is_dirty_rect(rect); }

void view::redraw() { m_pimpl->redraw(); }

void view::redraw(const nano::rect<int>& rect) { m_pimpl->redraw(rect); }

view* view::get_parent() const { return m_pimpl->m_parent; }

nano::rect<int> get_native_view_bounds(nano::native_view_handle native_view) {
  return nano::rect<int>(objc::call<CGRect>(reinterpret_cast<objc::obj_t*>(native_view), "bounds"));
}

//
//
//
//
//

window::window(window_flags flags)
    : view(flags)
    , window_proxy(get_window_proxy()) {}

window::~window() {}

//
//
//
//
//
class webview::native {
public:
  native(webview* wview)
      : m_view(wview) {}

  ~native() {
    if (m_web_view) {
      objc::release(m_web_view);
      m_web_view = nullptr;
    }
  }

  void load(const std::string& path) {
    CFStringRef url_str = CFStringCreateWithCString(kCFAllocatorDefault, path.c_str(), kCFStringEncodingUTF8);
    CFURLRef url = CFURLCreateWithString(kCFAllocatorDefault, url_str, nullptr);
    CFRelease(url_str);

    //    id request = class_createInstance(objc_getClass("NSURLRequest"), 0);
    //    nano::call<void, CFURLRef>(request, nano::get_selector("initWithURL:"), url);

    objc::obj_t* request = objc::create_object<void, CFURLRef>("NSURLRequest", "initWithURL:", url);
    CFRelease(url);

    [[maybe_unused]] objc::obj_t* wk_navigation
        = objc::call<objc::obj_t*, objc::obj_t*>(m_web_view, objc::get_selector("loadRequest:"), request);

    objc::release(request);
  }

  void create_web_view(const nano::rect<int>& rect) {

    if (m_web_view) {
      return;
    }

    objc::obj_t* conf = objc::create_class_instance("WKWebViewConfiguration");
    objc::call<void>(conf, objc::get_selector("init"));

    m_web_view = objc::create_object<void, CGRect, objc::obj_t*>(
        "WKWebView", "initWithFrame:configuration:", static_cast<CGRect>(rect), conf);

    //    m_web_view = class_createInstance(objc_getClass("WKWebView"), 0);
    //    nano::call<void, CGRect, id>(
    //        m_web_view, nano::get_selector("initWithFrame:configuration:"), static_cast<CGRect>(rect), conf);

    objc::release(conf);

    objc::icall(m_view->get_native_handle(), "addSubview:", m_web_view);

    load("https://www.facebook.com");
  }

  webview* m_view;
  objc::obj_t* m_web_view = nullptr;
};

webview::webview(view* parent, const nano::rect<int>& rect)
    : view(parent, rect) {
  m_native = new native(this);
  m_native->create_web_view(rect.with_position({ 0, 0 }));
}

webview::~webview() {
  if (m_native) {
    delete m_native;
    m_native = nullptr;
  }
}

void webview::set_wframe(const nano::rect<int>& rect) {
  objc::call<void, CGRect>(m_native->m_web_view, "setFrame:", static_cast<CGRect>(rect));
}

void webview::load(const std::string& path) { m_native->load(path); }

//
//
//
class application::native {
public:
  static constexpr const char* className = "UIApplicationNativeDelegate";
  static constexpr const char* baseName = "NSObject";
  static constexpr const char* valueName = "owner";

  native(application* app)
      : m_app(app) {
    m_obj = classObject.create_instance();
    objc::call(m_obj, "init");
    app_delegate_class_object::set_pointer(m_obj, this);

    objc::obj_t* sharedApplication = objc::get_class_property("NSApplication", "sharedApplication");
    objc::call<void, objc::obj_t*>(sharedApplication, "setDelegate:", m_obj);
  }

  ~native() {
    if (m_obj) {
      objc::obj_t* sharedApplication = objc::get_class_property("NSApplication", "sharedApplication");
      objc::call<void, objc::obj_t*>(sharedApplication, "setDelegate:", nullptr);
      objc::reset(m_obj);
    }
  }

  void application_did_finish_launching(objc_object*) {
    std::cout << "application_did_finish_launching" << std::endl;
    m_app->initialise();
  }

  void application_did_become_active(objc_object*) {
    std::cout << "application_did_become_active" << std::endl;
    m_app->resumed();
  }

  void application_did_resign_active(objc_object*) {
    std::cout << "application_did_resign_active" << std::endl;
    m_app->suspended();
  }

  void application_will_terminate(objc_object*) {
    std::cout << "application_will_terminate" << std::endl;
    m_app->shutdown();
  }

  //  bool application_should_terminate(objc_object*) {
  //    std::cout << "application_should_terminate" << std::endl;
  //    return m_app->should_terminate();
  //  }

  bool application_should_terminate_after_last_window_closed([[maybe_unused]] objc::obj_t* notification) {
    std::cout << "application_should_terminate_after_last_window_closed" << std::endl;
    return true;
  }

  objc::ns_uint_t application_should_terminate([[maybe_unused]] objc::obj_t* notification) {

    if (m_app->should_terminate()) {
      // NSTerminateNow = 1
      return 1;
    }

    // NSTerminateCancel = 0
    return 0;
  }

  //    static bool application_should_terminate_after_last_window_closed(id self, objc::selector_t*, id notification) {
  //      if (auto* p = get_pointer(self)) {
  //        return p->application_should_terminate_after_last_window_closed(notification);
  //      }
  //
  //      return TRUE;
  //    }

  application* m_app;
  objc::obj_t* m_obj;
  std::vector<std::string> m_args;

private:
  class app_delegate_class_object : public objc::class_descriptor<native> {
  public:
    using ClassType = native;

    app_delegate_class_object()
        : class_descriptor("app_delegate_class_object") {
      if (!add_protocol("NSApplicationDelegate", true)) {
        std::cerr << "ERROR AddProtocol NSApplicationDelegate" << std::endl;
        return;
      }

      add_notification_method<&ClassType::application_did_finish_launching>("applicationDidFinishLaunching:");
      add_notification_method<&ClassType::application_did_become_active>("applicationDidBecomeActive:");
      add_notification_method<&ClassType::application_did_resign_active>("applicationDidResignActive:");
      add_notification_method<&ClassType::application_will_terminate>("applicationWillTerminate:");

      if (!add_method<&ClassType::application_should_terminate>("applicationShouldTerminate:", "L@:@")) {
        std::cerr << "ERROR add_method applicationShouldTerminate" << std::endl;
        return;
      }

      if (!add_method<&ClassType::application_should_terminate_after_last_window_closed>(
              "applicationShouldTerminateAfterLastWindowClosed:", "c@:@")) {
        std::cerr << "ERROR add_method applicationShouldTerminateAfterLastWindowClosed" << std::endl;
        return;
      }

      //      if (!add_pointer<ClassType>(ClassType::valueName, ClassType::className)) {
      //        std::cerr << "ERROR add_pointer app_delegate_class_object" << std::endl;
      //        return;
      //      }

      register_class();
    }

    static void set_pointer(objc::obj_t* self, ClassType* owner) {
      objc::set_ivar_pointer(self, ClassType::valueName, owner);
    }

    static ClassType* get_pointer(objc::obj_t* self) {
      return objc::get_ivar_pointer<ClassType*>(self, ClassType::valueName);
    }

    //    static objc::ns_uint_t application_should_terminate(id self, objc::selector_t*, id notification) {
    //      if (auto* p = get_pointer(self)) {
    //        if (p->application_should_terminate(notification)) {
    //          // NSTerminateNow = 1
    //          return 1;
    //        }
    //        else {
    //          // NSTerminateCancel = 0
    //          return 0;
    //        }
    //      }
    //
    //      return 1;
    //    }

    //    static bool application_should_terminate_after_last_window_closed(id self, objc::selector_t*, id notification)
    //    {
    //      if (auto* p = get_pointer(self)) {
    //        return p->application_should_terminate_after_last_window_closed(notification);
    //      }
    //
    //      return TRUE;
    //    }
  };

  static app_delegate_class_object classObject;
};

NANO_CLANG_DIAGNOSTIC_PUSH()
NANO_CLANG_DIAGNOSTIC(ignored, "-Wexit-time-destructors")
NANO_CLANG_DIAGNOSTIC(ignored, "-Wglobal-constructors")
application::native::app_delegate_class_object application::native::classObject{};
NANO_CLANG_DIAGNOSTIC_POP()

application::application() { m_native = std::unique_ptr<native>(new native(this)); }

application::~application() {}

void application::quit() {
  objc_object* sharedApplication = objc::get_class_property("NSApplication", "sharedApplication");
  objc::call<void, objc_object*>(sharedApplication, "terminate:", nullptr);
}

std::string application::get_command_line_arguments() const {
  const std::vector<std::string>& args = m_native->m_args;

  std::string str;
  str.reserve(args.size() * 8);

  for (std::size_t i = 0; i < args.size(); i++) {
    str.append(args[i]);
    str.push_back(' ');
  }

  str.pop_back();

  return str;
}

std::vector<std::string> application::get_command_line_arguments_array() const { return m_native->m_args; }

void application::initialize_application(application* app, int argc, const char* argv[]) {
  std::vector<std::string> args;
  args.resize(static_cast<std::size_t>(argc));

  for (std::size_t i = 0; i < args.size(); i++) {
    args[i] = argv[i];
  }

  app->m_native->m_args = std::move(args);
  app->prepare();
}
} // namespace nano

extern "C" {
extern int NSApplicationMain(int argc, const char* argv[]);
}

namespace nano {
int application::run() {
  std::vector<const char*> args;
  args.resize(m_native->m_args.size());

  for (std::size_t i = 0; i < args.size(); i++) {
    args[i] = m_native->m_args[i].c_str();
  }

  return NSApplicationMain(static_cast<int>(m_native->m_args.size()), args.data());
}

message::~message() {}

struct async_main_thread_call {
  static inline std::vector<std::shared_ptr<message>>& get_messages() {
    NANO_CLANG_PUSH_WARNING("-Wexit-time-destructors")
    static std::vector<std::shared_ptr<message>> messages;
    NANO_CLANG_POP_WARNING()
    return messages;
  }

  static inline void add_message(std::shared_ptr<message> msg) {
    std::vector<std::shared_ptr<message>>& messages = get_messages();

    if (std::find(messages.begin(), messages.end(), msg) != messages.end()) {
      return;
    }

    messages.push_back(msg);
  }

  static inline void remove_message(message* msg) {
    std::vector<std::shared_ptr<message>>& messages = get_messages();

    auto it
        = std::find_if(messages.begin(), messages.end(), [msg](std::shared_ptr<message> m) { return m.get() == msg; });

    if (it == messages.end()) {
      return;
    }

    messages.erase(it);
  }
};

void post_message(std::shared_ptr<message> msg) {

  message* m = msg.get();
  async_main_thread_call::add_message(msg);

  dispatch_async(dispatch_get_main_queue(), ^{
      m->call();
      async_main_thread_call::remove_message(m);
  });

  //    void dispatch_async_f(dispatch_queue_t queue, void *context, dispatch_function_t work);
}

} // namespace nano.
NANO_CLANG_DIAGNOSTIC_POP()
