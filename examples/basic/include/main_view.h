#pragma once

#include <nano/ui.h>

class MainView : public nano::view {
public:
  MainView(nano::view* parent, const nano::rect<int>& rect);

  ~MainView() override;

protected:
  void on_mouse_down(const nano::event& evt) override;
  void on_key_down(const nano::event& evt) override;
  void on_focus() override;
  void on_unfocus() override;
  void on_will_draw() override;
  void on_draw(nano::graphic_context& gc, const nano::rect<float>& dirty_rect) override;
};
