#include "toolbar.h"

Toolbar::Toolbar(nano::view* parent, const nano::rect<int>& rect)
    : nano::view(parent, rect) {}

Toolbar::~Toolbar() {}

void Toolbar::on_draw(nano::graphic_context& gc, const nano::rect<float>& dirty_rect) {
  nano::rect<float> bounds = get_bounds();
  gc.set_fill_color(0xBBBBBBFF);
  gc.fill_rect(bounds);

  constexpr float line_width = 2.0f;
  constexpr float h_line_width = line_width * 0.5f;

  gc.set_line_width(line_width);
  gc.set_stroke_color(0x888888FF);

  nano::rect<float> contour = bounds.reduce({ 0, h_line_width });
  gc.stroke_line(contour.top_left(), contour.top_right());
  gc.stroke_line(contour.bottom_left(), contour.bottom_right());
}
