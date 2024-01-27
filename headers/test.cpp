// #include <cstdio>
#include "multi-file-c.h"

void parg(char *a, char **v, size_t t1, size_t t2) {
  printf("A: %s; B: %s;\n", a, *v ? *v : "");
}

int main(int argc, char **argv) {
  multi_file_struct S;
  multi_file_write_oprations(&S, 0, (void*)"1.txt", 1);
  multi_file_write_oprations(&S, 2, (void*)"HEL_LO_\xff", 0, 8);
//   multi_file_write_oprations(&S, 2, (void*)"This Seems To be working", 0);
  multi_file_write_oprations(&S, 0, (void*)"1.txt", 1);
  multi_file_write_oprations(&S, 2, (void*)"!!!!", 1);
  multi_file_write_oprations(&S, 3);
  void *what;
  return 0;
}
