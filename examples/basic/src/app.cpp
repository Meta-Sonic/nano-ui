#include "app.h"
#include "main_window.h"

App::App() {}

App::~App() {}

std::string App::get_application_name() const { return ""; }

std::string App::get_application_version() const { return "1"; }

void App::initialise() {
  m_window = std::make_unique<MainWindow>();
  m_window->set_title("Demo");
  m_window->set_window_frame({ 0, 0, 800, 800 });
  m_window->center();
}

void App::shutdown() {
  std::cout << "Shutdown" << std::endl;
  m_window.reset();
}
