#include "toolbar.h"

Toolbar::Toolbar(nano::view* parent, const nano::rect<int>& rect)
    : nano::view(parent, rect) {}

Toolbar::~Toolbar() {}

void Toolbar::on_mouse_down(const nano::event& evt) {
  std::cout << "MainView::on_mouse_down " << evt.get_position() << std::endl;

  focus();
}

void Toolbar::on_focus() { redraw(); }

void Toolbar::on_unfocus() { redraw(); }

void Toolbar::on_draw(nano::graphic_context& gc, const nano::rect<float>& dirty_rect) {
  constexpr nano::color bg_color = 0x468DE7FF;
  constexpr nano::color bg_focused_color = bg_color.brighter(0.1f);

  const nano::rect<float> bounds = get_bounds();

  gc.set_fill_color(is_focused() ? bg_focused_color : bg_color);
  gc.fill_rect(bounds);
}
