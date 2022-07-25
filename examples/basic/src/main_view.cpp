#include "main_view.h"

MainView::MainView(nano::view* parent, const nano::rect<int>& rect)
    : nano::view(parent, rect) {}

MainView::~MainView() {}


void MainView::on_mouse_down(const nano::event& evt)
{
  std::cout << "MainView::on_mouse_down " << evt.get_position() << std::endl;

  focus();
}

void MainView::on_key_down(const nano::event& evt)
{
  std::cout << "MainView::on_key_down " << std::string((const char*)evt.get_key().data()) << std::endl;

}

void MainView::on_will_draw() {
  std::cout << "MainView::on_will_draw " << is_dirty_rect({0, 0, 10, 10}) << std::endl;
}

void MainView::on_draw(nano::graphic_context& gc, const nano::rect<float>& dirty_rect) {
  std::cout << "MainView::on_draw " << dirty_rect << std::endl;

  nano::rect<float> bounds = get_bounds();

  gc.set_fill_color(0xAAAAAAFF);
  gc.fill_rounded_rect(bounds, 10);

  constexpr float line_width = 1.5f;
  constexpr float h_line_width = line_width * 0.5f;

  nano::rect<float> contour = bounds.reduced({ h_line_width, h_line_width });
  gc.set_line_width(line_width);
  gc.set_stroke_color(0x333333FF);
  gc.stroke_rounded_rect(contour, 10);
}
