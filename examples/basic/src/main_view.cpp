#include "main_view.h"

MainView::MainView(nano::view* parent, const nano::rect<int>& rect)
    : nano::view(parent, rect) {}

MainView::~MainView() {}

void MainView::on_mouse_down(const nano::event& evt) {
  std::cout << "MainView::on_mouse_down " << evt.get_position() << std::endl;

  focus();
}

void MainView::on_key_down(const nano::event& evt) {
  std::cout << "MainView::on_key_down " << std::string((const char*)evt.get_key().data()) << std::endl;
}

void MainView::on_focus() { redraw(); }

void MainView::on_unfocus() { redraw(); }

void MainView::on_will_draw() {
  std::cout << "MainView::on_will_draw " << is_dirty_rect({ 0, 0, 10, 10 }) << std::endl;
}

void MainView::on_draw(nano::graphic_context& gc, const nano::rect<float>& dirty_rect) {
  std::cout << "MainView::on_draw " << dirty_rect << std::endl;

  nano::rect<float> bounds = get_bounds();

  constexpr nano::color bg_color = 0xAAAAAAFF;

  gc.set_fill_color(bg_color);
  gc.fill_rounded_rect(bounds, 10);

  if (is_focused()) {
    constexpr float line_width = 2.5f;
    constexpr float h_line_width = line_width * 0.5f;

    nano::rect<float> contour = bounds.reduced({ h_line_width, h_line_width });
    gc.set_line_width(line_width);
    gc.set_stroke_color(nano::colors::white);
    gc.stroke_rounded_rect(contour, 8);
  }
}
