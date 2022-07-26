#pragma once

#include <nano/ui.h>

class MainWindow;

class App : public nano::application {
public:
  App();

  ~App() override;

  std::string get_application_name() const override;

  std::string get_application_version() const override;

  void initialise() override;

  void shutdown() override;

private:
  std::unique_ptr<MainWindow> m_window;
};
