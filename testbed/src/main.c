#include <core/logger.h>
#include <platform/platform.h>

#define WIDTH 1280
#define HEIGHT 720
#define NAME "Kohi Engine Testbed"
#define X 100
#define Y 100

int main(void) {
  const float pi = 3.14F;
  ktrace("Pi = %f", pi);
  kdebug("Pi = %f", pi);
  kinfo("Pi = %f", pi);
  kwarn("Pi = %f", pi);
  kerror("Pi = %f", pi);
  kfatal("Pi = %f", pi);

  platform_config config = {
      .width = WIDTH,
      .height = HEIGHT,
      .application_name = NAME,
      .x = X,
      .y = Y,
  };
  if (platform_startup(config)) {
    while (platform_pump_messages()) {
    }
  }
  platform_shutdown();
  return 0;
}
