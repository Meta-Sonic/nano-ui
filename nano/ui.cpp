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

#include "nano/ui.h"
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#include <objc/message.h>
#include <objc/objc.h>
#include <objc/runtime.h>
#include <dispatch/dispatch.h>

#include <random>

extern "C" {
extern CFStringRef NSViewFrameDidChangeNotification;
}

NANO_CLANG_DIAGNOSTIC_PUSH()
NANO_CLANG_DIAGNOSTIC(warning, "-Weverything")
NANO_CLANG_DIAGNOSTIC(ignored, "-Wc++98-compat")
NANO_CLANG_DIAGNOSTIC(ignored, "-Wc++98-compat-bind-to-temporary-copy")

namespace nano {

template <typename ReturnType>
inline ReturnType return_default_value() {
  return ReturnType{};
}

template <>
inline void return_default_value<void>() {}

///
///
///

NANO_INLINE_CXPR unsigned long uiNSTrackingMouseEnteredAndExited = 0x01;
NANO_INLINE_CXPR unsigned long uiNSTrackingActiveInKeyWindow = 0x20;
NANO_INLINE_CXPR unsigned long uiNSTrackingMouseMoved = 0x02;
NANO_INLINE_CXPR unsigned long uiNSTrackingInVisibleRect = 0x200;

namespace detail {
  template <typename _CFType>
  struct cf_object_deleter {
    inline void operator()(std::add_pointer_t<std::remove_pointer_t<_CFType>> obj) const noexcept {
      if (obj) {
        CFRelease(obj);
      }
    }
  };

} // namespace detail.

/// A unique_ptr for CFTypes.
/// The deleter will call CFRelease().
/// @remarks The _CFType can either be a CFTypeRef of the underlying type (e.g.
/// CFStringRef or __CFString).
///          In other words, as opposed to std::unique_ptr<>, _CFType can also
///          be a pointer.
template <class _CFType>
struct cf_unique_ptr : std::unique_ptr<std::remove_pointer_t<_CFType>, detail::cf_object_deleter<_CFType>> {
  using std::unique_ptr<std::remove_pointer_t<_CFType>, detail::cf_object_deleter<_CFType>>::unique_ptr;
};

inline cf_unique_ptr<CFStringRef> create_cf_string_ptr(const char* str) {
  return cf_unique_ptr<CFStringRef>(CFStringCreateWithCString(kCFAllocatorDefault, str, kCFStringEncodingUTF8));
}

inline cf_unique_ptr<CFStringRef> create_cf_string_ptr(std::string_view str) {
  return cf_unique_ptr<CFStringRef>(CFStringCreateWithBytes(kCFAllocatorDefault,
      reinterpret_cast<const UInt8*>(str.data()), static_cast<CFIndex>(str.size()), kCFStringEncodingUTF8, false));
}

template <std::size_t N>
inline CFDictionaryRef create_cf_dictionary(CFStringRef const (&keys)[N], CFTypeRef const (&values)[N]) {
  return CFDictionaryCreate(kCFAllocatorDefault, reinterpret_cast<const void**>(&keys),
      reinterpret_cast<const void**>(&values), N, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
}

inline nano::cf_unique_ptr<CGColorRef> to_cg_color_ref(const nano::color& color) {
  return nano::cf_unique_ptr<CGColorRef>(CGColorCreateGenericRGB(
      color.red<CGFloat>(), color.green<CGFloat>(), color.blue<CGFloat>(), color.alpha<CGFloat>()));
}

namespace detail {
  struct obj_c_object_deleter {
    inline void operator()(id obj) const noexcept;
  };
} // namespace detail

struct objc_unique_ptr : std::unique_ptr<objc_object, detail::obj_c_object_deleter> {
  using std::unique_ptr<objc_object, detail::obj_c_object_deleter>::unique_ptr;
};

template <typename R = void, typename... Args>
using method_ptr = R (*)(id, SEL, Args...);

template <typename R = void, typename... Args>
using class_method_ptr = R (*)(Class, SEL, Args...);

template <typename R = void, typename... Args>
using super_method_ptr = R (*)(struct objc_super*, SEL, Args...);

///
// template <typename R = void, typename... Args, typename... Params>
// inline R call(id obj, SEL selector, Params&&... params) {
//   IMP fctImpl = class_getMethodImplementation(object_getClass(obj), selector);
//   return reinterpret_cast<method_ptr<R, Args...>>(fctImpl)(obj, selector, std::forward<Params>(params)...);
// }

template <typename R = void, typename... Args, typename... Params, typename SelectorType, typename IdType>
inline R call(IdType* obj_ptr, SelectorType selector, Params&&... params) {

  static_assert(std::is_same_v<SelectorType, SEL> || std::is_constructible_v<std::string_view, SelectorType>, "");

  id obj = reinterpret_cast<id>(obj_ptr);
  SEL sel = [](SelectorType s) {
    if constexpr (std::is_same_v<SelectorType, SEL>) {
      return s;
    }
    else {
      return sel_registerName(s);
    }
  }(std::forward<SelectorType>(selector));

  IMP fctImpl = class_getMethodImplementation(object_getClass(obj), sel);
  return reinterpret_cast<method_ptr<R, Args...>>(fctImpl)(obj, sel, std::forward<Params>(params)...);
}

template <typename IdType, typename... ObjType, typename SelectorType>
inline void icall(IdType* obj_ptr, SelectorType selector, ObjType... obj_type_ptr) {
  static_assert(sizeof...(ObjType) < 2, "obj_type_ptr must either be an objc id or nullptr");
  if constexpr (sizeof...(ObjType)) {
    return call<void, id>(reinterpret_cast<id>(obj_ptr), selector, reinterpret_cast<id>(obj_type_ptr)...);
  }
  else {
    return call<void, id>(reinterpret_cast<id>(obj_ptr), selector, nullptr);
  }
}

template <typename R = void, typename... Args, typename... Params>
inline R call_meta(const char* className, const char* selectorName, Params&&... params) {
  Class objClass = objc_getClass(className);
  Class meta = objc_getMetaClass(className);
  SEL selector = sel_registerName(selectorName);
  IMP fctImpl = class_getMethodImplementation(meta, selector);
  return reinterpret_cast<class_method_ptr<R, Args...>>(fctImpl)(objClass, selector, std::forward<Params>(params)...);
}

inline id get_class_property(const char* className, const char* propertyName) {
  return call_meta<id>(className, propertyName);
}

inline SEL get_selector(const char* name) { return sel_registerName(name); }

template <typename Type>
void set_ivar(id obj, const char* name, Type* value) {
  object_setInstanceVariable(obj, name, value);
}

template <typename Type>
inline Type get_ivar(id self, const char* name) {
  void* v = nullptr;
  object_getInstanceVariable(self, name, &v);
  return static_cast<Type>(v);
}

inline void objc_release(id obj) { nano::call(obj, "release"); }
inline void objc_reset(id& obj) {
  nano::call(obj, "release");
  obj = nullptr;
}

namespace detail {

  void obj_c_object_deleter::operator()(id obj) const noexcept { nano::objc_release(obj); }

} // namespace detail.

inline std::string generate_random_alphanum_string(std::size_t length) {
  static const char alphanum[] = "0123456789"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                 "abcdefghijklmnopqrstuvwxyz";

  static const int alphanumSize = sizeof(alphanum);

  std::string str;
  str.resize(length, 0);

  std::mt19937 rng;
  std::uniform_int_distribution<int> dist(0, alphanumSize - 1);

  for (std::size_t i = 0; i < str.size(); i++) {
    str[i] = alphanum[dist(rng)];
  }

  return str;
}

NANO_CLANG_PUSH_WARNING("-Wold-style-cast")
template <typename Descriptor>
class objc_class {
public:
  objc_class(const char* rootName)
      : m_classObject(objc_allocateClassPair(
          objc_getClass(Descriptor::baseName), (rootName + generate_random_alphanum_string(10)).c_str(), 0)) {

    if (!add_pointer<Descriptor>(Descriptor::valueName, Descriptor::className)) {
      std::cout << "ERROR" << std::endl;
      return;
    }

    register_class();
  }

  objc_class(const objc_class&) = delete;

  ~objc_class() {
    std::string kvoSubclassName = std::string("NSKVONotifying_") + class_getName(m_classObject);

    if (objc_getClass(kvoSubclassName.c_str()) == nullptr) {
      objc_disposeClassPair(m_classObject);
    }
  }

  objc_class& operator=(const objc_class&) = delete;

  ///
  void register_class() { objc_registerClassPair(m_classObject); }

  ///
  id create_instance() const { return class_createInstance(m_classObject, 0); }

  template <typename ReturnType, typename... Args, typename... Params>
  static ReturnType send_superclass_message(id self, const char* selectorName, Params&&... params) {

    if (Class objClass = objc_getClass(Descriptor::baseName)) {
      using FctType = super_method_ptr<ReturnType, Args...>;
      objc_super s = { self, objClass };
      return reinterpret_cast<FctType>(objc_msgSendSuper)(
          &s, sel_registerName(selectorName), std::forward<Params>(params)...);
    }

    NANO_ERROR("Could not create objc class");
    return return_default_value<ReturnType>();
  }

  template <typename Type>
  inline bool add_pointer(const char* name, const char* className) {
    std::string enc = "^{" + std::string(className) + "=}";
    return class_addIvar(m_classObject, name, sizeof(Type*), alignof(Type*), enc.c_str());
  }

  //  template <typename FunctionType>
  //    inline bool add_method(const char* selectorName, FunctionType callbackFn, const char* signature) {
  //    SEL selector = sel_registerName(selectorName);
  //    return class_addMethod(m_classObject, selector, (IMP)callbackFn, signature);
  //  }

  //        template <typename ReturnType, ReturnType (Descriptor::*MemberFunctionPointer)()>
  //        inline bool add_method( const char* selectorName, const char* signature) {
  //          return class_addMethod(
  //              m_classObject, sel_registerName(selectorName),
  //              (IMP)(method_ptr<ReturnType>)[ ](id self, SEL) {
  //                if (auto* p = nano::get_ivar<Descriptor*>(self, Descriptor::valueName)) {
  //                  return (p->*MemberFunctionPointer)();
  //                }
  //
  //                  return return_default_value<ReturnType>();
  //              },
  //              signature);
  //        }

  template <auto FunctionType>
  inline bool add_method(const char* selectorName, const char* signature) {
    SEL selector = sel_registerName(selectorName);

    //        if constexpr(std::is_invocable_v<decltype(FunctionType), Descriptor*> &&
    //        std::is_member_function_pointer_v<decltype(FunctionType)>) {
    //            return add_member_method_impl<FunctionType>(FunctionType, selector, signature);
    //        }
    if constexpr (std::is_member_function_pointer_v<decltype(FunctionType)>) {
      return add_member_method_impl<FunctionType>(FunctionType, selector, signature);
    }
    else {
      return class_addMethod(m_classObject, selector, (IMP)FunctionType, signature);
    }
  }

  template <void (Descriptor::*MemberFunctionPointer)(id)>
  bool add_notification_method(const char* selectorName) {
    return class_addMethod(
        m_classObject, sel_registerName(selectorName),
        (IMP)(method_ptr<void, id>)[](id self, SEL, id notification) {
          if (auto* p = nano::get_ivar<Descriptor*>(self, Descriptor::valueName)) {
            (p->*MemberFunctionPointer)(notification);
          }
        },
        "v@:@");
  }

  inline bool add_protocol(const char* protocolName, bool force = false) {

    if (Protocol* protocol = objc_getProtocol(protocolName)) {
      return class_addProtocol(m_classObject, protocol);
    }

    if (!force) {
      return false;
    }

    // Force protocol allocation.
    if (Protocol* protocol = objc_allocateProtocol(protocolName)) {
      objc_registerProtocol(protocol);
      return class_addProtocol(m_classObject, protocol);
    }

    return false;
  }

  //  bool add_protocol(const char* protocolName, bool force = false) {
  //    Protocol* protocol = objc_getProtocol(protocolName);
  //
  //    if (protocol) {
  //      return class_addProtocol(m_classObject, protocol);
  //    }
  //    else if (force) {
  //      protocol = objc_allocateProtocol(protocolName);
  //      objc_registerProtocol(protocol);
  //    }
  //    else {
  //      return false;
  //    }
  //
  //    if (protocol) {
  //      return class_addProtocol(m_classObject, protocol);
  //    }
  //
  //    return false;
  //  }

private:
  Class m_classObject;

  template <auto FunctionType, typename ReturnType, typename... Args>
  inline bool add_member_method_impl(ReturnType (Descriptor::*)(Args...), SEL selector, const char* signature) {
    return class_addMethod(
        m_classObject, selector,
        (IMP)(method_ptr<ReturnType, Args...>)[](id self, SEL, Args... args) {
          auto* p = nano::get_ivar<Descriptor*>(self, Descriptor::valueName);
          return p ? (p->*FunctionType)(args...) : return_default_value<ReturnType>();
        },
        signature);
  }
};

NANO_CLANG_POP_WARNING()

template <typename T>
struct ns_unique_ptr : std::unique_ptr<T, detail::obj_c_object_deleter> {
  using std::unique_ptr<T, detail::obj_c_object_deleter>::unique_ptr;
};

template <typename R = void, typename... Args, typename... Params>
inline nano::objc_unique_ptr create_objc_object(const char* classType, const char* initFct, Params&&... params) {
  nano::objc_unique_ptr obj(class_createInstance(objc_getClass(classType), 0));
  nano::call<R, Args...>(obj.get(), initFct, std::forward<Params>(params)...);
  return obj;
}

template <typename R = void, typename... Args, typename... Params>
inline id create_objc_object_ref(const char* classType, const char* initFct, Params&&... params) {
  id obj = class_createInstance(objc_getClass(classType), 0);
  nano::call<R, Args...>(obj, initFct, std::forward<Params>(params)...);
  return obj;
}

inline nano::objc_unique_ptr to_ns_image(const nano::image& img) {
  if (!img.is_valid() || img.get_size().empty()) {
    return nullptr;
  }

  return create_objc_object<id, CGImageRef, CGSize>("NSImage",
      "initWithCGImage:size:", reinterpret_cast<CGImageRef>(img.get_native_image()),
      static_cast<CGSize>(img.get_size()));
}

inline nano::image from_ns_image(id nsImg) {
  return nano::image(
      reinterpret_cast<nano::native_image_ref>(nano::call<CGImageRef, CGRect*, CGContextRef, CFDictionaryRef>(
          nsImg, "CGImageForProposedRect:context:hints:", nullptr, nullptr, nullptr)));
}

inline nano::image from_ns_image(const nano::objc_unique_ptr& nsImg) {
  return nano::image(
      reinterpret_cast<nano::native_image_ref>(nano::call<CGImageRef, CGRect*, CGContextRef, CFDictionaryRef>(
          nsImg.get(), "CGImageForProposedRect:context:hints:", nullptr, nullptr, nullptr)));
}

struct image::native {
  //    native(CGImageRef img) : m_img(img) {
  //
  //    }
  //
  //    ~native() {
  //        CGImageRelease(m_img);
  //    }
  //
  //    inline CGImageRef get() const {
  //        return m_img;
  //    }
  //
  //    CGImageRef m_img;
};

// image::image() {}

image::image(const char* filepath) {
  CGDataProviderRef dataProvider = CGDataProviderCreateWithFilename(filepath);
  assert(dataProvider != nullptr);

  if (!dataProvider) {
    m_scale_factor = 1;
    return;
  }

  m_native = reinterpret_cast<native*>(
      CGImageCreateWithPNGDataProvider(dataProvider, nullptr, true, kCGRenderingIntentDefault));

  CGDataProviderRelease(dataProvider);
}

image::image(const char* filepath, double scaleFactor)
    : image(filepath) {
  m_scale_factor = scaleFactor;
}

image::image(const image& img)
    : m_native(img.m_native)
    , m_scale_factor(img.m_scale_factor) {

  if (m_native) {
    CGImageRetain(reinterpret_cast<CGImageRef>(m_native));
  }
}

image::image(image&& img)
    : m_native(std::move(img.m_native))
    , m_scale_factor(img.m_scale_factor) {
  img.m_scale_factor = 1;
  img.m_native = nullptr;
}

image::image(native_image_ref nativeImg, double scaleFactor)
    : m_native(reinterpret_cast<native*>(nativeImg))
    , m_scale_factor(scaleFactor) {
  if (m_native) {
    CGImageRetain(reinterpret_cast<CGImageRef>(m_native));
  }
}

image::~image() {
  if (m_native) {
    CGImageRelease(reinterpret_cast<CGImageRef>(m_native));
  }
}

image& image::operator=(const image& img) {
  if (m_native == img.m_native) {
    m_scale_factor = img.m_scale_factor;
    return *this;
  }

  if (m_native) {
    CGImageRelease(reinterpret_cast<CGImageRef>(m_native));
  }

  m_native = img.m_native;
  m_scale_factor = img.m_scale_factor;

  if (m_native) {
    CGImageRetain(reinterpret_cast<CGImageRef>(m_native));
  }

  return *this;
}

image& image::operator=(image&& img) {
  if (m_native) {
    CGImageRelease(reinterpret_cast<CGImageRef>(m_native));
  }

  m_native = img.m_native;
  m_scale_factor = img.m_scale_factor;
  img.m_scale_factor = 1;
  img.m_native = nullptr;
  return *this;
}

native_image_ref image::get_native_image() { return reinterpret_cast<native_image_ref>(m_native); }

native_image_ref image::get_native_image() const { return reinterpret_cast<native_image_ref>(m_native); }

bool image::is_valid() const { return m_native != nullptr; }

nano::size<int> image::get_size() const {
  if (!is_valid()) {
    return { 0, 0 };
  }

  int w = static_cast<int>(CGImageGetWidth(reinterpret_cast<CGImageRef>(m_native)));
  int h = static_cast<int>(CGImageGetHeight(reinterpret_cast<CGImageRef>(m_native)));
  return nano::size<int>(w, h);
  //    return nano::size<int>(w, h) * (((float)1) / m_scale_factor);
}

nano::size<int> image::get_scaled_size() const {
  if (!is_valid()) {
    return nano::size<int>::zero();
  }

  int w = static_cast<int>(CGImageGetWidth(reinterpret_cast<CGImageRef>(m_native)));
  int h = static_cast<int>(CGImageGetHeight(reinterpret_cast<CGImageRef>(m_native)));
  double ratio = 1.0 / m_scale_factor;

  return nano::size<int>(static_cast<int>(w * ratio), static_cast<int>(h * ratio));
}

image image::make_copy() {
  if (!is_valid()) {
    return image();
  }

  image img;
  img.m_native = reinterpret_cast<native*>(CGImageCreateCopy(reinterpret_cast<CGImageRef>(m_native)));
  img.m_scale_factor = m_scale_factor;
  return img;
}

// CGImageRef CGImageCreateWithImageInRect(CGImageRef image, CGRect rect);

image image::get_sub_image(const nano::rect<int>& r) const {
  CGImageRef cgImage = CGImageCreateWithImageInRect(reinterpret_cast<CGImageRef>(m_native), static_cast<CGRect>(r));
  image img(reinterpret_cast<native_image_ref>(cgImage));
  CGImageRelease(cgImage);
  return img;
}

namespace {
  static inline CGContextRef create_bitmap_context(const nano::size<int>& size) {
    CGColorSpaceRef colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);

    CGContextRef context
        = CGBitmapContextCreate(nullptr, static_cast<size_t>(size.width), static_cast<size_t>(size.height), 8,
            static_cast<size_t>(size.width) * 4, colorSpace, kCGImageAlphaPremultipliedLast);

    CGColorSpaceRelease(colorSpace);

    return context;
  }

  static inline image create_colored_image_impl(const nano::image& img, const nano::color& color) {
    nano::rect<int> rect = img.get_rect();

    CGContextRef ctx = create_bitmap_context(rect.size);
    CGContextClipToMask(ctx, static_cast<CGRect>(rect), reinterpret_cast<CGImageRef>(img.get_native_image()));

    CGContextSetRGBFillColor(
        ctx, color.red<CGFloat>(), color.green<CGFloat>(), color.blue<CGFloat>(), color.alpha<CGFloat>());
    CGContextFillRect(ctx, static_cast<CGRect>(rect));

    CGImageRef cgImage = CGBitmapContextCreateImage(ctx);
    nano::image outputImage(reinterpret_cast<native_image_ref>(cgImage));
    CGImageRelease(cgImage);

    return outputImage;
  }
} // namespace.

image image::create_colored_image(const nano::color& color) const { return create_colored_image_impl(*this, color); }

//
//
//

struct font::native {
  ~native() = default;
};

font::font(const char* fontName, double fontSize)
    : m_font_size(fontSize) {

  CFStringRef cfName = CFStringCreateWithCString(kCFAllocatorDefault, fontName, kCFStringEncodingUTF8);

  m_native = const_cast<native*>(
      reinterpret_cast<const native*>(CTFontCreateWithName(cfName, static_cast<CGFloat>(m_font_size), nullptr)));

  CFRelease(cfName);

  //  m_native = (native*)CTFontCreateWithName(cfName, static_cast<CGFloat>(m_font_size), nullptr);
}

font::font(const char* filepath, double fontSize, filepath_tag)
    : m_font_size(fontSize) {
  CFStringRef cgFilepath = CFStringCreateWithCString(kCFAllocatorDefault, filepath, kCFStringEncodingUTF8);

  CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, cgFilepath, kCFURLPOSIXPathStyle, false);
  CFRelease(cgFilepath);

