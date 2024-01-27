// #include <cstdio>
#include "multi-file.h"

void parg(char *a, char **v, size_t t1, size_t t2) {
  printf("A: %s; B: %s;\n", a, *v ? *v : "");
}

int main(int argc, char **argv) {
  multi_file_struct S;
  FILE *multi_file = multi_file_init(&S, 1, (char *)"1.txt");

  multi_file_write(&S, 0, (char *)"HEL_LO_\xff");
  multi_file_write(&S, 0, (char *)"!!sdasdasd!!");

  multi_file_flush(&S);
  fclose(multi_file); /* close the file */
  void *what;
  return 0;
}
