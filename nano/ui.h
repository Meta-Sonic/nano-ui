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

#pragma once

/*!
 * @file      ui.h
 * @brief     nano core
 * @copyright Copyright (C) 2022, Meta-Sonic
 * @author    Alexandre Arsenault alx.arsenault@gmail.com
 * @date      Created 16/06/2022
 */

#include <nano/graphics.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <unordered_map>
#include <vector>

//
// MARK: - Macros -
//

#ifdef _MSVC_LANG
  #define NANO_CPP_VERSION _MSVC_LANG
#else
  #define NANO_CPP_VERSION __cplusplus
#endif

#if NANO_CPP_VERSION >= 202002L
  #define NANO_CPP_20
  #define NANO_IF_CPP_20(X) X
#else
  #define NANO_IF_CPP_20(X)
#endif

#if NANO_CPP_VERSION >= 201703L
  #define NANO_CPP_17
  #define NANO_IF_CPP_17(X) X
#else
  #define NANO_IF_CPP_17(X)
#endif

#define NANO_INLINE inline

#define NANO_CXPR constexpr

#define NANO_NOEXCEPT noexcept

#define NANO_NODISCARD [[nodiscard]]

/// @def NANO_INLINE_CXPR
#define NANO_INLINE_CXPR NANO_INLINE NANO_CXPR

/// @def NANO_NODC_INLINE
#define NANO_NODC_INLINE NANO_NODISCARD inline

/// @def NANO_NODC_INLINE_CXPR
#define NANO_NODC_INLINE_CXPR NANO_NODISCARD NANO_INLINE_CXPR

#define NANO_UNUSED(x) (void)x

/// @def NANO_LIKELY
#define NANO_LIKELY(EXPR) NANO_LIKELY_IMPL(EXPR)

/// @def NANO_UNLIKELY
#define NANO_UNLIKELY(EXPR) NANO_UNLIKELY_IMPL(EXPR)

/// @def NANO_DEBUGTRAP
/// On compilers which support it, expands to an expression which causes the
/// program to break while running under a debugger.
#define NANO_DEBUGTRAP() NANO_DEBUGTRAP_IMPL()

/// @def NANO_ASSERT
#define NANO_ASSERT(Expr, Msg) NANO_ASSERT_IMPL(Expr, Msg)

/// @def NANO_ERROR
#define NANO_ERROR(Msg) NANO_ERROR_IMPL(Msg)

#define NANO_STRINGIFY(X) NANO_STR(X)
#define NANO_STR(X) #X

#ifdef _MSC_VER
  #define NANO_MSVC_PRAGMA(X) __pragma(X)
#else
  #define NANO_MSVC_PRAGMA(X)
#endif

#ifdef __clang__
  #define NANO_CLANG_PRAGMA(X) _Pragma(X)
#else
  #define NANO_CLANG_PRAGMA(X)
#endif

#define NANO_MSVC_DIAGNOSTIC_PUSH() NANO_MSVC_PRAGMA(warning(push))
#define NANO_MSVC_DIAGNOSTIC_POP() NANO_MSVC_PRAGMA(warning(pop))
#define NANO_MSVC_PUSH_WARNING(X) NANO_MSVC_DIAGNOSTIC_PUSH() NANO_MSVC_PRAGMA(warning(disable : X))
#define NANO_MSVC_POP_WARNING() NANO_MSVC_DIAGNOSTIC_POP()

#define NANO_CLANG_DIAGNOSTIC_PUSH() NANO_CLANG_PRAGMA("clang diagnostic push")
#define NANO_CLANG_DIAGNOSTIC_POP() NANO_CLANG_PRAGMA("clang diagnostic pop")
#define NANO_CLANG_DIAGNOSTIC(TYPE, X) NANO_CLANG_PRAGMA(NANO_STRINGIFY(clang diagnostic TYPE X))
#define NANO_CLANG_PUSH_WARNING(X)                                                                                     \
  NANO_CLANG_DIAGNOSTIC_PUSH() NANO_CLANG_PRAGMA(NANO_STRINGIFY(clang diagnostic ignored X))
#define NANO_CLANG_POP_WARNING() NANO_CLANG_DIAGNOSTIC_POP()

#define NANO_USING_TYPE(x)                                                                                             \
  template <class T>                                                                                                   \
  using x = decltype(T::x)

#define NANO_ENUM_CLASS_FLAGS(enum_class)                                                                              \
  inline enum_class operator|(enum_class lhs, enum_class rhs) {                                                        \
    using type = std::underlying_type_t<enum_class>;                                                                   \
    return static_cast<enum_class>(static_cast<type>(lhs) | static_cast<type>(rhs));                                   \
  }                                                                                                                    \
                                                                                                                       \
  inline enum_class operator&(enum_class lhs, enum_class rhs) {                                                        \
    using type = std::underlying_type_t<enum_class>;                                                                   \
    return static_cast<enum_class>(static_cast<type>(lhs) & static_cast<type>(rhs));                                   \
  }                                                                                                                    \
                                                                                                                       \
  inline enum_class& operator|=(enum_class& lhs, enum_class rhs) { return lhs = (lhs | rhs); }                         \
  inline enum_class& operator&=(enum_class& lhs, enum_class rhs) { return lhs = (lhs & rhs); }                         \
                                                                                                                       \
  inline enum_class operator~(const enum_class& lhs) {                                                                 \
    using type = std::underlying_type_t<enum_class>;                                                                   \
    return static_cast<enum_class>(~type(lhs));                                                                        \
  }                                                                                                                    \
                                                                                                                       \
  inline bool operator==(enum_class lhs, std::underlying_type_t<enum_class> value) {                                   \
    return std::underlying_type_t<enum_class>(lhs) == value;                                                           \
  }                                                                                                                    \
                                                                                                                       \
  inline bool operator!=(enum_class lhs, std::underlying_type_t<enum_class> value) {                                   \
    return std::underlying_type_t<enum_class>(lhs) != value;                                                           \
  }

NANO_CLANG_DIAGNOSTIC_PUSH()
NANO_CLANG_DIAGNOSTIC(warning, "-Weverything")
NANO_CLANG_DIAGNOSTIC(ignored, "-Wc++98-compat")

namespace nano {
enum class text_alignment { left, center, right };

/// https://developer.apple.com/documentation/quartzcore/cashapelayer/1521905-linecap?language=objc
enum class line_join {
  /// The edges of the adjacent line segments are continued to meet at a sharp point.
  miter,

  /// A join with a rounded end. Core Graphics draws the line to extend beyond
  /// the endpoint of the path. The line ends with a semicircular arc with a
  /// radius of 1/2 the line’s width, centered on the endpoint.
  round,

  /// A join with a squared-off end. Core Graphics draws the line to extend beyond
  /// the endpoint of the path, for a distance of 1/2 the line’s width.
  bevel

};

/// https://developer.apple.com/documentation/quartzcore/cashapelayer/1522147-linejoin?language=objc
enum class line_cap {
  /// A line with a squared-off end. Core Graphics draws the line to
  /// extend only to the exact endpoint of the path.
  /// This is the default.
  butt,

  /// A line with a rounded end. Core Graphics draws the line to extend beyond the
  /// endpoint of the path. The line ends with a semicircular arc with a radius
  /// of 1/2 the line’s width, centered on the endpoint.
  round,

  /// A line with a squared-off end. Core Graphics extends the line beyond the
  /// endpoint of the path for a distance equal to half the line width.
  square

};

///
///
///
class font {
public:
  struct filepath_tag {};

  font() = default;

  ///
  font(const char* font_name, double font_size);

  ///
  font(const char* filepath, double font_size, filepath_tag);

  ///
  font(const std::uint8_t* data, std::size_t data_size, double font_size);

  font(const font& f);
  font(font&& f);

  ~font();

  font& operator=(const font& f);

  font& operator=(font&& f);

  ///
  bool is_valid() const noexcept;

  ///
  inline explicit operator bool() const noexcept { return is_valid(); }

  ///
  double get_height() const noexcept;

  ///
  double get_font_size() const noexcept { return m_font_size; }

  ///
  float get_string_width(std::string_view text) const;

  native_font_ref get_native_font() noexcept;
  native_font_ref get_native_font() const noexcept;

  struct native;

private:
  native* m_native = nullptr;
  double m_font_size;
};

///
///
///
class graphic_context {
public:
  class native;

  class scoped_state;

  graphic_context(native_graphic_context_ref nc);

  graphic_context(const graphic_context&) = delete;
  graphic_context(graphic_context&&) = delete;

  ~graphic_context();

  graphic_context& operator=(const graphic_context&) = delete;
  graphic_context& operator=(graphic_context&&) = delete;

  /// CTM (current transformation matrix)
  /// clip region
  /// image interpolation quality
  /// line width
  /// line join
  /// miter limit
  /// line cap
  /// line dash
  /// flatness
  /// should anti-alias
  /// rendering intent
  /// fill color space
  /// stroke color space
  /// fill color
  /// stroke color
  /// alpha value
  /// font
  /// font size
  /// character spacing
  /// text drawing mode
  /// shadow parameters
  /// the pattern phase
  /// the font smoothing parameter
  /// blend mode
  void save_state();
  void restore_state();

  void begin_transparent_layer(float alpha);
  void end_transparent_layer();

  void translate(const nano::point<float>& pos);

  void clip();
  void clip_even_odd();
  void reset_clip();
  void clip_to_rect(const nano::rect<float>& rect);
  // void clip_to_path(const nano::path& p);
  // void clip_to_path_even_odd(const nano::path& p);
  void clip_to_mask(const nano::image& img, const nano::rect<float>& rect);

  void begin_path();
  void close_path();
  void add_rect(const nano::rect<float>& rect);
  // void add_path(const nano::path& p);

  nano::rect<float> get_clipping_rect() const;

  void set_line_width(float width);
  void set_line_join(line_join lj);
  void set_line_cap(line_cap lc);
  void set_line_style(float width, line_join lj, line_cap lc);

  void set_fill_color(const nano::color& c);
  void set_stroke_color(const nano::color& c);

  void fill_rect(const nano::rect<float>& r);
  void stroke_rect(const nano::rect<float>& r);
  void stroke_rect(const nano::rect<float>& r, float line_width);

  void stroke_line(const nano::point<float>& p0, const nano::point<float>& p1);

  void fill_ellipse(const nano::rect<float>& r);
  void stroke_ellipse(const nano::rect<float>& r);

  void fill_rounded_rect(const nano::rect<float>& r, float radius);

  void stroke_rounded_rect(const nano::rect<float>& r, float radius);
  // void fill_quad(const nano::quad& q);
  // void stroke_quad(const nano::quad& q);

  // void fill_path(const nano::path& p);
  // void fill_path(const nano::path& p, const nano::rect<float>& rect);
  // void fill_path_with_shadow(
  // const nano::path& p, float blur, const nano::color& shadow_color, const nano::size<float>& offset);

  // void stroke_path(const nano::path& p);

  void draw_image(const nano::image& img, const nano::point<float>& pos);
  void draw_image(const nano::image& img, const nano::rect<float>& r);
  void draw_image(const nano::image& img, const nano::rect<float>& r, const nano::rect<float>& clip_rect);

  void draw_sub_image(const nano::image& img, const nano::rect<float>& r, const nano::rect<float>& img_rect);

  void draw_text(const nano::font& f, const std::string& text, const nano::point<float>& pos);

  void draw_text(
      const nano::font& f, const std::string& text, const nano::rect<float>& rect, nano::text_alignment alignment);

  // void set_shadow(float blur, const nano::color& shadow_color, const nano::size<float>& offset);

