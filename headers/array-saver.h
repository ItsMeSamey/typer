#pragma once

#ifndef __cplusplus // if !c++
#include <stdio.h>
// #include <stdint.h>
#include <stdlib.h>
#endif // end if !c++

#ifdef __cplusplus // if c++
#include <cstdio>
// #include <cstdint>
#include <cstdlib>
#endif // end if c++

#include "multi-file.h"

#define STRUCT_UNLOADER(STRUCT);

void array_saver(multi_file_struct *STRUCT, void** ARRAY_ARRAY, size_t *LEN_ARRAY, size_t LEN){
  char **buff_loc; size_t *tell;
  FILE *fp = open_memstream(buff_loc, tell);
  return ;
}