  // This function returns a retained reference to a CFArray of
  // CTFontDescriptorRef objects, or NULL on error. The caller is responsible
  // for releasing the array.
  CFArrayRef cfArray = CTFontManagerCreateFontDescriptorsFromURL(url);
  CFRelease(url);

  if (cfArray == nullptr) {
    m_font_size = 0;
    return;
  }

  CFIndex arraySize = CFArrayGetCount(cfArray);
  if (arraySize == 0) {
    CFRelease(cfArray);
    m_font_size = 0;
    return;
  }

  CTFontDescriptorRef desc = reinterpret_cast<CTFontDescriptorRef>(CFArrayGetValueAtIndex(cfArray, 0));

  m_native = const_cast<native*>(reinterpret_cast<const native*>(
      CTFontCreateWithFontDescriptor(desc, static_cast<CGFloat>(m_font_size), nullptr)));
  CFRelease(cfArray);
}

font::font(const std::uint8_t* data, std::size_t data_size, double font_size)
    : m_font_size(font_size) {
  if (!data || !data_size) {
    m_font_size = 0;
    return;
  }

  CFDataRef cfData = CFDataCreate(nullptr, data, static_cast<CFIndex>(data_size));
  if (!cfData) {
    m_font_size = 0;
    return;
  }

  CTFontDescriptorRef desc = CTFontManagerCreateFontDescriptorFromData(cfData);
  CFRelease(cfData);

  if (!desc) {
    m_font_size = 0;
    return;
  }

  m_native = const_cast<native*>(reinterpret_cast<const native*>(
      CTFontCreateWithFontDescriptor(desc, static_cast<CGFloat>(m_font_size), nullptr)));

  CFRelease(desc);
}