  native_graphic_context_ref get_native_handle() const;

private:
  native* m_native;
};

//----------------------------------------------------------------------------------------------------------------------
// MARK: - UI -
//----------------------------------------------------------------------------------------------------------------------

/// Native view handle.
/// NSView* on mac.
/// HWND on windows.
typedef struct native_view_handle_type* native_view_handle;

/// Native window handle.
/// NSWindow* on mac.
/// HWND on windows.
typedef struct native_window_handle_type* native_window_handle;

/// Native display handle.
typedef struct native_display_handle_type* native_display_handle;

/// Native event handle.
/// NSEvent* on mac.
/// nullptr on windows.
typedef struct native_event_handle_type* native_event_handle;

/// Native menu handle.
typedef struct native_menu_handle_type* native_menu_handle;

///
class view;

///
enum class window_flags {
  border_less = 0,
  titled = 1 << 0,
  closable = 1 << 1,
  minimizable = 1 << 2,
  maximizable = 1 << 3,
  resizable = 1 << 4,
  panel = 1 << 5,
  full_size_content_view = 1 << 6,
  default_flags = titled | closable | minimizable | maximizable | resizable
};

NANO_ENUM_CLASS_FLAGS(window_flags)

enum class view_flags { none = 0, auto_resize = 1 << 0, default_flags = none };

NANO_ENUM_CLASS_FLAGS(view_flags)

enum class event_type : std::uint64_t {
  none,

  // mouse events.

  left_mouse_down,
  left_mouse_up,
  right_mouse_down,
  right_mouse_up,
  mouse_moved,
  left_mouse_dragged,
  right_mouse_dragged,

  mouse_entered,
  mouse_exited,

  // specialized control devices.

  scroll_wheel,
  tablet_pointer,
  tablet_proximity,
  other_mouse_down,
  other_mouse_up,
  other_mouse_dragged,

  // keyboard events.

  key_down,
  key_up,
  key_flags_changed
};

inline bool is_mouse_event(event_type type) noexcept;

inline bool is_scroll_or_drag_event(event_type type) noexcept;

inline bool is_click_event(event_type type) noexcept;

inline bool is_key_event(event_type type) noexcept;

enum class event_modifiers : std::uint64_t {
  none = 0,
  left_mouse_down = 1 << 0,
  middle_mouse_down = 1 << 1,
  right_mouse_down = 1 << 2,
  command = 1 << 3,
  shift = 1 << 4,
  control = 1 << 5,
  alt = 1 << 6,
  function = 1 << 7
};

NANO_ENUM_CLASS_FLAGS(event_modifiers)

class event {
public:
  event(native_event_handle handle, nano::view* view);

  //  event(native_event_handle handle, nano::cview_core* view, event_type type, const nano::point<float>& pos,
  //      event_modifiers mods, std::int64_t click_count);

  event(event&&) = delete;
  event(const event&) = delete;

  ~event() = default;

  event& operator=(event&&) = delete;
  event& operator=(const event&) = delete;

  inline native_event_handle get_native_handle() const noexcept;

  inline nano::view* get_view() const noexcept;

  native_window_handle get_native_window() const noexcept;

  inline event_type get_event_type() const noexcept;

  inline bool is_mouse_event() const noexcept;

  inline bool is_scroll_or_drag_event() const noexcept;

  inline bool is_click_event() const noexcept;

  inline bool is_key_event() const noexcept;

  /// the time when the event occurred in nanoseconds since system startup.
  inline std::uint64_t get_timestamp() const noexcept;

  // MARK: mouse events

  const nano::point<float> get_window_position() const noexcept;

  /// the position of the mouse when the event occurred.
  /// @details this position is relative to the top-left of the view to which
  /// the event applies (as returned by get_view()).
  inline const nano::point<float>& get_position() const noexcept;

  inline const nano::point<float>& get_click_position() const noexcept;

  const nano::point<float> get_bounds_position() const noexcept;

  nano::point<float> get_screen_position() const noexcept;

  /// for a click event, the number of times the mouse was clicked in
  /// succession.
  inline std::int64_t get_click_count() const noexcept;

  inline bool is_left_button_down() const noexcept;

  inline bool is_middle_button_down() const noexcept;

  inline bool is_right_button_down() const noexcept;

  inline bool is_command_down() const noexcept;

  inline bool is_shift_down() const noexcept;

  inline bool is_ctrl_down() const noexcept;

  inline bool is_alt_down() const noexcept;

  inline bool is_function_down() const noexcept;

  // MARK: scroll events

  inline const nano::point<float>& get_wheel_delta() const noexcept;

  // MARK: common

  inline event_modifiers get_modifiers() const noexcept;
  
  std::u16string get_key() const noexcept;

private:
  native_event_handle m_native_handle = nullptr;
  nano::view* m_view = nullptr;

  std::uint64_t m_timestamp = 0;

  nano::point<float> m_position = { 0, 0 };
  nano::point<float> m_click_position = { 0, 0 };
  nano::point<float> m_wheel_delta = { 0, 0 };

  event_type m_type = event_type::none;
  event_modifiers m_event_modifiers = event_modifiers::none;
  std::int64_t m_click_count = 0;
  
//  std::string m_key;
  //  int m_reserved = 0;

  event() = default;
};

class cwindow;
class window;

class window_proxy {
public:
  class delegate {
  public:
    inline delegate() = default;

    virtual ~delegate() = default;

    virtual void window_did_miniaturize(view*) {}
    virtual void window_did_deminiaturize(view*) {}

    virtual void window_did_enter_full_screen(view*) {}
    virtual void window_did_exit_full_screen(view*) {}

    virtual void window_did_become_key(view*) {}
    virtual void window_did_resign_key(view*) {}

    virtual void window_did_change_screen(view*) {}

    virtual void window_will_close(view*) {}

    virtual bool window_should_close(view*) { return true; }
  };

  void set_window_frame(const nano::rect<int>& rect);

  native_window_handle get_native_handle() const;

  void set_title(std::string_view title);
  nano::rect<int> get_frame() const;

  void set_flags(window_flags flags);

  void set_document_edited(bool dirty);

  void close();

  void center();

  void set_shadow(bool visible);

  void set_window_delegate(delegate* d);

  inline view* get_view() { return &m_view; }

  inline const view* get_view() const { return &m_view; }

private:
  view& m_view;
  friend class view;
  friend class cwindow;
  friend class cwindow;
  friend class window;

  window_proxy(view& v)
      : m_view(v) {}

  window_proxy(const window_proxy&) = delete;
  window_proxy(window_proxy&&) = default;

  window_proxy& operator=(const window_proxy&) = delete;
  window_proxy& operator=(window_proxy&&) = delete;
};

///
///
///
class view {
public:
  class pimpl;

  /// creates a window.
  view(window_flags flags);

  /// add view to parent.
  view(view* parent, const nano::rect<int>& rect);

  /// create view from platform native view.
  view(native_view_handle parent, const nano::rect<int>& rect, view_flags flags = view_flags::default_flags);

  virtual ~view();

  native_view_handle get_native_handle() const;

  // MARK: geometry

  /// changing the frame does not mark the view as needing to be displayed.
  /// call redraw() when you want the view to be redisplayed.
  void set_frame(const nano::rect<int>& rect);
  void set_frame_position(const nano::point<int>& pos);
  void set_frame_size(const nano::size<int>& size);

  nano::rect<int> get_frame() const;
  nano::point<int> get_frame_position() const;
  nano::size<int> get_frame_size() const;

  nano::point<int> get_position_in_window() const;
  nano::point<int> get_position_in_screen() const;

  nano::rect<int> get_bounds() const;

  nano::rect<int> get_visible_rect() const;

  /// converts a point from the coordinate system of a given view to that of the view.
  /// @param point a point specifying a location in the coordinate system of a_view.
  /// @param view the view with a_point in its coordinate system. both a_view and the view
  ///             must belong to the same ns_window object, and that window must not be nil.
  ///             if a_view is nil, this method converts from window coordinates instead.
  ///
  /// @returns the point converted to the coordinate system of the view.
  nano::point<int> convert_from_view(const nano::point<int>& point, nano::view* view) const;

  nano::point<int> convert_to_view(const nano::point<int>& point, nano::view* view) const;

  void set_hidden(bool hidden);
  bool is_hidden() const;

  inline void set_visible(bool visible);
  inline bool is_visible() const;

  bool is_focused() const;
  void focus();
  void unfocus();

  // MARK: window

  bool is_window() const;

  inline window_proxy get_window_proxy();

  // MARK: node

  view* get_parent() const;

  // MARK: painting

  /// marks the viewr’s entire bounds rectangle as needing to be redrawn.
  ///
  /// @details you can use this method or the redraw(const nano::rect&) to notify
  /// the system
  ///          that your view’s contents need to be redrawn. this method makes a
  ///          note of the request and returns immediately. the view is not
  ///          actually redrawn until the next drawing cycle, at which point all
  ///          invalidated views are updated.
  void redraw();

  /// marks the region of the view within the specified rectangle as needing
  /// display, increasing the view’s existing invalid region to include it.
  void redraw(const nano::rect<int>& rect);

  //  bool set_responder(responder* d);

  ///
  void set_auto_resize();

protected:
  /// returns true if the specified rectangle intersects any part of the area
  /// that the view is being asked to draw.
  ///
  /// @details this only works from within the on_draw() method.
  ///          it gives you a convenient way to determine whether any part of a
  ///          given graphical entity might need to be drawn. it is optimized to
  ///          efficiently reject any rectangle that lies outside the bounding
  ///          box of the area that the view is being asked to draw in on_draw().
  bool is_dirty_rect(const nano::rect<int>& rect) const;

  virtual void on_frame_changed() {}

  virtual void on_show() {}
  virtual void on_hide() {}

  virtual void on_focus() {}
  virtual void on_unfocus() {}

  virtual void on_did_add_subview(view* view) { NANO_UNUSED(view); }
  virtual void on_did_remove_subview(view* view) { NANO_UNUSED(view); }

  virtual void on_mouse_down(const nano::event& evt) { NANO_UNUSED(evt); }
  virtual void on_mouse_up(const nano::event& evt) { NANO_UNUSED(evt); }
  virtual void on_mouse_dragged(const nano::event& evt) { NANO_UNUSED(evt); }

  virtual void on_right_mouse_down(const nano::event& evt) { NANO_UNUSED(evt); }
  virtual void on_right_mouse_up(const nano::event& evt) { NANO_UNUSED(evt); }
  virtual void on_right_mouse_dragged(const nano::event& evt) { NANO_UNUSED(evt); }

  virtual void on_other_mouse_down(const nano::event& evt) { NANO_UNUSED(evt); }
  virtual void on_other_mouse_up(const nano::event& evt) { NANO_UNUSED(evt); }
  virtual void on_other_mouse_dragged(const nano::event& evt) { NANO_UNUSED(evt); }

  virtual void on_mouse_moved(const nano::event& evt) { NANO_UNUSED(evt); }
  virtual void on_mouse_entered(const nano::event& evt) { NANO_UNUSED(evt); }
  virtual void on_mouse_exited(const nano::event& evt) { NANO_UNUSED(evt); }

  virtual void on_scroll_wheel(const nano::event& evt) { NANO_UNUSED(evt); }

  virtual void on_key_down(const nano::event& evt) { NANO_UNUSED(evt); }
  virtual void on_key_up(const nano::event& evt) { NANO_UNUSED(evt); }
  virtual void on_key_flags_changed(const nano::event& evt) { NANO_UNUSED(evt); }

  virtual void on_will_draw() {
  }
  
  virtual void on_draw(nano::graphic_context& gc, const nano::rect<float>& dirtyRect) {
    NANO_UNUSED(gc);
    NANO_UNUSED(dirtyRect);
  }

private:
  std::unique_ptr<pimpl> m_pimpl;

  void initialize();

