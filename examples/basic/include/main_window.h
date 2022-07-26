#pragma once

#include <nano/ui.h>

class MainView;
class Toolbar;

class MainWindow : public nano::window {
public:
  MainWindow();
  ~MainWindow() override;

protected:
  void on_frame_changed() override;
  void on_draw(nano::graphic_context& gc, const nano::rect<float>& dirtyRect) override;

private:
  std::unique_ptr<Toolbar> m_toolbar;
  std::unique_ptr<MainView> m_view;
};