font::font(const font& f)
    : m_native(f.m_native)
    , m_font_size(f.m_font_size) {
  if (m_native) {

    CFRetain(reinterpret_cast<CTFontRef>(m_native));
  }
}

font::font(font&& f)
    : m_native(f.m_native)
    , m_font_size(f.m_font_size) {
  f.m_font_size = 0;
  f.m_native = nullptr;
}

font::~font() {
  if (m_native) {
    CFRelease(reinterpret_cast<CTFontRef>(m_native));
  }
}

font& font::operator=(const font& f) {
  if (m_native) {
    CFRelease(reinterpret_cast<CTFontRef>(m_native));
  }

  m_native = (f.m_native);
  m_font_size = f.m_font_size;

  if (m_native) {
    CFRetain(reinterpret_cast<CTFontRef>(m_native));
  }

  return *this;
}

font& font::operator=(font&& f) {
  if (m_native) {
    CFRelease(reinterpret_cast<CTFontRef>(m_native));
  }

  m_native = f.m_native;
  m_font_size = f.m_font_size;

  f.m_native = nullptr;
  f.m_font_size = 0;
  return *this;
}

bool font::is_valid() const noexcept { return m_native != nullptr; }

double font::get_height() const noexcept {
  return static_cast<double>(CTFontGetCapHeight(reinterpret_cast<CTFontRef>(m_native)));
}

struct Advances {
  Advances(CTRunRef run, CFIndex numGlyphs)
      : advances(CTRunGetAdvancesPtr(run)) {

    if (advances == nullptr) {
      local.resize(static_cast<size_t>(numGlyphs));
      CTRunGetAdvances(run, CFRangeMake(0, 0), local.data());
      advances = local.data();
    }
  }

  const CGSize* advances;
  std::vector<CGSize> local;
};

struct Glyphs {
  Glyphs(CTRunRef run, CFIndex numGlyphs)
      : glyphs(CTRunGetGlyphsPtr(run)) {
    if (glyphs == nullptr) {
      local.resize(static_cast<size_t>(numGlyphs));
      CTRunGetGlyphs(run, CFRangeMake(0, 0), local.data());
      glyphs = local.data();
    }
  }

  const CGGlyph* glyphs;
  std::vector<CGGlyph> local;
};

float font::get_string_width(std::string_view text) const {
  if (text.empty()) {
    return 0;
  }

  nano::cf_unique_ptr<CFDictionaryRef> attributes(nano::create_cf_dictionary(
      { kCTFontAttributeName, kCTLigatureAttributeName }, { reinterpret_cast<CTFontRef>(m_native), kCFBooleanTrue }));

  nano::cf_unique_ptr<CFStringRef> str = nano::create_cf_string_ptr(text);

  nano::cf_unique_ptr<CFAttributedStringRef> attr_str(
      CFAttributedStringCreate(kCFAllocatorDefault, str.get(), attributes.get()));

  nano::cf_unique_ptr<CTLineRef> line(CTLineCreateWithAttributedString(attr_str.get()));
  CFArrayRef run_array = CTLineGetGlyphRuns(line.get());

  float x = 0;

  for (CFIndex i = 0; i < CFArrayGetCount(run_array); i++) {
    CTRunRef run = reinterpret_cast<CTRunRef>(CFArrayGetValueAtIndex(run_array, i));
    CFIndex length = CTRunGetGlyphCount(run);

    const Advances advances(run, length);

    for (CFIndex j = 0; j < length; j++) {
      x += static_cast<float>(advances.advances[j].width);
    }
  }

  return x;
}