  friend class window_proxy;
  friend nano::rect<int> get_native_view_bounds(nano::native_view_handle);
};

nano::rect<int> get_native_view_bounds(nano::native_view_handle native_view);

class window : public view, public window_proxy {
public:
  /// creates a window.
  window(window_flags flags);

  ~window() override;

private:
  using view::view;
};

///
class webview : public view {
public:
  webview(view* parent, const nano::rect<int>& rect);

  virtual ~webview() override;

  void set_wframe(const nano::rect<int>& rect);

  void load(const std::string& path);

  class native;

private:
  native* m_native;
};

#ifdef WIN32
  #define NANO_APPLICATION_MAIN_ARGS 0, nullptr
  #define UIApplicationMain()                                                                                          \
    __stdcall WinMain(struct HINSTANCE__* hInstance, struct HINSTANCE__* hPrevInstance, char* lpCmdLine, int nCmdShow)
#else
  #define NANO_APPLICATION_MAIN_ARGS argc, argv
  #define UIApplicationMain() main(int argc, const char* argv[])
#endif

class application {
public:
  class native;

  template <class AppType, typename... Args>
  static inline std::unique_ptr<AppType> create_application(int argc, const char* argv[], Args&&... args) {
    std::unique_ptr<AppType> app(new AppType(std::forward<Args>(args)...));
    initialize_application(app.get(), argc, argv);
    return app;
  }

  application();

  virtual ~application();

  /// returns the application's name.
  virtual std::string get_application_name() const = 0;

  /// returns the application's version number.
  virtual std::string get_application_version() const = 0;

  int run();

  /// signals that the main message loop should stop and the application should
  /// terminate.
  ///
  /// @details this isn't synchronous, it just posts a quit message to the main
  /// queue,
  ///          and when this message arrives, the message loop will stop, the
  ///          shutdown() method will be called, and the app will exit
  static void quit();

  /// returns the application's command line arguments as a single string.
  std::string get_command_line_arguments() const;

  /// returns the application's command line arguments.
  std::vector<std::string> get_command_line_arguments_array() const;

protected:
  /// called before the event loop and before any ui related stuff.
  ///
  /// @details use this to initialize any non graphics related elements.
  virtual void prepare() {}

  /// called when the application starts.
  ///
  /// @details this will be called once to let the application do whatever
  /// initialisation
  ///          it needs, create its windows, etc.
  ///
  ///          after the method returns, the normal event-dispatch loop will be
  ///          run, until the quit() method is called, at which point the
  ///          shutdown() method will be called to let the application clear up
  ///          anything it needs to delete.
  virtual void initialise() {}

  /// this method is called when the application is being put into background
  /// mode by the operating system.
  virtual void suspended() {}

  /// this method is called when the application is being woken from background
  /// mode by the operating system.
  virtual void resumed() {}

  virtual void shutdown() {}

  /// called when the operating system is trying to close the application.
  virtual bool should_terminate() { return true; }

private:
  std::unique_ptr<native> m_native;

  static void initialize_application(application* app, int argc, const char* argv[]);
};
} // namespace nano.

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//
//
//
//
// MARK: - IMPLEMENTATION -
//
//
//
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

// MARK: - Macros -

#ifndef __has_feature
  #define __has_feature(x) 0
#endif

#ifndef __has_builtin
  #define __has_builtin(x) 0
#endif

#if __has_builtin(__builtin_expect)
  #define NANO_LIKELY_IMPL(EXPR) __builtin_expect(static_cast<bool>(EXPR), true)
  #define NANO_UNLIKELY_IMPL(EXPR) __builtin_expect(static_cast<bool>(EXPR), false)
#else
  #define NANO_LIKELY_IMPL(EXPR) (EXPR)
  #define NANO_UNLIKELY_IMPL(EXPR) (EXPR)
#endif

#if __has_builtin(__builtin_debugtrap)
  #define NANO_DEBUGTRAP_IMPL() __builtin_debugtrap()

#elif defined(_WIN32)
  // The __debugbreak intrinsic is supported by MSVC and breaks while
  // running under the debugger, and also supports invoking a debugger
  // when the OS is configured appropriately.
  #define NANO_DEBUGTRAP_IMPL() __debugbreak()

#elif defined(__clang__) && (defined(unix) || defined(__unix__) || defined(__unix) || defined(__MACH__))
  #include <signal.h>
  #define NANO_DEBUGTRAP_IMPL() raise(SIGTRAP)

#else
  #define NANO_DEBUGTRAP_IMPL() std::abort()
#endif

#ifdef NDEBUG
  #define NANO_ASSERT_IMPL(Expr, Msg)
  #define NANO_ERROR_IMPL(Msg)

#else
  #define NANO_ASSERT_IMPL(Expr, Msg) nano::AssertDetail::CustomAssert(#Expr, Expr, __FILE__, __LINE__, Msg)
  #define NANO_ERROR_IMPL(Msg) nano::AssertDetail::CustomError(__FILE__, __LINE__, Msg)

namespace nano {
namespace AssertDetail {
  NANO_INLINE void CustomAssert(const char* expr_str, bool expr, const char* file, int line, const char* msg);
  NANO_INLINE void CustomError(const char* file, int line, const char* msg);
} // namespace AssertDetail.
} // namespace nano.
#endif // NDEBUG.

