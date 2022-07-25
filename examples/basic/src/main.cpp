#include "app.h"

int UIApplicationMain() {
  std::unique_ptr<App> app = nano::application::create_application<App>(NANO_APPLICATION_MAIN_ARGS);
  return app->run();
}
