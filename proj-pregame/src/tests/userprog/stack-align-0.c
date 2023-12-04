/* Does absolutely nothing. */
#include <stdio.h>
#include "tests/lib.h"

int main(int argc UNUSED, char* argv[] UNUSED) {
  //hex_dump(0xbfffffb0, 0xbfffffb0, 0x50, true);
  register unsigned int esp asm("esp");
  return esp % 16;
}