native_font_ref font::get_native_font() noexcept { return reinterpret_cast<native_font_ref>(m_native); }

native_font_ref font::get_native_font() const noexcept { return reinterpret_cast<native_font_ref>(m_native); }

//
//
//

class graphic_context::native {
public:
  ~native() = default;
};

namespace {
  static inline CGContextRef to_cg_context(graphic_context::native* native) {
    return reinterpret_cast<CGContextRef>(native);
  }
} // namespace

graphic_context::graphic_context(native_graphic_context_ref nc) { m_native = reinterpret_cast<native*>(nc); }

graphic_context::~graphic_context() {}

void graphic_context::save_state() { CGContextSaveGState(to_cg_context(m_native)); }

void graphic_context::restore_state() { CGContextRestoreGState(to_cg_context(m_native)); }

void graphic_context::begin_transparent_layer(float alpha) {
  save_state();
  CGContextSetAlpha(to_cg_context(m_native), static_cast<CGFloat>(alpha));
  CGContextBeginTransparencyLayer(to_cg_context(m_native), nullptr);
}

void graphic_context::end_transparent_layer() {
  CGContextEndTransparencyLayer(to_cg_context(m_native));
  restore_state();
}

void graphic_context::translate(const nano::point<float>& pos) {
  CGContextTranslateCTM(to_cg_context(m_native), static_cast<CGFloat>(pos.x), static_cast<CGFloat>(pos.y));
}

void graphic_context::clip() { CGContextClip(to_cg_context(m_native)); }

void graphic_context::clip_even_odd() { CGContextEOClip(to_cg_context(m_native)); }

void graphic_context::reset_clip() { CGContextResetClip(to_cg_context(m_native)); }

void graphic_context::clip_to_rect(const nano::rect<float>& rect) {
  CGContextClipToRect(to_cg_context(m_native), static_cast<CGRect>(rect));
}

// void graphic_context::clip_to_path(const nano::Path& p)
// {
//     CGContextRef g = to_cg_context(m_native);
//     CGContextBeginPath(g);
//     CGContextAddPath(g, (CGPathRef)p.GetnativePath());
//     CGContextClip(g);
// }

// void graphic_context::clip_to_path_even_odd(const nano::Path& p)
// {
//     CGContextRef g = to_cg_context(m_native);
//     CGContextBeginPath(g);
//     CGContextAddPath(g, (CGPathRef)p.get_native_path());
//     CGContextEOClip(g);
// }

static inline void flip(CGContextRef c, float flipHeight) {
  CGContextConcatCTM(c, CGAffineTransformMake(1.0f, 0.0f, 0.0f, -1.0f, 0.0f, flipHeight));
}

void graphic_context::clip_to_mask(const nano::image& img, const nano::rect<float>& rect) {
  CGContextRef g = to_cg_context(m_native);
  CGContextTranslateCTM(g, static_cast<CGFloat>(rect.x), static_cast<CGFloat>(rect.y));
  flip(g, rect.height);
  CGContextClipToMask(
      g, static_cast<CGRect>(rect.with_position({ 0, 0 })), reinterpret_cast<CGImageRef>(img.get_native_image()));
  flip(g, rect.height);
  CGContextTranslateCTM(g, static_cast<CGFloat>(-rect.x), static_cast<CGFloat>(-rect.y));
}

void graphic_context::add_rect(const nano::rect<float>& rect) {
  CGContextAddRect(to_cg_context(m_native), static_cast<CGRect>(rect));
}

// void graphic_context::add_path(const nano::Path& p)
// {
//     CGContextAddPath(to_cg_context(m_native), (CGPathRef)p.GetNativePath());
// }

void graphic_context::begin_path() { CGContextBeginPath(to_cg_context(m_native)); }
void graphic_context::close_path() { CGContextClosePath(to_cg_context(m_native)); }

nano::rect<float> graphic_context::get_clipping_rect() const {
  return CGContextGetClipBoundingBox(to_cg_context(m_native));
}

void graphic_context::set_line_width(float width) {
  CGContextSetLineWidth(to_cg_context(m_native), static_cast<CGFloat>(width));
}

void graphic_context::set_line_join(line_join lj) {
  switch (lj) {
  case line_join::miter:
    CGContextSetLineJoin(to_cg_context(m_native), kCGLineJoinMiter);
    break;

  case line_join::round:
    CGContextSetLineJoin(to_cg_context(m_native), kCGLineJoinRound);
    break;

  case line_join::bevel:
    CGContextSetLineJoin(to_cg_context(m_native), kCGLineJoinBevel);
    break;
  }
}

void graphic_context::set_line_cap(line_cap lc) {
  switch (lc) {
  case line_cap::butt:
    CGContextSetLineCap(to_cg_context(m_native), kCGLineCapButt);
    break;

  case line_cap::round:
    CGContextSetLineCap(to_cg_context(m_native), kCGLineCapRound);
    break;

  case line_cap::square:
    CGContextSetLineCap(to_cg_context(m_native), kCGLineCapSquare);
    break;
  }
}

void graphic_context::set_line_style(float width, line_join lj, line_cap lc) {
  set_line_width(width);
  set_line_join(lj);
  set_line_cap(lc);
}

void graphic_context::set_fill_color(const nano::color& c) {
  CGContextRef cg = to_cg_context(m_native);
  CGColorRef color
      = CGColorCreateGenericRGB(c.red<CGFloat>(), c.green<CGFloat>(), c.blue<CGFloat>(), c.alpha<CGFloat>());
  CGContextSetFillColorWithColor(cg, color);
  CGColorRelease(color);
}

void graphic_context::set_stroke_color(const nano::color& c) {
  CGContextRef cg = to_cg_context(m_native);
  CGColorRef color
      = CGColorCreateGenericRGB(c.red<CGFloat>(), c.green<CGFloat>(), c.blue<CGFloat>(), c.alpha<CGFloat>());
  CGContextSetStrokeColorWithColor(cg, color);
  CGColorRelease(color);
}

void graphic_context::fill_rect(const nano::rect<float>& r) {
  CGContextFillRect(to_cg_context(m_native), r.convert<CGRect>());
}

void graphic_context::stroke_rect(const nano::rect<float>& r) {
  CGContextStrokeRect(to_cg_context(m_native), r.convert<CGRect>());
}

void graphic_context::stroke_rect(const nano::rect<float>& r, float lineWidth) {
  CGContextRef cg = to_cg_context(m_native);
  CGContextSetLineWidth(cg, static_cast<CGFloat>(lineWidth));
  CGContextStrokeRect(cg, static_cast<CGRect>(r));
}

void graphic_context::stroke_line(const nano::point<float>& p0, const nano::point<float>& p1) {
  CGContextRef g = to_cg_context(m_native);
  CGContextMoveToPoint(g, static_cast<CGFloat>(p0.x), static_cast<CGFloat>(p0.y));
  CGContextAddLineToPoint(g, static_cast<CGFloat>(p1.x), static_cast<CGFloat>(p1.y));
  CGContextStrokePath(g);
}

void graphic_context::fill_ellipse(const nano::rect<float>& r) {
  CGContextRef g = to_cg_context(m_native);
  CGContextAddEllipseInRect(g, static_cast<CGRect>(r));
  CGContextFillPath(g);
}

void graphic_context::stroke_ellipse(const nano::rect<float>& r) {
  CGContextRef g = to_cg_context(m_native);
  CGContextAddEllipseInRect(g, static_cast<CGRect>(r));
  CGContextStrokePath(g);
}

void graphic_context::fill_rounded_rect(const nano::rect<float>& r, float radius) {
  CGContextRef g = to_cg_context(m_native);
  CGPathRef path = CGPathCreateWithRoundedRect(
      r.convert<CGRect>(), static_cast<CGFloat>(radius), static_cast<CGFloat>(radius), nullptr);
  CGContextBeginPath(g);
  CGContextAddPath(g, path);
  CGContextFillPath(g);
  CGPathRelease(path);
}

void graphic_context::stroke_rounded_rect(const nano::rect<float>& r, float radius) {
  CGContextRef g = to_cg_context(m_native);
  CGPathRef path = CGPathCreateWithRoundedRect(
      r.convert<CGRect>(), static_cast<CGFloat>(radius), static_cast<CGFloat>(radius), nullptr);
  CGContextBeginPath(g);
  CGContextAddPath(g, path);
  CGContextStrokePath(g);
  CGPathRelease(path);
}

// void graphic_context::fill_path(const nano::Path& p)
// {
//     CGContextRef g = to_cg_context(m_native);
//     CGContextAddPath(g, (CGPathRef)p.GetNativePath());
//     CGContextFillPath(g);
// }

// void graphic_context::fill_path(const nano::Path& p, const nano::rect<float>& rect)
// {
//     SaveState();
//     Translate(rect.position);

//     CGContextRef g = to_cg_context(m_native);

//     CGContextScaleCTM(g, rect.width, rect.height);
//     CGContextAddPath(g, (CGPathRef)p.GetNativePath());
//     CGContextFillPath(g);

//     RestoreState();
// }