namespace nano {

//
// MARK: - point -
//

// template <typename T>
// NANO_INLINE_CXPR point<T>::point(value_type X, value_type Y) NANO_NOEXCEPT : x(X), y(Y) {}

/// Construct a Point from PointType with member x and y.
template <typename T>
template <typename PointType, if_members<PointType, meta::x, meta::y>>
NANO_INLINE_CXPR point<T>::point(const PointType& point) NANO_NOEXCEPT : x(static_cast<value_type>(point.x)),
                                                                         y(static_cast<value_type>(point.y)) {}

/// Construct a Point from PointType with member X and Y.
template <typename T>
template <typename PointType, if_members<PointType, meta::X, meta::Y>>
NANO_INLINE_CXPR point<T>::point(const PointType& point) NANO_NOEXCEPT : x(static_cast<value_type>(point.X)),
                                                                         y(static_cast<value_type>(point.Y)) {}

template <typename T>
NANO_INLINE_CXPR point<T> point<T>::operator+(value_type v) const NANO_NOEXCEPT {
  return point(x + v, y + v);
}

template <typename T>
NANO_INLINE_CXPR point<T> point<T>::operator-(value_type v) const NANO_NOEXCEPT {
  return point(x - v, y - v);
}

template <typename T>
NANO_INLINE_CXPR point<T> point<T>::operator*(value_type v) const NANO_NOEXCEPT {
  return point(x * v, y * v);
}

template <typename T>
NANO_INLINE_CXPR point<T> point<T>::operator/(value_type v) const NANO_NOEXCEPT {
  return point(x / v, y / v);
}

template <typename T>
NANO_INLINE_CXPR point<T> point<T>::operator+(const point& pt) const NANO_NOEXCEPT {
  return point(x + pt.x, y + pt.y);
}

template <typename T>
NANO_INLINE_CXPR point<T> point<T>::operator-(const point& pt) const NANO_NOEXCEPT {
  return point(x - pt.x, y - pt.y);
}

template <typename T>
NANO_INLINE_CXPR point<T> point<T>::operator*(const point& pt) const NANO_NOEXCEPT {
  return point(x * pt.x, y * pt.y);
}

template <typename T>
NANO_INLINE_CXPR point<T> point<T>::operator/(const point& pt) const NANO_NOEXCEPT {
  return point(x / pt.x, y / pt.y);
}

template <typename T>
NANO_INLINE_CXPR point<T>& point<T>::operator+=(value_type v) NANO_NOEXCEPT {
  return *this = (*this + v);
}

template <typename T>
NANO_INLINE_CXPR point<T>& point<T>::operator-=(value_type v) NANO_NOEXCEPT {
  return *this = (*this - v);
}

template <typename T>
NANO_INLINE_CXPR point<T>& point<T>::operator*=(value_type v) NANO_NOEXCEPT {
  return *this = (*this * v);
}

template <typename T>
NANO_INLINE_CXPR point<T>& point<T>::operator/=(value_type v) NANO_NOEXCEPT {
  return *this = (*this / v);
}

template <typename T>
NANO_INLINE_CXPR point<T>& point<T>::operator+=(const point& pt) NANO_NOEXCEPT {
  return *this = (*this + pt);
}

template <typename T>
NANO_INLINE_CXPR point<T>& point<T>::operator-=(const point& pt) NANO_NOEXCEPT {
  return *this = (*this - pt);
}

template <typename T>
NANO_INLINE_CXPR point<T>& point<T>::operator*=(const point& pt) NANO_NOEXCEPT {
  return *this = (*this * pt);
}

template <typename T>
NANO_INLINE_CXPR point<T>& point<T>::operator/=(const point& pt) NANO_NOEXCEPT {
  return *this = (*this / pt);
}

template <typename T>
NANO_INLINE_CXPR bool point<T>::operator==(const point& pt) const NANO_NOEXCEPT {
  if constexpr (std::is_floating_point_v<T>) {
    return nano::fcompare(x, pt.x) && nano::fcompare(y, pt.y);
  }
  else {
    return x == pt.x && y == pt.y;
  }
}

template <typename T>
NANO_INLINE_CXPR bool point<T>::operator!=(const point& pt) const NANO_NOEXCEPT {
  return !operator==(pt);
}

template <typename T>
NANO_INLINE_CXPR bool point<T>::operator<(const point& pt) const NANO_NOEXCEPT {
  return (x < pt.x && y < pt.y);
}
template <typename T>
NANO_INLINE_CXPR bool point<T>::operator<=(const point& pt) const NANO_NOEXCEPT {
  return (x <= pt.x && y <= pt.y);
}
template <typename T>
NANO_INLINE_CXPR bool point<T>::operator>(const point& pt) const NANO_NOEXCEPT {
  return (x > pt.x && y > pt.y);
}
template <typename T>
NANO_INLINE_CXPR bool point<T>::operator>=(const point& pt) const NANO_NOEXCEPT {
  return (x >= pt.x && y >= pt.y);
}

template <typename T>
NANO_INLINE_CXPR point<T> point<T>::operator-() const NANO_NOEXCEPT {
  return { -x, -y };
}

template <typename T>
NANO_INLINE_CXPR point<T>& point<T>::set_x(value_type _x) NANO_NOEXCEPT {
  x = _x;
  return *this;
}

template <typename T>
NANO_INLINE_CXPR point<T>& point<T>::set_y(value_type _y) NANO_NOEXCEPT {
  y = _y;
  return *this;
}

template <typename T>
NANO_INLINE_CXPR point<T>& point<T>::add_x(value_type dx) NANO_NOEXCEPT {
  return set_x(x + dx);
}

template <typename T>
NANO_INLINE_CXPR point<T>& point<T>::add_y(value_type dy) NANO_NOEXCEPT {
  return set_y(y + dy);
}

template <typename T>
NANO_INLINE_CXPR point<T> point<T>::with_x(value_type _x) const NANO_NOEXCEPT {
  return point(_x, y);
}

template <typename T>
NANO_INLINE_CXPR point<T> point<T>::with_y(value_type _y) const NANO_NOEXCEPT {
  return point(x, _y);
}

template <typename T>
NANO_INLINE_CXPR point<T> point<T>::with_add_x(value_type dx) const NANO_NOEXCEPT {
  return point(x + dx, y);
}

template <typename T>
NANO_INLINE_CXPR point<T> point<T>::with_add_y(value_type dy) const NANO_NOEXCEPT {
  return point(x, y + dy);
}

template <typename T>
template <typename PointType, if_members<PointType, meta::x, meta::y>>
NANO_INLINE point<T>::operator PointType() const {
  using Type = decltype(PointType{}.x);
  return PointType{ static_cast<Type>(x), static_cast<Type>(y) };
}

template <typename T>
template <typename PointType, if_members<PointType, meta::X, meta::Y>>
NANO_INLINE point<T>::operator PointType() const {
  using Type = decltype(PointType{}.X);
  return PointType{ static_cast<Type>(x), static_cast<Type>(y) };
}

template <typename T>
template <typename PointType, if_members<PointType, meta::x, meta::y>>
NANO_INLINE PointType point<T>::convert() const {
  using Type = decltype(PointType{}.x);
  return PointType{ static_cast<Type>(x), static_cast<Type>(y) };
}

template <typename T>
template <typename PointType, if_members<PointType, meta::X, meta::Y>>
NANO_INLINE PointType point<T>::convert() const {
  using Type = decltype(PointType{}.X);
  return PointType{ static_cast<Type>(x), static_cast<Type>(y) };
}

template <typename T>
NANO_INLINE std::ostream& operator<<(std::ostream& s, const nano::point<T>& point) {
  return s << '{' << point.x << ',' << point.y << '}';
}

//
// MARK: - size -
//

template <typename T>
NANO_INLINE_CXPR size<T>::size(value_type W, value_type H) NANO_NOEXCEPT : width(W), height(H) {}

template <typename T>
template <typename SizeType, if_members<SizeType, meta::width, meta::height>>
NANO_INLINE_CXPR size<T>::size(const SizeType& s) NANO_NOEXCEPT : width(static_cast<value_type>(s.width)),
                                                                  height(static_cast<value_type>(s.height)) {}

template <typename T>
template <typename SizeType, if_members<SizeType, meta::Width, meta::Height>>
NANO_INLINE_CXPR size<T>::size(const SizeType& s) NANO_NOEXCEPT : width(static_cast<value_type>(s.Width)),
                                                                  height(static_cast<value_type>(s.Height)) {}

template <typename T>
NANO_INLINE_CXPR size<T> size<T>::full_scale() {
  return { std::numeric_limits<value_type>::max(), std::numeric_limits<value_type>::max() };
}

template <typename T>
NANO_INLINE_CXPR size<T> size<T>::zero() {
  return { 0, 0 };
}

template <typename T>
NANO_INLINE_CXPR size<T> size<T>::operator+(value_type v) const NANO_NOEXCEPT {
  return { width + v, height + v };
}
template <typename T>
NANO_INLINE_CXPR size<T> size<T>::operator-(value_type v) const NANO_NOEXCEPT {
  return { width - v, height - v };
}
template <typename T>
NANO_INLINE_CXPR size<T> size<T>::operator*(value_type v) const NANO_NOEXCEPT {
  return { width * v, height * v };
}
template <typename T>
NANO_INLINE_CXPR size<T> size<T>::operator/(value_type v) const NANO_NOEXCEPT {
  return { width / v, height / v };
}
template <typename T>
NANO_INLINE_CXPR size<T> size<T>::operator+(const size& s) const NANO_NOEXCEPT {
  return { width + s.width, height + s.height };
}
template <typename T>
NANO_INLINE_CXPR size<T> size<T>::operator-(const size& s) const NANO_NOEXCEPT {
  return { width - s.width, height - s.height };
}
template <typename T>
NANO_INLINE_CXPR size<T> size<T>::operator*(const size& s) const NANO_NOEXCEPT {
  return { width * s.width, height * s.height };
}
template <typename T>
NANO_INLINE_CXPR size<T> size<T>::operator/(const size& s) const NANO_NOEXCEPT {
  return { width / s.width, height / s.height };
}
template <typename T>
NANO_INLINE_CXPR size<T>& size<T>::operator+=(value_type v) NANO_NOEXCEPT {
  return *this = (*this + v);
}
template <typename T>
NANO_INLINE_CXPR size<T>& size<T>::operator-=(value_type v) NANO_NOEXCEPT {
  return *this = (*this - v);
}
template <typename T>
NANO_INLINE_CXPR size<T>& size<T>::operator*=(value_type v) NANO_NOEXCEPT {
  return *this = (*this * v);
}
template <typename T>
NANO_INLINE_CXPR size<T>& size<T>::operator/=(value_type v) NANO_NOEXCEPT {
  return *this = (*this / v);
}
template <typename T>
NANO_INLINE_CXPR size<T>& size<T>::operator+=(const size& s) NANO_NOEXCEPT {
  return *this = (*this + s);
}
template <typename T>
NANO_INLINE_CXPR size<T>& size<T>::operator-=(const size& s) NANO_NOEXCEPT {
  return *this = (*this - s);
}
template <typename T>
NANO_INLINE_CXPR size<T>& size<T>::operator*=(const size& s) NANO_NOEXCEPT {
  return *this = (*this * s);
}
template <typename T>
NANO_INLINE_CXPR size<T>& size<T>::operator/=(const size& s) NANO_NOEXCEPT {
  return *this = (*this / s);
}

template <typename T>
NANO_INLINE_CXPR bool size<T>::operator==(const size& s) const NANO_NOEXCEPT {
  if constexpr (std::is_floating_point_v<T>) {
    return nano::fcompare(width, s.width) && nano::fcompare(height, s.height);
  }
  else {
    return (width == s.width && height == s.height);
  }
}

template <typename T>
NANO_INLINE_CXPR bool size<T>::operator!=(const size& s) const NANO_NOEXCEPT {
  return !operator==(s);
}

template <typename T>
NANO_INLINE_CXPR bool size<T>::operator<(const size& s) const NANO_NOEXCEPT {
  return (width < s.width && height < s.height);
}
template <typename T>
NANO_INLINE_CXPR bool size<T>::operator<=(const size& s) const NANO_NOEXCEPT {
  return (width <= s.width && height <= s.height);
}
template <typename T>
NANO_INLINE_CXPR bool size<T>::operator>(const size& s) const NANO_NOEXCEPT {
  return (width > s.width && height > s.height);
}
template <typename T>
NANO_INLINE_CXPR bool size<T>::operator>=(const size& s) const NANO_NOEXCEPT {
  return (width >= s.width && height >= s.height);
}

//
template <typename T>
NANO_INLINE_CXPR size<T> size<T>::operator-() const NANO_NOEXCEPT {
  return { -width, -height };
}

template <typename T>
NANO_INLINE_CXPR size<T>& size<T>::set_width(value_type w) NANO_NOEXCEPT {
  width = w;
  return *this;
}

template <typename T>
NANO_INLINE_CXPR size<T>& size<T>::set_height(value_type h) NANO_NOEXCEPT {
  height = h;
  return *this;
}

template <typename T>
NANO_INLINE_CXPR size<T>& size<T>::add_width(value_type dw) NANO_NOEXCEPT {
  return set_width(width + dw);
}

template <typename T>
NANO_INLINE_CXPR size<T>& size<T>::add_height(value_type dh) NANO_NOEXCEPT {
  return set_height(height + dh);
}

template <typename T>
NANO_INLINE_CXPR size<T> size<T>::with_width(value_type w) const NANO_NOEXCEPT {
  return size(w, height);
}

template <typename T>
NANO_INLINE_CXPR size<T> size<T>::with_height(value_type h) const NANO_NOEXCEPT {
  return size(width, h);
}

template <typename T>
NANO_INLINE_CXPR size<T> size<T>::with_add_width(value_type dw) const NANO_NOEXCEPT {
  return size(width + dw, height);
}

template <typename T>
NANO_INLINE_CXPR size<T> size<T>::with_add_height(value_type dh) const NANO_NOEXCEPT {
  return size(width, height + dh);
}

template <typename T>
NANO_NODC_INLINE_CXPR bool size<T>::empty() const NANO_NOEXCEPT {
  return width == 0 && height == 0;
}

template <typename T>
template <typename SizeType, if_members<SizeType, meta::width, meta::height>>
NANO_INLINE size<T>::operator SizeType() const {
  using Type = decltype(SizeType{}.width);
  return SizeType{ static_cast<Type>(width), static_cast<Type>(height) };
}

template <typename T>
template <typename SizeType, if_members<SizeType, meta::Width, meta::Height>>
NANO_INLINE size<T>::operator SizeType() const {
  using Type = decltype(SizeType{}.Width);
  return SizeType{ static_cast<Type>(width), static_cast<Type>(height) };
}

template <typename T>
template <typename SizeType, if_members<SizeType, meta::width, meta::height>>
NANO_INLINE SizeType size<T>::convert() const {
  using Type = decltype(SizeType{}.width);
  return SizeType{ static_cast<Type>(width), static_cast<Type>(height) };
}

template <typename T>
template <typename SizeType, if_members<SizeType, meta::Width, meta::Height>>
NANO_INLINE SizeType size<T>::convert() const {
  using Type = decltype(SizeType{}.Width);
  return SizeType{ static_cast<Type>(width), static_cast<Type>(height) };
}

template <typename T>
NANO_INLINE std::ostream& operator<<(std::ostream& s, const nano::size<T>& size) {
  return s << '{' << size.width << ',' << size.height << '}';
}

//
// MARK: - rect -
//

template <typename T>
NANO_INLINE_CXPR rect<T>::rect(value_type x, value_type y, value_type w, value_type h) NANO_NOEXCEPT : origin{ x, y },
                                                                                                       size{ w, h } {}

template <typename T>
NANO_INLINE_CXPR rect<T>::rect(value_type x, value_type y, const size_type& s) NANO_NOEXCEPT : origin{ x, y },
                                                                                               size(s) {}

template <typename T>
template <typename RectType, if_members<RectType, meta::origin, meta::size>>
NANO_INLINE_CXPR rect<T>::rect(const RectType& rect) NANO_NOEXCEPT
    : origin{ static_cast<value_type>(rect.origin.x), static_cast<value_type>(rect.origin.y) },
      size{ static_cast<value_type>(rect.size.width), static_cast<value_type>(rect.size.height) } {}

template <typename T>
template <typename RectType, if_members<RectType, meta::X, meta::Y, meta::Width, meta::Height>>
NANO_INLINE_CXPR rect<T>::rect(const RectType& rect) NANO_NOEXCEPT
    : origin{ static_cast<value_type>(rect.X), static_cast<value_type>(rect.Y) },
      size{ static_cast<value_type>(rect.Width), static_cast<value_type>(rect.Height) } {}

template <typename T>
template <typename RectType, if_members<RectType, meta::left, meta::top, meta::right, meta::bottom>>
NANO_INLINE_CXPR rect<T>::rect(const RectType& rect) NANO_NOEXCEPT
    : origin{ static_cast<value_type>(rect.left), static_cast<value_type>(rect.top) },
      size{ static_cast<value_type>(rect.right - rect.left), static_cast<value_type>(rect.bottom - rect.top) } {}

template <typename T>
template <typename RectType, detail::enable_if_rect_xywh<RectType>>
NANO_INLINE_CXPR rect<T>::rect(const RectType& rect) NANO_NOEXCEPT
    : origin{ static_cast<value_type>(rect.x), static_cast<value_type>(rect.y) },
      size{ static_cast<value_type>(rect.width), static_cast<value_type>(rect.height) } {}

template <typename T>
NANO_INLINE_CXPR rect<T> rect<T>::create_from_point(
    const point_type& topLeft, const point_type& bottomRight) NANO_NOEXCEPT {
  return rect(topLeft.x, topLeft.y, bottomRight.x - topLeft.x, bottomRight.y - topLeft.y);
}

template <typename T>
NANO_INLINE_CXPR rect<T> rect<T>::create_from_bottom_left(
    value_type x, value_type y, value_type w, value_type h) NANO_NOEXCEPT {
  return rect(x, y - h, w, h);
}

template <typename T>
NANO_INLINE_CXPR rect<T> rect<T>::create_from_bottom_left(const point_type& p, const size_type& s) NANO_NOEXCEPT {
  return rect(p.x, p.y - s.height, s.width, s.height);
}

template <typename T>
NANO_INLINE_CXPR rect<T> rect<T>::create_from_bottom_right(
    value_type x, value_type y, value_type w, value_type h) NANO_NOEXCEPT {
  return rect(x - w, y - h, w, h);
}

template <typename T>
NANO_INLINE_CXPR rect<T> rect<T>::create_from_bottom_right(const point_type& p, const size_type& s) NANO_NOEXCEPT {
  return rect(p.x - s.width, p.y - s.height, s.width, s.height);
}

template <typename T>
NANO_INLINE_CXPR rect<T> rect<T>::create_from_top_left(
    value_type x, value_type y, value_type w, value_type h) NANO_NOEXCEPT {
  return rect(x, y, w, h);
}

template <typename T>
NANO_INLINE_CXPR rect<T> rect<T>::create_from_top_left(const point_type& p, const size_type& s) NANO_NOEXCEPT {
  return rect(p.x, p.y, s.width, s.height);
}

template <typename T>
NANO_INLINE_CXPR rect<T> rect<T>::create_from_top_right(
    value_type x, value_type y, value_type w, value_type h) NANO_NOEXCEPT {
  return rect(x - w, y, w, h);
}

template <typename T>
NANO_INLINE_CXPR rect<T> rect<T>::create_from_top_right(const point_type& p, const size_type& s) NANO_NOEXCEPT {
  return rect(p.x - s.width, p.y, s.width, s.height);
}

template <typename T>
NANO_INLINE_CXPR rect<T>& rect<T>::set_x(value_type _x) NANO_NOEXCEPT {
  x = _x;
  return *this;
}

template <typename T>
NANO_INLINE_CXPR rect<T>& rect<T>::set_y(value_type _y) NANO_NOEXCEPT {
  y = _y;
  return *this;
}

template <typename T>
NANO_INLINE_CXPR rect<T>& rect<T>::set_width(value_type w) NANO_NOEXCEPT {
  width = w;
  return *this;
}

template <typename T>
NANO_INLINE_CXPR rect<T>& rect<T>::set_height(value_type h) NANO_NOEXCEPT {
  height = h;
  return *this;
}

template <typename T>
NANO_INLINE_CXPR rect<T>& rect<T>::set_position(const point_type& point) NANO_NOEXCEPT {
  origin = point;
  return *this;
}

template <typename T>
NANO_INLINE_CXPR rect<T>& rect<T>::set_size(const size_type& s) NANO_NOEXCEPT {
  size = s;
  return *this;
}

template <typename T>
NANO_INLINE_CXPR rect<T> rect<T>::with_top_left(const point_type& point) const NANO_NOEXCEPT {
  return { point, size };
}

template <typename T>
NANO_INLINE_CXPR rect<T> rect<T>::with_top_right(const point_type& p) const NANO_NOEXCEPT {
  return { p - point_type{ width, static_cast<value_type>(0) }, size };
}

template <typename T>
NANO_INLINE_CXPR rect<T> rect<T>::with_bottom_left(const point_type& p) const NANO_NOEXCEPT {
  return { p - point_type{ static_cast<value_type>(0), height }, size };
}

template <typename T>
NANO_INLINE_CXPR rect<T> rect<T>::with_bottom_right(const point_type& p) const NANO_NOEXCEPT {
  return { p - point_type{ width, height }, size };
}

template <typename T>
NANO_INLINE_CXPR rect<T> rect<T>::with_middle(const point_type& point) const NANO_NOEXCEPT {
  return { static_cast<value_type>(point.x - width * 0.5), static_cast<value_type>(point.y - height * 0.5), width,
    height };
}

template <typename T>
NANO_INLINE_CXPR rect<T> rect<T>::with_middle_left(const point_type& point) const NANO_NOEXCEPT {
  return { point.x, point.y - height * 0.5, width, height };
}

template <typename T>
NANO_INLINE_CXPR rect<T> rect<T>::with_middle_right(const point_type& point) const NANO_NOEXCEPT {
  return { point.x - width, static_cast<value_type>(point.y - height * 0.5), width, height };
}

template <typename T>
NANO_INLINE_CXPR rect<T> rect<T>::with_middle_top(const point_type& point) const NANO_NOEXCEPT {
  return { static_cast<value_type>(point.x - width * 0.5), point.y, width, height };
}

template <typename T>
NANO_INLINE_CXPR rect<T> rect<T>::with_middle_bottom(const point_type& point) const NANO_NOEXCEPT {
  return { static_cast<value_type>(point.x - width * 0.5), point.y - height, width, height };
}

template <typename T>
NANO_INLINE_CXPR rect<T>& rect<T>::add_x(value_type _x) NANO_NOEXCEPT {
  x += _x;
  return *this;
}

template <typename T>
NANO_INLINE_CXPR rect<T>& rect<T>::add_y(value_type _y) NANO_NOEXCEPT {
  y += _y;
  return *this;
}

template <typename T>
NANO_INLINE_CXPR rect<T>& rect<T>::add_width(value_type w) NANO_NOEXCEPT {
  width += w;
  return *this;
}

template <typename T>
NANO_INLINE_CXPR rect<T>& rect<T>::add_height(value_type h) NANO_NOEXCEPT {
  height += h;
  return *this;
}

template <typename T>
NANO_INLINE_CXPR rect<T>& rect<T>::add_point(const point_type& point) NANO_NOEXCEPT {
  x += point.x;
  y += point.y;
  return *this;
}

template <typename T>
NANO_INLINE_CXPR rect<T>& rect<T>::add_size(const size_type& s) NANO_NOEXCEPT {
  width += s.width;
  height += s.height;
  return *this;
}

template <typename T>
NANO_INLINE_CXPR rect<T>& rect<T>::mul_x(value_type _x) NANO_NOEXCEPT {
  x *= _x;
  return *this;
}

template <typename T>
NANO_INLINE_CXPR rect<T>& rect<T>::mul_y(value_type _y) NANO_NOEXCEPT {
  y *= _y;
  return *this;
}

template <typename T>
NANO_INLINE_CXPR rect<T>& rect<T>::mul_width(value_type w) NANO_NOEXCEPT {
  width *= w;
  return *this;
}

template <typename T>
NANO_INLINE_CXPR rect<T>& rect<T>::mul_height(value_type h) NANO_NOEXCEPT {
  height *= h;
  return *this;
}

template <typename T>
NANO_INLINE_CXPR rect<T> rect<T>::with_x(value_type _x) const NANO_NOEXCEPT {
  return rect(point_type{ _x, y }, size);
}

template <typename T>
NANO_INLINE_CXPR rect<T> rect<T>::with_y(value_type _y) const NANO_NOEXCEPT {
  return rect(point_type{ x, _y }, size);
}

template <typename T>
NANO_INLINE_CXPR rect<T> rect<T>::with_width(value_type w) const NANO_NOEXCEPT {
  return rect(origin, size_type{ w, height });
}

template <typename T>
NANO_INLINE_CXPR rect<T> rect<T>::with_height(value_type h) const NANO_NOEXCEPT {
  return rect(origin, size_type{ width, h });
}

template <typename T>
NANO_INLINE_CXPR rect<T> rect<T>::with_position(const point_type& point) const NANO_NOEXCEPT {
  return rect(point, size);
}

template <typename T>
NANO_INLINE_CXPR rect<T> rect<T>::with_size(const size_type& s) const NANO_NOEXCEPT {
  return rect(origin, s);
}

template <typename T>
NANO_INLINE_CXPR rect<T> rect<T>::operator+(const point_type& pos) const NANO_NOEXCEPT {
  return rect(x + pos.x, y + pos.y, width, height);
}

template <typename T>
NANO_INLINE_CXPR rect<T> rect<T>::operator-(const point_type& pos) const NANO_NOEXCEPT {
  return rect(x - pos.x, y - pos.y, width, height);
}

template <typename T>
NANO_INLINE_CXPR rect<T>& rect<T>::operator+=(const point_type& pos) NANO_NOEXCEPT {
  x += pos.x;
  y += pos.y;
  return *this;
}

template <typename T>
NANO_INLINE_CXPR rect<T>& rect<T>::operator-=(const point_type& pos) NANO_NOEXCEPT {
  x -= pos.x;
  y -= pos.y;
  return *this;
}

template <typename T>
NANO_INLINE_CXPR typename rect<T>::value_type rect<T>::left() const NANO_NOEXCEPT {
  return x;
}

template <typename T>
NANO_INLINE_CXPR typename rect<T>::value_type rect<T>::right() const NANO_NOEXCEPT {
  return x + width;
}

template <typename T>
NANO_INLINE_CXPR typename rect<T>::value_type rect<T>::top() const NANO_NOEXCEPT {
  return y;
}

template <typename T>
NANO_INLINE_CXPR typename rect<T>::value_type rect<T>::bottom() const NANO_NOEXCEPT {
  return y + height;
}

template <typename T>
NANO_INLINE_CXPR typename rect<T>::point_type rect<T>::top_left() const NANO_NOEXCEPT {
  return origin;
}

template <typename T>
NANO_INLINE_CXPR typename rect<T>::point_type rect<T>::top_right() const NANO_NOEXCEPT {
  return { x + width, y };
}

template <typename T>
NANO_INLINE_CXPR typename rect<T>::point_type rect<T>::top_right(value_type dx, value_type dy) const NANO_NOEXCEPT {
  return point_type{ x + width + dx, y + dy };
}

template <typename T>
NANO_INLINE_CXPR typename rect<T>::point_type rect<T>::bottom_left() const NANO_NOEXCEPT {
  return point_type{ x, y + height };
}

template <typename T>
NANO_INLINE_CXPR typename rect<T>::point_type rect<T>::bottom_right() const NANO_NOEXCEPT {
  return point_type{ x + width, y + height };
}

template <typename T>
NANO_INLINE_CXPR typename rect<T>::point_type rect<T>::middle() const NANO_NOEXCEPT {

  return point_type{ static_cast<value_type>(x + width * 0.5), static_cast<value_type>(y + height * 0.5) };
}

template <typename T>
NANO_INLINE_CXPR typename rect<T>::point_type rect<T>::middle_left() const NANO_NOEXCEPT {
  return point_type{ x, static_cast<value_type>(y + height * 0.5) };
}

template <typename T>
NANO_INLINE_CXPR typename rect<T>::point_type rect<T>::middle_right() const NANO_NOEXCEPT {
  return point_type{ x + width, static_cast<value_type>(y + height * 0.5) };
}

template <typename T>
NANO_INLINE_CXPR typename rect<T>::point_type rect<T>::middle_top() const NANO_NOEXCEPT {
  return point_type{ static_cast<value_type>(x * 0.5), y };
}

template <typename T>
NANO_INLINE_CXPR typename rect<T>::point_type rect<T>::middle_bottom() const NANO_NOEXCEPT {
  return point_type{ static_cast<value_type>(x * 0.5), y + height };
}

template <typename T>
NANO_INLINE_CXPR typename rect<T>::point_type rect<T>::next_left(value_type delta) const NANO_NOEXCEPT {
  return point_type{ x - delta, y };
}

template <typename T>
NANO_INLINE_CXPR typename rect<T>::point_type rect<T>::next_left(const point_type& dt) const NANO_NOEXCEPT {
  return point_type{ x - dt.x, y + dt.y };
}

template <typename T>
NANO_INLINE_CXPR typename rect<T>::point_type rect<T>::next_right(value_type delta) const NANO_NOEXCEPT {
  return point_type{ x + width + delta, y };
}

template <typename T>
NANO_INLINE_CXPR typename rect<T>::point_type rect<T>::next_right(const point_type& dt) const NANO_NOEXCEPT {
  return point_type{ x + width + dt.x, y + dt.y };
}

template <typename T>
NANO_INLINE_CXPR typename rect<T>::point_type rect<T>::next_down(value_type delta) const NANO_NOEXCEPT {
  return point_type{ x, y + height + delta };
}

template <typename T>
NANO_INLINE_CXPR typename rect<T>::point_type rect<T>::next_down(const point_type& dt) const NANO_NOEXCEPT {
  return point_type{ x + dt.x, y + height + dt.y };
}

template <typename T>
NANO_INLINE_CXPR typename rect<T>::point_type rect<T>::next_up(value_type delta) const NANO_NOEXCEPT {
  return point_type{ x, y - delta };
}

template <typename T>
NANO_INLINE_CXPR typename rect<T>::point_type rect<T>::next_up(const point_type& dt) const NANO_NOEXCEPT {
  return point_type{ x + dt.x, y - dt.y };
}

template <typename T>
NANO_INLINE_CXPR bool rect<T>::operator==(const rect& r) const NANO_NOEXCEPT {
  if constexpr (std::is_floating_point_v<T>) {
    return nano::fcompare(x, r.x) && nano::fcompare(y, r.y) && nano::fcompare(width, r.width)
        && nano::fcompare(height, r.height);
  }
  else {
    return x == r.x && y == r.y && width == r.width && height == r.height;
  }

  //  return x == r.x && y == r.y && width == r.width && height == r.height;
}

template <typename T>
NANO_INLINE_CXPR bool rect<T>::operator!=(const rect& r) const NANO_NOEXCEPT {
  return !operator==(r);
}

template <typename T>
NANO_INLINE_CXPR bool rect<T>::contains(const point_type& pos) const NANO_NOEXCEPT {
  return pos.x >= x && pos.x <= x + width && pos.y >= y && pos.y <= y + height;
}

template <typename T>
NANO_INLINE_CXPR rect<T>& rect<T>::reduce(const point_type& pt) NANO_NOEXCEPT {
  x += pt.x;
  y += pt.y;
  width -= static_cast<value_type>(2) * pt.x;
  height -= static_cast<value_type>(2) * pt.y;
  return *this;
}

template <typename T>
NANO_INLINE_CXPR rect<T> rect<T>::reduced(const point_type& pt) const NANO_NOEXCEPT {
  return rect(
      x + pt.x, y + pt.y, width - static_cast<value_type>(2 * pt.x), height - static_cast<value_type>(2 * pt.y));
}

template <typename T>
NANO_INLINE_CXPR rect<T>& rect<T>::expand(const point_type& pt) NANO_NOEXCEPT {
  x -= pt.x;
  y -= pt.y;
  width += static_cast<value_type>(2) * pt.x;
  height += static_cast<value_type>(2) * pt.y;
  return *this;
}

template <typename T>
NANO_NODC_INLINE_CXPR rect<T> rect<T>::expanded(const point_type& pt) const NANO_NOEXCEPT {
  return rect(
      x - pt.x, y - pt.y, width + static_cast<value_type>(2 * pt.x), height + static_cast<value_type>(2 * pt.y));
}

template <typename T>
NANO_INLINE_CXPR bool rect<T>::intersects(const rect& r) const NANO_NOEXCEPT {
  return ((std::min(right(), r.right()) - std::max(x, r.x)) > 0)
      && ((std::min(bottom(), r.bottom()) - std::max(y, r.y)) > 0);
}

template <typename T>
NANO_INLINE_CXPR bool rect<T>::intersects(const point_type& p) const NANO_NOEXCEPT {
  return ((std::min(right(), p.x + std::numeric_limits<value_type>::epsilon()) - std::max(x, p.x)) >= 0)
      && ((std::min(bottom(), p.y + std::numeric_limits<value_type>::epsilon()) - std::max(y, p.y)) >= 0);
}

template <typename T>
NANO_INLINE_CXPR typename rect<T>::value_type rect<T>::area() const NANO_NOEXCEPT {
  return size.width * size.height;
}

template <typename T>
NANO_NODC_INLINE_CXPR rect<T> rect<T>::get_union(const rect& rhs) const NANO_NOEXCEPT {
  value_type nx = std::min(x, rhs.x);
  value_type ny = std::min(y, rhs.y);
  return { nx, ny, std::max(right(), rhs.right()) - nx, std::max(bottom(), rhs.bottom()) - ny };
}

template <typename T>
NANO_INLINE_CXPR rect<T>& rect<T>::merge(const rect& rhs) NANO_NOEXCEPT {
  value_type nx = std::min(x, rhs.x);
  value_type ny = std::min(y, rhs.y);
  *this = { nx, ny, std::max(right(), rhs.right()) - nx, std::max(bottom(), rhs.bottom()) - ny };
  return *this;
}

template <typename T>
NANO_NODC_INLINE_CXPR rect<T> rect<T>::merged(const rect& rhs) const NANO_NOEXCEPT {
  value_type nx = std::min(x, rhs.x);
  value_type ny = std::min(y, rhs.y);
  return { nx, ny, std::max(right(), rhs.right()) - nx, std::max(bottom(), rhs.bottom()) - ny };
}

template <typename T>
NANO_NODC_INLINE_CXPR rect<T> rect<T>::intersection(const rect& rhs) const NANO_NOEXCEPT {
  value_type nx = std::max(x, rhs.x);
  value_type nw = std::min(right(), rhs.right()) - nx;

  if (nw < 0) {
    return { 0, 0, 0, 0 };
  }

  value_type ny = std::max(y, rhs.y);
  value_type nh = std::min(bottom(), rhs.bottom()) - ny;

  if (nh < 0) {
    return { 0, 0, 0, 0 };
  }

  return { nx, ny, nw, nh };
}

template <typename T>
NANO_NODC_INLINE_CXPR rect<T> rect<T>::get_fitted_rect(const rect& r) const NANO_NOEXCEPT {
  if (width < height) {
    double hRatio = r.height / static_cast<double>(r.width);
    return r.with_size({ width, static_cast<value_type>(hRatio * width) });
  }
  else {
    double wRatio = r.width / static_cast<double>(r.height);
    return r.with_size({ static_cast<value_type>(wRatio * height), height });
  }
}

template <typename T>
NANO_INLINE void rect<T>::swap(rect<T>& r) NANO_NOEXCEPT {
  rect tmp = r;
  r = *this;
  *this = tmp;
}

template <typename T>
template <typename U>
NANO_INLINE void rect<T>::swap(rect<U>& r) NANO_NOEXCEPT {
  rect tmp = r;
  r = *this;
  *this = tmp;
}

template <typename T>
template <typename RectType, if_members<RectType, meta::origin, meta::size>>
NANO_INLINE rect<T>::operator RectType() const {
  using Type = decltype(RectType{}.origin.x);
  return RectType{ { static_cast<Type>(x), static_cast<Type>(y) },
    { static_cast<Type>(width), static_cast<Type>(height) } };
}

template <typename T>
template <typename RectType, if_members<RectType, meta::X, meta::Y, meta::Width, meta::Height>>
NANO_INLINE rect<T>::operator RectType() const {
  using Type = decltype(RectType{}.X);
  return RectType{ static_cast<Type>(x), static_cast<Type>(y), static_cast<Type>(width), static_cast<Type>(height) };
}

template <typename T>
template <typename RectType, if_members<RectType, meta::left, meta::top, meta::right, meta::bottom>>
NANO_INLINE rect<T>::operator RectType() const {
  using Type = decltype(RectType{}.left);
  return RectType{ static_cast<Type>(x), static_cast<Type>(y), static_cast<Type>(x + width),
    static_cast<Type>(y + height) };
}

template <typename T>
template <typename RectType, detail::enable_if_rect_xywh<RectType>>
NANO_INLINE rect<T>::operator RectType() const {
  using Type = decltype(RectType{}.x);
  return RectType{ static_cast<Type>(x), static_cast<Type>(y), static_cast<Type>(width), static_cast<Type>(height) };
}

template <typename T>
template <typename RectType, if_members<RectType, meta::origin, meta::size>>
NANO_INLINE RectType rect<T>::convert() const {
  using Type = decltype(RectType{}.origin.x);
  return RectType{ { static_cast<Type>(x), static_cast<Type>(y) },
    { static_cast<Type>(width), static_cast<Type>(height) } };
}

template <typename T>
template <typename RectType, if_members<RectType, meta::X, meta::Y, meta::Width, meta::Height>>
NANO_INLINE RectType rect<T>::convert() const {
  using Type = decltype(RectType{}.X);
  return RectType{ static_cast<Type>(x), static_cast<Type>(y), static_cast<Type>(width), static_cast<Type>(height) };
}

template <typename T>
template <typename RectType, if_members<RectType, meta::left, meta::top, meta::right, meta::bottom>>
NANO_INLINE RectType rect<T>::convert() const {
  using Type = decltype(RectType{}.left);
  return RectType{ static_cast<Type>(x), static_cast<Type>(y), static_cast<Type>(x + width),
    static_cast<Type>(y + height) };
}

template <typename T>
template <typename RectType, detail::enable_if_rect_xywh<RectType>>
NANO_INLINE RectType rect<T>::convert() const {
  using Type = decltype(RectType{}.x);
  return RectType{ static_cast<Type>(x), static_cast<Type>(y), static_cast<Type>(width), static_cast<Type>(height) };
}

template <typename T>
NANO_INLINE std::ostream& operator<<(std::ostream& s, const nano::rect<T>& rect) {
  return s << '{' << rect.x << ',' << rect.y << ',' << rect.width << ',' << rect.height << '}';
}

//
// MARK: - range -
//

template <typename T>
NANO_CXPR range<T> range<T>::with_length(value_type start, value_type len) NANO_NOEXCEPT {
  return { start, start + len };
}

template <typename T>
NANO_CXPR range<T>::range(value_type _start, value_type _end) NANO_NOEXCEPT : start(_start), end(_end) {}

template <typename T>
NANO_CXPR range<T> range<T>::with_start(value_type s) const NANO_NOEXCEPT {
  return { s, end };
}

template <typename T>
NANO_CXPR range<T> range<T>::with_end(value_type e) const NANO_NOEXCEPT {
  return { start, e };
}

template <typename T>
NANO_CXPR range<T> range<T>::with_shifted_start(value_type delta) const NANO_NOEXCEPT {
  return { start + delta, end };
}

template <typename T>
NANO_CXPR range<T> range<T>::with_shifted_end(value_type delta) const NANO_NOEXCEPT {
  return { start, end + delta };
}

template <typename T>
NANO_CXPR range<T> range<T>::with_length(value_type len) const NANO_NOEXCEPT {
  return { start, start + len };
}

template <typename T>
NANO_CXPR range<T> range<T>::with_shift(value_type delta) const NANO_NOEXCEPT {
  return { start + delta, end + delta };
}

template <typename T>
NANO_CXPR range<T> range<T>::with_move(value_type s) const NANO_NOEXCEPT {
  return { s, s + length() };
}

template <typename T>
NANO_INLINE_CXPR range<T>& range<T>::set_start(value_type s) NANO_NOEXCEPT {
  start = s;
  return *this;
}

template <typename T>
NANO_INLINE_CXPR range<T>& range<T>::set_end(value_type e) NANO_NOEXCEPT {
  end = e;
  return *this;
}

template <typename T>
NANO_INLINE_CXPR range<T>& range<T>::move_to(value_type s) NANO_NOEXCEPT {
  value_type len = length();
  start = s;
  end = s + len;
  return *this;
}

template <typename T>
NANO_INLINE_CXPR range<T>& range<T>::shift(value_type delta) NANO_NOEXCEPT {
  start += delta;
  end += delta;
  return *this;
}

template <typename T>
NANO_INLINE_CXPR range<T>& range<T>::shift_start(value_type delta) NANO_NOEXCEPT {
  start += delta;
  return *this;
}

template <typename T>
NANO_INLINE_CXPR range<T>& range<T>::shift_end(value_type delta) NANO_NOEXCEPT {
  end += delta;
  return *this;
}

template <typename T>
NANO_INLINE_CXPR range<T>& range<T>::set_length(value_type len) NANO_NOEXCEPT {
  end = start + len;
  return *this;
}

template <typename T>
NANO_CXPR typename range<T>::value_type range<T>::length() const NANO_NOEXCEPT {
  return end - start;
}

template <typename T>
NANO_CXPR typename range<T>::value_type range<T>::middle() const NANO_NOEXCEPT {
  return static_cast<T>(start + (end - start) * 0.5);
}

template <typename T>
NANO_CXPR bool range<T>::is_sorted() const NANO_NOEXCEPT {
  return start <= end;
}

template <typename T>
NANO_CXPR bool range<T>::is_symmetric() const NANO_NOEXCEPT {
  if constexpr (std::is_floating_point_v<T>) {
    return nano::fcompare(start, -end);
  }
  else {
    return start == -end;
  }
}

template <typename T>
NANO_CXPR bool range<T>::contains(value_type x) const NANO_NOEXCEPT {
  return x >= start && x <= end;
}

template <typename T>
NANO_CXPR bool range<T>::contains_closed(value_type x) const NANO_NOEXCEPT {
  return x >= start && x <= end;
}

template <typename T>
NANO_CXPR bool range<T>::contains_opened(value_type x) const NANO_NOEXCEPT {
  return x > start && x < end;
}

template <typename T>
NANO_CXPR bool range<T>::contains_left_opened(value_type x) const NANO_NOEXCEPT {
  return x > start && x <= end;
}

template <typename T>
NANO_CXPR bool range<T>::contains_right_opened(value_type x) const NANO_NOEXCEPT {
  return x >= start && x < end;
}

template <typename T>
NANO_CXPR bool range<T>::contains(const range& r) const NANO_NOEXCEPT {
  return contains(r.start) && contains(r.end);
}

template <typename T>
NANO_NODC_INLINE_CXPR typename range<T>::value_type range<T>::clipped_value(value_type x) const NANO_NOEXCEPT {
  const value_type t = x < start ? start : x;
  return t > end ? end : t;
}

template <typename T>
NANO_CXPR void range<T>::sort() NANO_NOEXCEPT {
  if (!is_sorted()) {
    std::swap(start, end);
  }
}

template <typename T>
NANO_CXPR bool range<T>::operator==(const range<T>& r) const NANO_NOEXCEPT {
  if constexpr (std::is_floating_point_v<T>) {
    return nano::fcompare(start, r.start) && nano::fcompare(end, r.end);
  }
  else {
    return start == r.start && end == r.end;
  }
}

template <typename T>
NANO_CXPR bool range<T>::operator!=(const range<T>& r) const NANO_NOEXCEPT {
  return !operator==(r);
}

template <class T>
NANO_INLINE std::ostream& operator<<(std::ostream& s, const range<T>& r) {
  return s << '{' << r.start << ',' << r.end << '}';
}

//
// MARK: - padding -
//

template <class T>
NANO_CXPR padding<T>::padding(value_type t, value_type l, value_type b, value_type r) NANO_NOEXCEPT : top(t),
                                                                                                      left(l),
                                                                                                      bottom(b),
                                                                                                      right(r) {}

template <class T>
NANO_CXPR padding<T>::padding(value_type p) NANO_NOEXCEPT : top(p), left(p), bottom(p), right(p) {}

template <class T>
nano::rect<T> padding<T>::inside_rect(const nano::rect<T>& rect) const NANO_NOEXCEPT {
  return nano::rect<T>(
      rect.origin.x + left, rect.origin.y + top, rect.size.width - (left + right), rect.size.height - (top + bottom));
}

template <class T>
nano::rect<T> padding<T>::outside_rect(const nano::rect<T>& rect) const NANO_NOEXCEPT {
  return nano::rect<T>(
      rect.origin.x - left, rect.origin.y - top, rect.size.width + left + right, rect.size.height + top + bottom);
}

template <class T>
NANO_CXPR bool padding<T>::empty() const NANO_NOEXCEPT {
  return top == 0 && left == 0 && bottom == 0 && right == 0;
}

template <class T>
NANO_CXPR bool padding<T>::operator==(const padding& p) const NANO_NOEXCEPT {
  return top == p.top && left == p.left && bottom == p.bottom && right == p.right;
}

template <class T>
NANO_CXPR bool padding<T>::operator!=(const padding& p) const NANO_NOEXCEPT {
  return !operator==(p);
}

template <class T>
std::ostream& operator<<(std::ostream& s, const padding<T>& p) {
  return s << '{' << p.top << ',' << p.left << ',' << p.bottom << ',' << p.right << '}';
}
} // namespace nano.

