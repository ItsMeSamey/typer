#include <cstdio>
#include "arg-parse-c.h"

void parg(char* a, char** v, size_t t1, size_t t2){
 printf("A: %s; B: %s;\n", a, *v?*v:"");
}

int main(int argc,char **argv){
  char *a = (char *)"a";
  arg_parser::parsable_arguments(a, (void *)parg);
  arg_parser::parse_arguments(argc, argv);
  return 0;
}