// void graphic_context::fill_path_with_shadow(
//     const nano::Path& p, float blur, const nano::color& shadow_color, const nano::size<float>& offset)
// {

//     SaveState();
//     CGContextRef g = to_cg_context(m_native);
//     CGColorRef scolor = CGColorCreateGenericRGB(
//         shadow_color.fRed(), shadow_color.fGreen(), shadow_color.fBlue(), shadow_color.fAlpha());

//     CGContextSetShadowWithColor(g, (CGSize)offset, blur, scolor);
//     CGColorRelease(scolor);
//     FillPath(p);

//     RestoreState();
// }

// void graphic_context::stroke_path(const nano::Path& p)
// {
//     CGContextRef g = to_cg_context(m_native);
//     CGContextAddPath(g, (CGPathRef)p.get_native_path());
//     CGContextStrokePath(g);
// }

void graphic_context::draw_image(const nano::image& img, const nano::point<float>& pos) {
  CGContextRef g = to_cg_context(m_native);
  nano::rect<float> rect = { pos, img.get_scaled_size() };

  CGContextTranslateCTM(g, static_cast<CGFloat>(rect.x), static_cast<CGFloat>(rect.y));
  flip(g, rect.height);

  CGContextDrawImage(
      g, static_cast<CGRect>(rect.with_position({ 0.0f, 0.0f })), reinterpret_cast<CGImageRef>(img.get_native_image()));

  flip(g, rect.height);
  CGContextTranslateCTM(g, static_cast<CGFloat>(-rect.x), static_cast<CGFloat>(-rect.y));
}

void graphic_context::draw_image(const nano::image& img, const nano::rect<float>& rect) {
  CGContextRef g = to_cg_context(m_native);
  CGContextTranslateCTM(g, static_cast<CGFloat>(rect.x), static_cast<CGFloat>(rect.y));
  flip(g, rect.height);
  CGContextDrawImage(
      g, rect.with_position({ 0.0f, 0.0f }).convert<CGRect>(), reinterpret_cast<CGImageRef>(img.get_native_image()));
  flip(g, rect.height);
  CGContextTranslateCTM(g, static_cast<CGFloat>(-rect.x), static_cast<CGFloat>(-rect.y));
}

void graphic_context::draw_image(
    const nano::image& img, const nano::rect<float>& rect, const nano::rect<float>& clipRect) {
  CGContextRef g = to_cg_context(m_native);
  save_state();
  CGContextTranslateCTM(g, static_cast<CGFloat>(rect.x), static_cast<CGFloat>(rect.y));
  clip_to_rect(clipRect);
  flip(g, rect.height);
  CGContextDrawImage(
      g, rect.with_position({ 0.0f, 0.0f }).convert<CGRect>(), reinterpret_cast<CGImageRef>(img.get_native_image()));
  restore_state();
}

void graphic_context::draw_sub_image(
    const nano::image& img, const nano::rect<float>& rect, const nano::rect<float>& imgRect) {
  CGContextRef g = to_cg_context(m_native);

  nano::image sImg = img.get_sub_image(imgRect);

  CGContextTranslateCTM(g, static_cast<CGFloat>(rect.x), static_cast<CGFloat>(rect.y));
  flip(g, rect.height);
  CGContextDrawImage(
      g, rect.with_position({ 0.0f, 0.0f }).convert<CGRect>(), reinterpret_cast<CGImageRef>(sImg.get_native_image()));
  flip(g, rect.height);
  CGContextTranslateCTM(g, static_cast<CGFloat>(-rect.x), static_cast<CGFloat>(-rect.y));
}

//
// MARK: Text.
//
NANO_INLINE_CXPR double k_default_mac_font_height = 11.0;

void graphic_context::draw_text(const nano::font& f, const std::string& text, const nano::point<float>& pos) {
  const double fontHeight = f.is_valid() ? f.get_height() : k_default_mac_font_height;
  CGContextRef g = to_cg_context(m_native);

  nano::cf_unique_ptr<CFDictionaryRef> attributes(
      nano::create_cf_dictionary({ kCTFontAttributeName, kCTForegroundColorFromContextAttributeName },
          { reinterpret_cast<CTFontRef>(f.get_native_font()), kCFBooleanTrue }));

  nano::cf_unique_ptr<CFStringRef> str = nano::create_cf_string_ptr(text);
  nano::cf_unique_ptr<CFAttributedStringRef> attr_str(
      CFAttributedStringCreate(kCFAllocatorDefault, str.get(), attributes.get()));
  nano::cf_unique_ptr<CTLineRef> line(CTLineCreateWithAttributedString(attr_str.get()));

  CGContextSetTextDrawingMode(g, kCGTextFill);
  CGContextSetTextMatrix(g, CGAffineTransformMake(1.0, 0.0, 0.0, -1.0, 0.0, fontHeight));
  CGContextSetTextPosition(g, static_cast<CGFloat>(pos.x), static_cast<CGFloat>(pos.y) + fontHeight);
  CTLineDraw(line.get(), g);
}

void graphic_context::draw_text(
    const nano::font& f, const std::string& text, const nano::rect<float>& rect, nano::text_alignment alignment) {
  const double fontHeight = f.is_valid() ? f.get_height() : k_default_mac_font_height;
  CGContextRef g = to_cg_context(m_native);

  nano::cf_unique_ptr<CFDictionaryRef> attributes(
      nano::create_cf_dictionary({ kCTFontAttributeName, kCTForegroundColorFromContextAttributeName },
          { reinterpret_cast<CTFontRef>(f.get_native_font()), kCFBooleanTrue }));

  nano::cf_unique_ptr<CFStringRef> str = nano::create_cf_string_ptr(text);
  nano::cf_unique_ptr<CFAttributedStringRef> attr_str(
      CFAttributedStringCreate(kCFAllocatorDefault, str.get(), attributes.get()));
  nano::cf_unique_ptr<CTLineRef> line(CTLineCreateWithAttributedString(attr_str.get()));

  nano::point<float> textPos = { 0.0f, 0.0f };

  // Left.
  switch (alignment) {
  case nano::text_alignment::left: {
    textPos = nano::point<float>(rect.x, rect.y + (rect.height + static_cast<float>(fontHeight)) * 0.5f);
  } break;

  case nano::text_alignment::center: {
    float textWidth = static_cast<float>(CTLineGetTypographicBounds(line.get(), nullptr, nullptr, nullptr));
    textPos = rect.position
        + nano::point<float>(rect.width - textWidth, rect.height + static_cast<float>(fontHeight)) * 0.5f;
  } break;

  case nano::text_alignment::right: {
    float textWidth = static_cast<float>(CTLineGetTypographicBounds(line.get(), nullptr, nullptr, nullptr));
    textPos = rect.position
        + nano::point<float>(rect.width - textWidth, (rect.height + static_cast<float>(fontHeight)) * 0.5f);
  }

  break;
  }

  CGContextSetTextDrawingMode(g, kCGTextFill);
  CGContextSetTextMatrix(g, CGAffineTransformMake(1.0, 0.0, 0.0, -1.0, 0.0, fontHeight));
  CGContextSetTextPosition(g, static_cast<CGFloat>(textPos.x), static_cast<CGFloat>(textPos.y));
  CTLineDraw(line.get(), g);
}

native_graphic_context_ref graphic_context::get_native_handle() const {
  return reinterpret_cast<native_graphic_context_ref>(m_native);
}

namespace {

  template <typename T>
  inline id to_objc_id(T* obj) {
    return reinterpret_cast<id>(obj);
  }

  inline nano::point<int> get_location_in_view(native_event_handle handle, nano::view* view) {
    CGPoint locInWindow = nano::call<CGPoint>(to_objc_id(handle), "locationInWindow");
    return nano::call<CGPoint, CGPoint, id>(
        to_objc_id(view->get_native_handle()), "convertPoint:fromView:", static_cast<CGPoint>(locInWindow), nullptr);
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

  CGEventRef evt = nano::call<CGEventRef>(to_objc_id(m_native_handle), "CGEvent");
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
    m_wheel_delta = nano::point<CGFloat>(nano::call<CGFloat>(to_objc_id(m_native_handle), "deltaX"),
        nano::call<CGFloat>(to_objc_id(m_native_handle), "deltaY"));
  }

  if (nano::is_click_event(m_type)) {
    m_click_count = static_cast<int>(CGEventGetIntegerValueField(evt, kCGMouseEventClickState));
  }

  m_event_modifiers = get_event_modifiers_from_cg_event(evt);
  m_timestamp = CGEventGetTimestamp(evt);
}

nano::point<float> event::get_screen_position() const noexcept {
  CGEventRef evt = nano::call<CGEventRef>(to_objc_id(m_native_handle), "CGEvent");
  return CGEventGetLocation(evt);
}

const nano::point<float> event::get_window_position() const noexcept {
  nano::rect<float> frame = nano::call<CGRect>(to_objc_id(get_native_window()), "contentLayoutRect");
  nano::point<float> pos = nano::call<CGPoint>(to_objc_id(m_native_handle), "locationInWindow");
  pos.y = frame.height - pos.y;
  return pos;
}