//
// MARK: - assert -
//

#ifndef NDEBUG
namespace nano {
namespace AssertDetail {
  NANO_INLINE void CustomAssert(const char* expr_str, bool expr, const char* file, int line, const char* msg) {
    if (NANO_LIKELY(expr)) {
      return;
    }

    std::cerr << "Assert failed:\t" << msg << "\n"
              << "Expected:\t" << expr_str << "\n"
              << "Source:\t\t" << file << ", line " << line << "\n";

    NANO_DEBUGTRAP();
  }

  NANO_INLINE void CustomError(const char* file, int line, const char* msg) {
    std::cerr << "Assert failed:\t" << msg << "\n"
              << "Source:\t\t" << file << ", line " << line << "\n";

    NANO_DEBUGTRAP();
  }
} // namespace AssertDetail.
} // namespace nano.
#endif // NDEBUG.

//----------------------------------------------------------------------------------------------------------------------
// MARK: - Graphics -
//----------------------------------------------------------------------------------------------------------------------

namespace nano {

//
// MARK: - color -
//

namespace detail {
  template <typename T>
  static inline std::uint32_t color_float_component_to_uint32(T f) {
    return static_cast<std::uint32_t>(std::floor(f * 255));
  }

  enum color_shift : std::uint32_t {
    color_shift_red = 24,
    color_shift_green = 16,
    color_shift_blue = 8,
    color_shift_alpha = 0
  };

