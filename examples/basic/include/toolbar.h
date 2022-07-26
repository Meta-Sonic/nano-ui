#pragma once

#include <nano/ui.h>

class Toolbar : public nano::view {
public:
  Toolbar(nano::view* parent, const nano::rect<int>& rect);

  ~Toolbar() override;

protected:
  void on_mouse_down(const nano::event& evt) override;

  void on_focus() override;
  void on_unfocus() override;

  void on_draw(nano::graphic_context& gc, const nano::rect<float>& dirty_rect) override;
};