// std::uint64_t event::get_timestamp() const noexcept
//{
//     CGEventRef evt = nano::call<CGEventRef>((id)m_native_handle, "CGEvent");
//     return CGEventGetTimestamp(evt);
// }

native_window_handle event::get_native_window() const noexcept {
  return reinterpret_cast<native_window_handle>(nano::call<id>(to_objc_id(m_native_handle), "window"));
}

const nano::point<float> event::get_bounds_position() const noexcept {
  return m_position - m_view->get_bounds().position;
}

std::u16string event::get_key() const noexcept {
  CGEventRef evt = nano::call<CGEventRef>(to_objc_id(m_native_handle), "CGEvent");
  UniCharCount length = 0;
  std::u16string str(20, 0);
  CGEventKeyboardGetUnicodeString(evt, 20, &length, (UniChar*)str.data());

  return str;
}
//CGEventKeyboardGetUnicodeString(CGEventRef event, UniCharCount maxStringLength, UniCharCount *actualStringLength, UniChar *unicodeString);

class window_object {
public:
  static constexpr const char* className = "NanoWindowObject";
  static constexpr const char* baseName = "NSObject";
  static constexpr const char* valueName = "owner";

  window_object(nano::view* view, window_flags flags)
      : m_view(view) {

    m_obj = classObject.create_instance();
    nano::call(m_obj, "init");
    ClassObject::set_pointer(m_obj, this);

    //        constexpr unsigned int uiNSWindowStyleMaskBorderless = 0;
    constexpr unsigned long uiNSWindowStyleMaskTitled = 1 << 0;
    constexpr unsigned long uiNSWindowStyleMaskClosable = 1 << 1;
    constexpr unsigned long uiNSWindowStyleMaskMiniaturizable = 1 << 2;
    constexpr unsigned long uiNSWindowStyleMaskResizable = 1 << 3;
    constexpr unsigned long uiNSWindowStyleMaskUtilityWindow = 1 << 4;
    constexpr unsigned long uiNSWindowStyleMaskFullSizeContentView = 1 << 15;

    bool isPanel = (flags & window_flags::panel) != 0;
    unsigned long nsFlags = 0;

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
    m_window = create_objc_object_ref<void, CGRect, unsigned long, unsigned long, BOOL>(windowClassName,
        "initWithContentRect:styleMask:backing:defer:", CGRect{ { 0, 0 }, { 300, 300 } }, nsFlags, 2UL, YES);

    nano::icall(m_window, "setDelegate:", m_obj);
    nano::icall(m_window, "setContentView:", m_view->get_native_handle());
    nano::call<void, BOOL>(m_window, "setReleasedWhenClosed:", false);

    if ((flags & window_flags::full_size_content_view) != 0) {
      nano::call<void, BOOL>(m_window, "setTitlebarAppearsTransparent:", true);
    }

    nano::icall(m_window, "makeKeyAndOrderFront:");
    nano::call(m_window, "center");

    create_menu();
    //
  }

  ~window_object() {
    std::cout << "WindpwComponen :: ~Native" << std::endl;

    if (m_window) {

      nano::icall(m_window, "setDelegate:");
    }

    if (m_obj) {
      ClassObject::set_pointer(m_obj, nullptr);
      nano::objc_reset(m_obj);
    }

    if (m_window) {
      nano::objc_reset(m_window);
    }
  }

  native_window_handle get_native_handle() const { return reinterpret_cast<native_window_handle>(m_window); }

  void close() { nano::call(m_window, "close"); }

  void set_shadow(bool visible) { nano::call<void, BOOL>(m_window, "setHasShadow:", visible); }

  void set_title(std::string_view title) {
    CFStringRef str = CFStringCreateWithBytes(kCFAllocatorDefault, reinterpret_cast<const UInt8*>(title.data()),
        static_cast<CFIndex>(title.size()), kCFStringEncodingUTF8, false);

    nano::call<void, CFStringRef>(m_window, "setTitle:", str);

    CFRelease(str);
  }

  void set_frame(const nano::rect<int>& rect) {
    nano::call<void, CGRect, BOOL>(m_window, "setFrame:display:", rect.convert<CGRect>(), true);
  }

  nano::rect<int> get_frame() const { return nano::call<CGRect>(m_window, "frame"); }

  void center() { nano::call(m_window, "center"); }

  void set_background_color(const nano::color& c) {
    nano::cf_unique_ptr<CGColorRef> color = nano::to_cg_color_ref(c);
    id nsColor = nano::call_meta<id, CGColorRef>("NSColor", "colorWithCGColor:", color.get());
    nano::icall(m_window, "setBackgroundColor:", nsColor);
  }

  //    void set_background_color(nano::system_color scolor) { SetBackgroundColor(GetSystemColor(scolor)); }

  void set_flags(window_flags flags) {
    //        constexpr unsigned int uiNSWindowStyleMaskBorderless = 0;
    constexpr unsigned long uiNSWindowStyleMaskTitled = 1 << 0;
    constexpr unsigned long uiNSWindowStyleMaskClosable = 1 << 1;
    constexpr unsigned long uiNSWindowStyleMaskMiniaturizable = 1 << 2;
    constexpr unsigned long uiNSWindowStyleMaskResizable = 1 << 3;

    unsigned long nsFlags = 0;

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

    nano::call<void, unsigned long>(m_window, "setStyleMask:", nsFlags);

    //        - (void)setDocumentEdited:(BOOL)dirtyFlag;
  }

  void set_document_edited(bool dirty) {
    nano::call<void, BOOL>(m_window, "setDocumentEdited:", static_cast<BOOL>(dirty));
  }

  void set_delegate(window_proxy::delegate* d) { m_window_delegate = d; }

  inline id create_menu_item(const char* title, const char* keyEquivalent, long tag) {
    static SEL initItemWithTitleSelector = nano::get_selector("initWithTitle:action:keyEquivalent:");
    static SEL menuActionSelector = nano::get_selector("menuAction:");

    id item = class_createInstance(objc_getClass("NSMenuItem"), 0);
    nano::call<void, CFStringRef, SEL, CFStringRef>(item, initItemWithTitleSelector, create_cf_string_ptr(title).get(),
        menuActionSelector, create_cf_string_ptr(keyEquivalent).get());

    nano::call<void, long>(item, "setTag:", tag);
    nano::icall(item, "setTarget:", m_obj);

    return item;
  }

  inline id create_menu(const char* title) {

    return create_objc_object_ref<void, CFStringRef>("NSMenu", "initWithTitle:", create_cf_string_ptr(title).get());
  }

  inline void add_menu_item(id menu, id item, bool release_item = false) {
    static SEL addItemSelector = nano::get_selector("addItem:");
    nano::icall(menu, addItemSelector, item);

    if (release_item) {
      nano::objc_release(item);
    }
  }

  inline void set_submenu(id item, id submenu, bool release_submenu = false) {
    static SEL selector = nano::get_selector("setSubmenu:");
    nano::icall(item, selector, submenu);

    if (release_submenu) {
      nano::objc_release(submenu);
    }
  }

  void create_menu() {

    id mainMenu = create_menu("Title");

    {
      id aboutMenu = create_menu("AppName");
      add_menu_item(aboutMenu, create_menu_item("About AppName", "", 1111), true);
      add_menu_item(aboutMenu, create_menu_item("Quit AppName", "q", 128932), true);

      id aboutMenuItem = create_menu_item("UI", "", 0);
      set_submenu(aboutMenuItem, aboutMenu, true);
      add_menu_item(mainMenu, aboutMenuItem, true);
    }

    {
      id fileMenu = create_menu("File");
      add_menu_item(fileMenu, create_menu_item("New", "", 232), true);

      id fileMenuItem = create_menu_item("UI", "", 0);
      set_submenu(fileMenuItem, fileMenu, true);
      add_menu_item(mainMenu, fileMenuItem, true);
    }

    objc_object* sharedApplication = nano::get_class_property("NSApplication", "sharedApplication");
    nano::call<void, objc_object*>(sharedApplication, "setMainMenu:", mainMenu);
    nano::objc_release(mainMenu);
  }

  nano::view* m_view;
  id m_window = nullptr;
  id m_obj;
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

  BOOL window_should_close([[maybe_unused]] objc_object* sender) {
    std::cout << "dddd window_should_close" << std::endl;

    if (m_window_delegate) {
      return m_window_delegate->window_should_close(m_view);
    }

    return true;
  }

