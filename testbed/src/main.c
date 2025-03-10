#include <core/asserts.h>
#include <core/logger.h>

int main(void) {
  const float pi = 3.14F;
  ktrace("Pi = %f", pi);
  kdebug("Pi = %f", pi);
  kinfo("Pi = %f", pi);
  kwarn("Pi = %f", pi);
  kerror("Pi = %f", pi);
  kfatal("Pi = %f", pi);

  kassert_msg(1 == 1, "1 not equal to 3!");
  return 0;
}
