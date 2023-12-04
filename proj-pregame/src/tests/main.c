#include <random.h>
#include "tests/lib.h"
#include "tests/main.h"
#include <stdio.h>
int main(int argc UNUSED, char* argv[]) {
  // hex_dump(0, 0xbfffffe0, 0x20, true);
  test_name = argv[0];

  msg("begin");
  random_init(0);
#ifdef THREADS
  console_init();
#endif
  test_main();
  msg("end");
  return 0;
}