  void on_menu_action(id sender) {
    long tag = nano::call<long>(sender, "tag");

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

  class ClassObject : public nano::objc_class<window_object> {
  public:
    using ClassType = window_object;

    ClassObject()
        : objc_class("WindowComponentClassObject") {

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

      //      register_class();
    }

    static void Dealloc(id self, SEL) {
      std::cout << "WindowComponent->Dealloc" << std::endl;

      if (auto* p = get_pointer(self)) {
        p->on_dealloc();
      }

      send_superclass_message<void>(self, "dealloc");
    }

    static BOOL window_should_close(id self, SEL, id sender) {
      std::cout << "window_should_close" << std::endl;
      if (auto* p = get_pointer(self)) {
        return p->window_should_close(sender);
      }

      return true;
    }

    static void set_pointer(id self, ClassType* owner) { nano::set_ivar(self, ClassType::valueName, owner); }

    static ClassType* get_pointer(id self) { return nano::get_ivar<ClassType*>(self, ClassType::valueName); }
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

    id conf = class_createInstance(objc_getClass("WKWebViewConfiguration"), 0);
    nano::call<void>(conf, nano::get_selector("init"));

    id web_view = class_createInstance(objc_getClass("WKWebView"), 0);
    nano::call<void, CGRect, id>(web_view, nano::get_selector("initWithFrame:configuration:"),
        static_cast<CGRect>(nano::rect<float>(0, 0, 200, 200)), conf);

    nano::objc_release(conf);

    nano::icall(view->get_native_handle(), "addSubview:", web_view);

    CFStringRef url_str
        = CFStringCreateWithCString(kCFAllocatorDefault, "https://www.google.com", kCFStringEncodingUTF8);
    CFURLRef url = CFURLCreateWithString(kCFAllocatorDefault, url_str, nullptr);

    id request = class_createInstance(objc_getClass("NSURLRequest"), 0);
    nano::call<void, CFURLRef>(request, nano::get_selector("initWithURL:"), url);

    [[maybe_unused]] id wk_navigation = nano::call<id, id>(web_view, nano::get_selector("loadRequest:"), request);

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

    nano::call<void, CGRect>(m_obj, "initWithFrame:", rect.convert<CGRect>());

    nano::call<void, BOOL>(m_obj, "setPostsFrameChangedNotifications:", TRUE);

    nano::call<void, objc_object*, objc_selector*, CFStringRef, objc_object*>( //
        nano::get_class_property("NSNotificationCenter", "defaultCenter"), //
        "addObserver:selector:name:object:", //
        m_obj, //
        sel_registerName("frameChanged:"), //
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
      nano::icall(m_obj, "removeTrackingArea:", m_trackingArea);
      nano::objc_reset(m_trackingArea);
    }

    // unsigned long count = CFGetRetainCount(m_obj);
    // std::cout << "native_view::~Native::release " << count << std::endl;
    ClassObject::set_pointer(m_obj, nullptr);
    nano::objc_reset(m_obj);

    std::cout << "native_view::~Native-->Donw " << std::endl;
  }

  void init(view* parent) {
    m_parent = parent;

    nano::icall(parent->get_native_handle(), "addSubview:", m_obj);

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
    nano::icall(parent, "addSubview:", m_obj);

    add_tracking_area(nano::uiNSTrackingMouseEnteredAndExited //
        | nano::uiNSTrackingActiveInKeyWindow //
        | nano::uiNSTrackingMouseMoved //
        | nano::uiNSTrackingInVisibleRect);
  }

  inline id get_window() const { return nano::call<id>(m_obj, "window"); }

  void add_tracking_area(unsigned long opts) {
    m_trackingArea = create_objc_object_ref<void, CGRect, unsigned long, id, id>(
        "NSTrackingArea", "initWithRect:options:owner:userInfo:", CGRectZero, opts, m_obj, nullptr);

    nano::icall(m_obj, "addTrackingArea:", m_trackingArea);
  }

  void initialize() {}

  native_view_handle get_native_handle() const { return reinterpret_cast<native_view_handle>(m_obj); }

  void set_hidden(bool hidden) { nano::call<void, BOOL>(m_obj, "setHidden:", hidden); }

  bool is_hidden() const { return nano::call<BOOL>(m_obj, "isHidden"); }

  void set_frame(const nano::rect<int>& rect) { nano::call<void, CGRect>(m_obj, "setFrame:", rect.convert<CGRect>()); }

  void set_frame_position(const nano::point<int>& pos) {
    nano::call<void, CGPoint>(m_obj, "setFrameOrigin:", static_cast<CGPoint>(pos));
  }

  void set_frame_size(const nano::size<int>& size) {
    nano::call<void, CGSize>(m_obj, "setFrameSize:", static_cast<CGSize>(size));
  }

  //  void set_bounds(const nano::rect<int>& rect) {
  //    nano::call<void, CGRect>(m_obj, "setBounds:", rect.convert<CGRect>());
  //  }

  nano::rect<int> get_frame() const { return nano::call<CGRect>(m_obj, "frame"); }

  nano::point<int> get_window_position() const {
    if (id window = get_window()) {
      return convert_to_view(nano::point<int>(0, 0), nullptr, true);
    }

    return get_frame().origin;
  }

  nano::point<int> get_screen_position() const {
    if (id window = get_window()) {
      id screen = nano::call<id>(window, "screen");

      nano::point<int> screenPos = nano::call<CGPoint, CGPoint>(
          window, "convertPointToScreen:", static_cast<CGPoint>(convert_to_view(get_frame().position, nullptr, false)));

      return screenPos.with_y(static_cast<int>(nano::call<CGRect>(screen, "frame").size.height) - screenPos.y);
    }

    return get_frame().origin;
  }

  nano::rect<int> get_bounds() const { return nano::rect<int>(nano::call<CGRect>(m_obj, "bounds")); }

  nano::rect<int> get_visible_rect() const { return nano::call<CGRect>(m_obj, "visibleRect"); }

  nano::point<int> convert_from_view(const nano::point<int>& point, nano::view* view) const {
    return nano::call<CGPoint, CGPoint, id>(m_obj, "convertPoint:fromView:", static_cast<CGPoint>(point),
        to_objc_id(view ? view->get_native_handle() : nullptr));
  }

  nano::point<int> convert_to_view(const nano::point<int>& point, nano::view* view, bool flip = true) const {
    if (view || !flip) {
      return nano::call<CGPoint, CGPoint, id>(m_obj, "convertPoint:toView:", static_cast<CGPoint>(point),
          to_objc_id(view ? view->get_native_handle() : nullptr));
    }

    if (id window = get_window()) {
      nano::rect<int> windowFrame = nano::call<CGRect>(window, "frame");
      nano::rect<int> frame = get_frame();
      frame.position = nano::call<CGPoint, CGPoint, id>(
          m_obj, "convertPoint:toView:", static_cast<CGPoint>(frame.position), nullptr);
      return frame.with_y(windowFrame.height - frame.y);
    }

    return point;
  }

  bool is_dirty_rect(const nano::rect<int>& rect) const {
    return nano::call<BOOL, CGRect>(m_obj, "needsToDrawRect:", rect.convert<CGRect>());
  }

  void unfocus() {
    if (id window = get_window()) {
      nano::call<BOOL, id>(window, "makeFirstResponder:", nullptr);
    }
  }

  void focus() {
    if (id window = get_window()) {
      nano::call<BOOL, id>(window, "makeFirstResponder:", m_obj);
    }
  }

  bool is_focused() const {
    id window = get_window();
    return window && nano::call<id>(window, "firstResponder") == m_obj;
  }

  void redraw() { nano::call<void, BOOL>(m_obj, "setNeedsDisplay:", true); }

  void redraw(const nano::rect<int>& rect) {
    nano::call<void, CGRect>(m_obj, "setNeedsDisplayInRect:", rect.convert<CGRect>());
  }

  void on_resize([[maybe_unused]] id evt) {
    m_view->on_frame_changed();
  }

  inline event create_event(id evt) { return event(reinterpret_cast<native_event_handle>(evt), m_view); }

#define NANO_IMPL_EVENT(MemberMethod, Method)                                                                          \
  void MemberMethod(id e) { m_view->Method(create_event(e)); }

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

  void on_mouse_moved(id evt) {

    if (id subview = nano::call<id, CGPoint>(m_obj, "hitTest:", nano::call<CGPoint>(evt, "locationInWindow"))) {
      if (auto* p = classObject.get_pointer(subview)) {
        p->m_view->on_mouse_moved(event(reinterpret_cast<native_event_handle>(evt), p->m_view));
        return;
      }
    }

    m_view->on_mouse_moved(create_event(evt));
  }

  void on_will_remove_subview([[maybe_unused]] id v) {
    std::cout << "native_view::Native->on_will_remove_subview" << std::endl;
  }

  void on_will_draw() {
            m_view->on_will_draw();
  }

  void on_draw(nano::rect<float> rect) {
    id nsContext = nano::get_class_property("NSGraphicsContext", "currentContext");
    nano::graphic_context gc(
        reinterpret_cast<native_graphic_context_ref>(nano::call<CGContextRef>(nsContext, "CGContext")));
    m_view->on_draw(gc, rect);
  }

  void on_did_hide() {
    m_view->on_hide();
  }

  void on_did_unhide() {
    m_view->on_show();
  }

  void on_dealloc() { std::cout << "view::~pimpl->on_dealloc " << std::endl; }

  void on_update_tracking_areas() { classObject.send_superclass_message<void>(m_obj, "updateTrackingAreas"); }

  BOOL become_first_responder() {
    m_view->on_focus();
    return true;
  }

  BOOL resign_first_responder() {
    m_view->on_unfocus();
    return true;
  }

  BOOL is_flipped() { return true; }

  //
  view* m_view;
  id m_obj;
  id m_trackingArea = nullptr;
  std::unique_ptr<window_object> m_win;
  view* m_parent = nullptr;
  std::vector<view*> m_children;

private:
  class ClassObject : public nano::objc_class<pimpl> {
  public:
    using ClassType = pimpl;

    ClassObject()
        : objc_class("UIViewClassObject") {

      add_method<dealloc>("dealloc", "v@:");

      //      add_method<method_ptr<BOOL>>(
      //          "isFlipped", [](id, SEL) -> BOOL { return true; }, "c@:");

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

      //      register_class();
    }

    //    static bool become_first_responder(id self, SEL) {
    //      if (auto* p = get_pointer(self)) {
    //        if (p->m_responder) {
    //          p->m_responder->on_focus();
    //        }
    //      }
    //
    //      return true;
    //    }
    //
    //    static bool resign_first_responder(id self, SEL) {
    //      if (auto* p = get_pointer(self)) {
    //        if (p->m_responder) {
    //          p->m_responder->on_unfocus();
    //        }
    //      }
    //
    //      return true;
    //    }

    static void draw_rect(id self, SEL, CGRect r) {
      if (auto* p = get_pointer(self)) {
        p->on_draw(r);
      }
    }

    static void view_will_draw(id self, SEL) {
      if (auto* p = get_pointer(self)) {
        p->on_will_draw();
      }

      send_superclass_message<void>(self, "viewWillDraw");
    }

    static void dealloc(id self, SEL) {
      std::cout << "view..dealloc " << self << std::endl;

      if (auto* p = get_pointer(self)) {
        p->on_dealloc();
      }

      send_superclass_message<void>(self, "dealloc");
    }

    static void set_pointer(id self, ClassType* owner) { nano::set_ivar(self, ClassType::valueName, owner); }

    static ClassType* get_pointer(id self) { return nano::get_ivar<ClassType*>(self, ClassType::valueName); }
  };

  static ClassObject classObject;
};
//
//// native_view::Native::ClassObject native_view::Native::classObject = native_view::Native::ClassObject {};

NANO_CLANG_DIAGNOSTIC_PUSH()
NANO_CLANG_DIAGNOSTIC(ignored, "-Wexit-time-destructors")
NANO_CLANG_DIAGNOSTIC(ignored, "-Wglobal-constructors")
view::pimpl::ClassObject view::pimpl::classObject{};
NANO_CLANG_DIAGNOSTIC_POP()

// cview_core::responder::~responder() {}

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
    constexpr unsigned long uiNSViewWidthSizable = 2;
    constexpr unsigned long uiNSViewHeightSizable = 16;
    nano::call<void, unsigned long>(
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

    nano::call(to_objc_id(get_native_handle()), "removeFromSuperview");

    //    if (responder* d = m_pimpl->m_parent->m_pimpl->m_responder) {
    m_pimpl->m_parent->on_did_remove_subview(this);
    //    }
  }
  else {
    nano::call(to_objc_id(get_native_handle()), "removeFromSuperview");
  }
}

