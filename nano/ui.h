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
 * @file      nano/ui.h
 * @brief     nano ui
 * @copyright Copyright (C) 2022, Meta-Sonic
 * @author    Alexandre Arsenault alx.arsenault@gmail.com
 * @date      Created 16/06/2022
 */

#include <nano/graphics.h>

#include <array>
#include <cassert>
#include <cstdlib>
#include <iomanip>
#include <iterator>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <vector>

NANO_CLANG_DIAGNOSTIC_PUSH()
NANO_CLANG_DIAGNOSTIC(warning, "-Weverything")
NANO_CLANG_DIAGNOSTIC(ignored, "-Wc++98-compat")

namespace nano {

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

  virtual void on_will_draw() {}

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

namespace nano {

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
