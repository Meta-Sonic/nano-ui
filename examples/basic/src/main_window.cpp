#include "main_window.h"
#include "main_view.h"
#include "toolbar.h"

MainWindow::MainWindow()
    : nano::window(nano::window_flags::default_flags) {
  m_toolbar = std::make_unique<Toolbar>(this, nano::rect<int>{ 0, 0, 200, 50 });
  m_view = std::make_unique<MainView>(this, nano::rect<int>{ 10, 10, 200, 200 });
}

MainWindow::~MainWindow() {}

void MainWindow::on_frame_changed() {
  nano::rect<int> bounds = get_bounds();
  m_toolbar->set_frame({ bounds.position, { bounds.width, 50 } });

  const int padding = 10;
  m_view->set_frame({ bounds.position + nano::point<int>{ padding, 50 + padding },
      { bounds.width - 2 * padding, bounds.height - 50 - 2 * padding } });
}

void MainWindow::on_draw(nano::graphic_context& gc, const nano::rect<float>& dirtyRect) {
  gc.set_fill_color(0xEEEEEEFF);
  gc.fill_rect(get_bounds());
}