void view::initialize() { m_pimpl->initialize(); }

void view::set_auto_resize() {

  constexpr unsigned long uiNSViewWidthSizable = 2;
  constexpr unsigned long uiNSViewHeightSizable = 16;
  nano::call<void, unsigned long>(m_pimpl->m_obj, "setAutoresizingMask:", uiNSViewWidthSizable | uiNSViewHeightSizable);
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

// bool cview_core::set_responder(responder* d) { return m_pimpl->set_responder(d); }

//
//
//

nano::rect<int> get_native_view_bounds(nano::native_view_handle native_view) {
  return nano::rect<int>(nano::call<CGRect>(reinterpret_cast<id>(native_view), "bounds"));
}

//
//
//
// cview::cview(window_flags flags)
//    : cview_core(flags) {
////  set_responder(this);
//}
//
// cview::cview(cview_core* parent, const nano::rect<int>& rect)
//    : cview_core(parent, rect) {
////  set_responder(this);
//}
//
// cview::cview(native_view_handle parent, const nano::rect<int>& rect, view_flags flags)
//    : cview_core(parent, rect, flags) {
////  set_responder(this);
//}
//
// cview::~cview() {}

window::window(window_flags flags)
    : view(flags)
    , window_proxy(get_window_proxy()) {}

window::~window() {}

class webview::native {
public:
  native(webview* wview)
      : m_view(wview) {}

  ~native() {
    if (m_web_view) {
      nano::objc_release(m_web_view);
      m_web_view = nullptr;
    }
  }

  void load(const std::string& path) {
    CFStringRef url_str = CFStringCreateWithCString(kCFAllocatorDefault, path.c_str(), kCFStringEncodingUTF8);
    CFURLRef url = CFURLCreateWithString(kCFAllocatorDefault, url_str, nullptr);
    CFRelease(url_str);

    //    id request = class_createInstance(objc_getClass("NSURLRequest"), 0);
    //    nano::call<void, CFURLRef>(request, nano::get_selector("initWithURL:"), url);

    id request = nano::create_objc_object_ref<void, CFURLRef>("NSURLRequest", "initWithURL:", url);
    CFRelease(url);

    [[maybe_unused]] id wk_navigation = nano::call<id, id>(m_web_view, nano::get_selector("loadRequest:"), request);

    nano::objc_release(request);
  }

  void create_web_view(const nano::rect<int>& rect) {

    if (m_web_view) {
      return;
    }

    id conf = class_createInstance(objc_getClass("WKWebViewConfiguration"), 0);
    nano::call<void>(conf, nano::get_selector("init"));

    m_web_view = nano::create_objc_object_ref<void, CGRect, id>(
        "WKWebView", "initWithFrame:configuration:", static_cast<CGRect>(rect), conf);

    //    m_web_view = class_createInstance(objc_getClass("WKWebView"), 0);
    //    nano::call<void, CGRect, id>(
    //        m_web_view, nano::get_selector("initWithFrame:configuration:"), static_cast<CGRect>(rect), conf);

    nano::objc_release(conf);

    nano::icall(m_view->get_native_handle(), "addSubview:", m_web_view);

    load("https://www.facebook.com");
  }

  webview* m_view;
  id m_web_view = nullptr;
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
  nano::call<void, CGRect>(m_native->m_web_view, "setFrame:", static_cast<CGRect>(rect));
}

void webview::load(const std::string& path) { m_native->load(path); }

class application::native {
public:
  static constexpr const char* className = "UIApplicationNativeDelegate";
  static constexpr const char* baseName = "NSObject";
  static constexpr const char* valueName = "owner";

  native(application* app)
      : m_app(app) {
    m_obj = classObject.create_instance();
    nano::call(m_obj, "init");
    app_delegate_class_object::set_pointer(m_obj, this);

    objc_object* sharedApplication = nano::get_class_property("NSApplication", "sharedApplication");
    nano::call<void, objc_object*>(sharedApplication, "setDelegate:", m_obj);
  }

  ~native() {
    if (m_obj) {
      objc_object* sharedApplication = nano::get_class_property("NSApplication", "sharedApplication");
      nano::call<void, objc_object*>(sharedApplication, "setDelegate:", nullptr);
      nano::objc_reset(m_obj);
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

  BOOL application_should_terminate_after_last_window_closed([[maybe_unused]] objc_object* notification) {
    std::cout << "application_should_terminate_after_last_window_closed" << std::endl;
    return true;
  }

  unsigned long application_should_terminate([[maybe_unused]] id notification) {

    if (m_app->should_terminate()) {
      // NSTerminateNow = 1
      return 1;
    }

    // NSTerminateCancel = 0
    return 0;
  }

  //    static BOOL application_should_terminate_after_last_window_closed(id self, SEL, id notification) {
  //      if (auto* p = get_pointer(self)) {
  //        return p->application_should_terminate_after_last_window_closed(notification);
  //      }
  //
  //      return TRUE;
  //    }

  application* m_app;
  id m_obj;
  std::vector<std::string> m_args;

private:
  class app_delegate_class_object : public nano::objc_class<native> {
  public:
    using ClassType = native;

    app_delegate_class_object()
        : objc_class("app_delegate_class_object") {
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

      //      register_class();
    }

    static void set_pointer(id self, ClassType* owner) { nano::set_ivar(self, ClassType::valueName, owner); }

    static ClassType* get_pointer(id self) { return nano::get_ivar<ClassType*>(self, ClassType::valueName); }

    //    static unsigned long application_should_terminate(id self, SEL, id notification) {
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

    //    static BOOL application_should_terminate_after_last_window_closed(id self, SEL, id notification) {
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
  objc_object* sharedApplication = nano::get_class_property("NSApplication", "sharedApplication");
  nano::call<void, objc_object*>(sharedApplication, "terminate:", nullptr);
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