  enum color_bits : std::uint32_t {
    color_bits_red = 0xff000000,
    color_bits_green = 0x00ff0000,
    color_bits_blue = 0x0000ff00,
    color_bits_alpha = 0x000000ff
  };
} // namespace detail.

NANO_INLINE_CXPR color::color(std::uint32_t rgba) NANO_NOEXCEPT : m_rgba(rgba) {}

NANO_INLINE_CXPR color::color(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) NANO_NOEXCEPT
    : m_rgba((std::uint32_t(a) << detail::color_shift_alpha) | (std::uint32_t(b) << detail::color_shift_blue)
          | (std::uint32_t(g) << detail::color_shift_green) | (std::uint32_t(r) << detail::color_shift_red)) {}

NANO_INLINE_CXPR color color::from_argb(std::uint32_t argb) NANO_NOEXCEPT {
  const color c(argb);
  return color(c.green(), c.blue(), c.alpha(), c.red());
}

template <typename T>
NANO_INLINE_CXPR color::color(const float_rgba<T>& rgba) NANO_NOEXCEPT {
  std::uint32_t ur = detail::color_float_component_to_uint32(rgba.r);
  std::uint32_t ug = detail::color_float_component_to_uint32(rgba.g);
  std::uint32_t ub = detail::color_float_component_to_uint32(rgba.b);
  std::uint32_t ua = detail::color_float_component_to_uint32(rgba.a);
  m_rgba = (ua << detail::color_shift_alpha) | (ub << detail::color_shift_blue) | (ug << detail::color_shift_green)
      | (ur << detail::color_shift_red);
}

template <typename T>
NANO_INLINE_CXPR color::color(const float_rgb<T>& rgb) NANO_NOEXCEPT {
  std::uint32_t ur = detail::color_float_component_to_uint32(rgb.r);
  std::uint32_t ug = detail::color_float_component_to_uint32(rgb.g);
  std::uint32_t ub = detail::color_float_component_to_uint32(rgb.b);
  m_rgba = (255 << detail::color_shift_alpha) | (ub << detail::color_shift_blue) | (ug << detail::color_shift_green)
      | (ur << detail::color_shift_red);
}

template <typename T>
NANO_INLINE_CXPR color::color(const float_grey_alpha<T>& ga) NANO_NOEXCEPT {
  std::uint32_t u = detail::color_float_component_to_uint32(ga.grey);
  std::uint32_t ua = detail::color_float_component_to_uint32(ga.alpha);
  m_rgba = (ua << detail::color_shift_alpha) | (u << detail::color_shift_blue) | (u << detail::color_shift_green)
      | (u << detail::color_shift_red);
}

template <typename T, std::size_t Size, std::enable_if_t<std::is_floating_point<T>::value, int>>
NANO_INLINE_CXPR color::color(const T (&data)[Size]) NANO_NOEXCEPT : color(&data[0], Size) {}

template <typename T, std::enable_if_t<std::is_floating_point<T>::value, int>>
NANO_INLINE_CXPR color::color(const T* data, std::size_t size) NANO_NOEXCEPT {
  switch (size) {
  case 2: {
    std::uint32_t u = detail::color_float_component_to_uint32(data[0]);
    std::uint32_t a = detail::color_float_component_to_uint32(data[1]);
    m_rgba = (a << detail::color_shift_alpha) | (u << detail::color_shift_blue) | (u << detail::color_shift_green)
        | (u << detail::color_shift_red);
  }
    return;
  case 3: {
    std::uint32_t ur = detail::color_float_component_to_uint32(data[0]);
    std::uint32_t ug = detail::color_float_component_to_uint32(data[1]);
    std::uint32_t ub = detail::color_float_component_to_uint32(data[2]);
    m_rgba = (255 << detail::color_shift_alpha) | (ub << detail::color_shift_blue) | (ug << detail::color_shift_green)
        | (ur << detail::color_shift_red);
  }
    return;
  case 4: {
    std::uint32_t ur = detail::color_float_component_to_uint32(data[0]);
    std::uint32_t ug = detail::color_float_component_to_uint32(data[1]);
    std::uint32_t ub = detail::color_float_component_to_uint32(data[2]);
    std::uint32_t ua = detail::color_float_component_to_uint32(data[3]);
    m_rgba = (ua << detail::color_shift_alpha) | (ub << detail::color_shift_blue) | (ug << detail::color_shift_green)
        | (ur << detail::color_shift_red);
  }
    return;
  }
}

NANO_NODC_INLINE_CXPR std::uint32_t& color::rgba() NANO_NOEXCEPT { return m_rgba; }

NANO_NODC_INLINE_CXPR std::uint32_t color::rgba() const NANO_NOEXCEPT { return m_rgba; }

NANO_NODC_INLINE_CXPR std::uint32_t color::argb() const NANO_NOEXCEPT {
  using u32 = std::uint32_t;
  return (u32(blue()) << detail::color_shift_alpha) | (u32(green()) << detail::color_shift_blue)
      | (u32(red()) << detail::color_shift_green) | (u32(alpha()) << detail::color_shift_red);
}

NANO_NODC_INLINE_CXPR color::float_rgba<float> color::f_rgba() const NANO_NOEXCEPT {
  return { f_red(), f_green(), f_blue(), f_alpha() };
}

template <typename T, std::enable_if_t<std::is_floating_point<T>::value, int>>
NANO_NODC_INLINE_CXPR color::float_rgba<T> color::f_rgba() const NANO_NOEXCEPT {
  return { static_cast<T>(f_red()), static_cast<T>(f_green()), static_cast<T>(f_blue()), static_cast<T>(f_alpha()) };
}

template <typename T>
NANO_NODC_INLINE_CXPR std::enable_if_t<std::is_arithmetic_v<T>, T> color::red() const NANO_NOEXCEPT {
  if constexpr (std::is_floating_point_v<T>) {
    return static_cast<T>(f_red());
  }
  else {
    return static_cast<T>(red());
  }
}

template <typename T>
NANO_NODC_INLINE_CXPR std::enable_if_t<std::is_arithmetic_v<T>, T> color::green() const NANO_NOEXCEPT {
  if constexpr (std::is_floating_point_v<T>) {
    return static_cast<T>(f_green());
  }
  else {
    return static_cast<T>(green());
  }
}

template <typename T>
NANO_NODC_INLINE_CXPR std::enable_if_t<std::is_arithmetic_v<T>, T> color::blue() const NANO_NOEXCEPT {
  if constexpr (std::is_floating_point_v<T>) {
    return static_cast<T>(f_blue());
  }
  else {
    return static_cast<T>(blue());
  }
}

template <typename T>
NANO_NODC_INLINE_CXPR std::enable_if_t<std::is_arithmetic_v<T>, T> color::alpha() const NANO_NOEXCEPT {
  if constexpr (std::is_floating_point_v<T>) {
    return static_cast<T>(f_alpha());
  }
  else {
    return static_cast<T>(alpha());
  }
}

NANO_INLINE_CXPR color& color::red(std::uint8_t r) NANO_NOEXCEPT {
  m_rgba = (m_rgba & ~detail::color_bits_red) | (std::uint32_t(r) << detail::color_shift_red);
  return *this;
}

NANO_INLINE_CXPR color& color::green(std::uint8_t g) NANO_NOEXCEPT {
  m_rgba = (m_rgba & ~detail::color_bits_green) | (std::uint32_t(g) << detail::color_shift_green);
  return *this;
}

NANO_INLINE_CXPR color& color::blue(std::uint8_t b) NANO_NOEXCEPT {
  m_rgba = (m_rgba & ~detail::color_bits_blue) | (std::uint32_t(b) << detail::color_shift_blue);
  return *this;
}

NANO_INLINE_CXPR color& color::alpha(std::uint8_t a) NANO_NOEXCEPT {
  m_rgba = (m_rgba & ~detail::color_bits_alpha) | (std::uint32_t(a) << detail::color_shift_alpha);
  return *this;
}

NANO_NODC_INLINE_CXPR std::uint8_t color::red() const NANO_NOEXCEPT {
  return static_cast<std::uint8_t>((m_rgba & detail::color_bits_red) >> detail::color_shift_red);
}

NANO_NODC_INLINE_CXPR std::uint8_t color::green() const NANO_NOEXCEPT {
  return static_cast<std::uint8_t>((m_rgba & detail::color_bits_green) >> detail::color_shift_green);
}

NANO_NODC_INLINE_CXPR std::uint8_t color::blue() const NANO_NOEXCEPT {
  return static_cast<std::uint8_t>((m_rgba & detail::color_bits_blue) >> detail::color_shift_blue);
}

NANO_NODC_INLINE_CXPR std::uint8_t color::alpha() const NANO_NOEXCEPT {
  return static_cast<std::uint8_t>((m_rgba & detail::color_bits_alpha) >> detail::color_shift_alpha);
}

NANO_NODC_INLINE_CXPR float color::f_red() const NANO_NOEXCEPT { return red() / 255.0f; }
NANO_NODC_INLINE_CXPR float color::f_green() const NANO_NOEXCEPT { return green() / 255.0f; }
NANO_NODC_INLINE_CXPR float color::f_blue() const NANO_NOEXCEPT { return blue() / 255.0f; }
NANO_NODC_INLINE_CXPR float color::f_alpha() const NANO_NOEXCEPT { return alpha() / 255.0f; }

NANO_NODC_INLINE_CXPR bool color::is_opaque() const NANO_NOEXCEPT { return alpha() == 255; }
NANO_NODC_INLINE_CXPR bool color::is_transparent() const NANO_NOEXCEPT { return alpha() != 255; }

NANO_NODC_INLINE_CXPR color color::darker(float amount) const NANO_NOEXCEPT {
  amount = 1.0f - std::clamp<float>(amount, 0.0f, 1.0f);
  return color(std::uint8_t(red() * amount), std::uint8_t(green() * amount), std::uint8_t(blue() * amount), alpha());
}

NANO_NODC_INLINE_CXPR color color::brighter(float amount) const NANO_NOEXCEPT {
  const float ratio = 1.0f / (1.0f + std::abs(amount));
  const float mu = 255 * (1.0f - ratio);

  return color(static_cast<std::uint8_t>(mu + ratio * red()), // r
      static_cast<std::uint8_t>(mu + ratio * green()), // g
      static_cast<std::uint8_t>(mu + ratio * blue()), // b
      alpha() // a
  );
}

NANO_NODC_INLINE_CXPR color color::with_red(std::uint8_t red) const NANO_NOEXCEPT {
  return color(red, green(), blue(), alpha());
}

NANO_NODC_INLINE_CXPR color color::with_green(std::uint8_t green) const NANO_NOEXCEPT {
  return color(red(), green, blue(), alpha());
}

NANO_NODC_INLINE_CXPR color color::with_blue(std::uint8_t blue) const NANO_NOEXCEPT {
  return color(red(), green(), blue, alpha());
}

NANO_NODC_INLINE_CXPR color color::with_alpha(std::uint8_t alpha) const NANO_NOEXCEPT {
  return color(red(), green(), blue(), alpha);
}

/// mu should be between [0, 1]
NANO_NODC_INLINE_CXPR color color::operator*(float mu) const NANO_NOEXCEPT {
  return color(static_cast<std::uint8_t>(red() * mu), static_cast<std::uint8_t>(green() * mu),
      static_cast<std::uint8_t>(blue() * mu), static_cast<std::uint8_t>(alpha() * mu));
}

NANO_NODC_INLINE_CXPR bool color::operator==(const color& c) const NANO_NOEXCEPT { return m_rgba == c.m_rgba; }

NANO_NODC_INLINE_CXPR bool color::operator!=(const color& c) const NANO_NOEXCEPT { return !operator==(c); }

template <class CharT, class TraitsT>
inline std::basic_ostream<CharT, TraitsT>& operator<<(std::basic_ostream<CharT, TraitsT>& s, const nano::color& c) {
  std::ios_base::fmtflags flags(s.flags());
  s << CharT('#') << std::uppercase << std::hex << std::setfill(CharT('0')) << std::setw(8) << c.rgba();
  s.flags(flags);
  return s;
}

//----------------------------------------------------------------------------------------------------------------------
// MARK: - UI -
//----------------------------------------------------------------------------------------------------------------------

//
// MARK: - event -
//

bool is_mouse_event(event_type type) noexcept {
  NANO_CLANG_PUSH_WARNING("-Wswitch-enum")
  switch (type) {
  case event_type::left_mouse_down:
  case event_type::left_mouse_up:
  case event_type::right_mouse_down:
  case event_type::right_mouse_up:
  case event_type::mouse_moved:
  case event_type::left_mouse_dragged:
  case event_type::right_mouse_dragged:
  case event_type::other_mouse_down:
  case event_type::other_mouse_up:
  case event_type::other_mouse_dragged:
    return true;

  default:
    return false;
  }
  NANO_CLANG_POP_WARNING()
}

bool is_scroll_or_drag_event(event_type type) noexcept {
  NANO_CLANG_PUSH_WARNING("-Wswitch-enum")
  switch (type) {
  case event_type::scroll_wheel:
  case event_type::mouse_moved:
  case event_type::left_mouse_dragged:
  case event_type::right_mouse_dragged:
  case event_type::other_mouse_dragged:
    return true;

  default:
    return false;
  }
  NANO_CLANG_POP_WARNING()
}

bool is_click_event(event_type type) noexcept {
  NANO_CLANG_PUSH_WARNING("-Wswitch-enum")
  switch (type) {
  case event_type::left_mouse_down:
  case event_type::left_mouse_up:
  case event_type::right_mouse_down:
  case event_type::right_mouse_up:
  case event_type::other_mouse_down:
  case event_type::other_mouse_up:
    return true;

  default:
    return false;
  }
  NANO_CLANG_POP_WARNING()
}

bool is_key_event(event_type type) noexcept {
  NANO_CLANG_PUSH_WARNING("-Wswitch-enum")
  switch (type) {
  case event_type::key_down:
  case event_type::key_up:
  case event_type::key_flags_changed:
    return true;

  default:
    return false;
  }
  NANO_CLANG_POP_WARNING()
}

native_event_handle event::get_native_handle() const noexcept { return m_native_handle; }

nano::view* event::get_view() const noexcept { return m_view; }

event_type event::get_event_type() const noexcept { return m_type; }

std::uint64_t event::get_timestamp() const noexcept { return m_timestamp; }

bool event::is_mouse_event() const noexcept { return nano::is_mouse_event(m_type); }

bool event::is_scroll_or_drag_event() const noexcept { return nano::is_scroll_or_drag_event(m_type); }

bool event::is_click_event() const noexcept { return nano::is_click_event(m_type); }

bool event::is_key_event() const noexcept { return nano::is_key_event(m_type); }

const nano::point<float>& event::get_position() const noexcept { return m_position; }

const nano::point<float>& event::get_click_position() const noexcept { return m_click_position; }

const nano::point<float>& event::get_wheel_delta() const noexcept { return m_wheel_delta; }

event_modifiers event::get_modifiers() const noexcept { return m_event_modifiers; }

std::int64_t event::get_click_count() const noexcept { return m_click_count; }

bool event::is_left_button_down() const noexcept { return (m_event_modifiers & event_modifiers::left_mouse_down) != 0; }

bool event::is_middle_button_down() const noexcept {
  return (m_event_modifiers & event_modifiers::middle_mouse_down) != 0;
}

bool event::is_right_button_down() const noexcept {
  return (m_event_modifiers & event_modifiers::right_mouse_down) != 0;
}

bool event::is_command_down() const noexcept { return (m_event_modifiers & event_modifiers::command) != 0; }

bool event::is_shift_down() const noexcept { return (m_event_modifiers & event_modifiers::shift) != 0; }

bool event::is_ctrl_down() const noexcept { return (m_event_modifiers & event_modifiers::control) != 0; }

bool event::is_alt_down() const noexcept { return (m_event_modifiers & event_modifiers::alt) != 0; }

bool event::is_function_down() const noexcept { return (m_event_modifiers & event_modifiers::function) != 0; }

inline window_proxy view::get_window_proxy() {
  NANO_ASSERT(is_window(), "Not a window");
  return window_proxy(*this);
}

class message {
public:
  virtual ~message();

  virtual void call() = 0;
};

void post_message(std::shared_ptr<message> msg);

template <typename Fct>
inline void post_message(Fct&& fct) {

  NANO_CLANG_PUSH_WARNING("-Wpadded")
  class callback : public message {
  public:
    callback(Fct&& fct)
        : m_fct(std::forward<Fct>(fct)) {}

    virtual ~callback() override = default;

    virtual void call() override { m_fct(); }

    Fct m_fct;
  };
  NANO_CLANG_POP_WARNING()

  post_message(std::shared_ptr<message>(new callback(std::forward<Fct>(fct))));
}

} // namespace nano

NANO_CLANG_DIAGNOSTIC_POP()
