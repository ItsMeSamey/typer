#pragma once

#ifndef PARSABLE_ARGUMENTS_NAME
#define PARSABLE_ARGUMENTS_NAME parsable_arguments
#endif
#ifndef PARSE_ARGUMENTS_NAME
#define PARSE_ARGUMENTS_NAME parse_arguments
#endif

#ifndef __cplusplus // if !c++
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#endif // end if !c++

#ifdef __cplusplus // if c++
#include <cstdio>

#include <cstddef>
#include <cstdlib>
#include <cstring>
namespace arg_parser {
#endif // end if c++

//default_callback_manager

inline void PARSABLE_ARGUMENTS_NAME(char * arg_name, void * callback_or_arg_value, size_t argc = 0, size_t* start = NULL){
 //argument_start, argument_end, start, argc;
  static void** args = (void **)malloc(sizeof(void *)*1024*4);
  static size_t total = 1024;
  static size_t filled = 0;
  if (!arg_name) {return;}
  const size_t len = strlen(arg_name);
  if (start == NULL){
    if (filled == total) args = (void**)realloc((void*) args, (total += 1024)*sizeof(void *)*4);
    args[ filled*4 ] = arg_name;
    args[filled*4+1] = (void *)len;
    strncpy((char *)(args+filled*4+2), arg_name, len>sizeof(void *)?sizeof(void*):len);
    args[filled*4+3] = callback_or_arg_value;
    ++filled;
  }else {
    void *tocomp;
    strncpy((char*)&tocomp, arg_name, len>sizeof(void *)?sizeof(void*):len);
    char **args_vals = (char **)malloc(sizeof(char *)*1024);
    size_t args_vals_filled = 1, args_vals_total = 1023;// not 1024, to keep 1 space empty at the end for null
    args_vals[0] = (char *)callback_or_arg_value;
    if (callback_or_arg_value){
      for (char* vals=(char *)callback_or_arg_value; *vals !='\0'; vals++) {
        if (*vals == ',') {
          *vals = '\0';
          if (args_vals_filled == total) args_vals = (char**)realloc((void*) args_vals, (args_vals_total += 1024)*sizeof(char **));
          args_vals[args_vals_filled] = ++vals;
        }
      }
      args_vals[args_vals_filled] = NULL;
    }
    for (size_t i = 0; i < filled; i++){
      if ((void *)len==args[i*4+1]){
        tocomp = NULL;
        strncpy((char *)&tocomp, arg_name, len>sizeof(void *)?sizeof(void*):len);
        if (tocomp==args[i*4+2]){
          if (len<=sizeof(void*)? true: 0==strcmp(arg_name-sizeof(void*),(char*)args[i*4]-sizeof(void*))){
            ( (void(*)(char*, char**, size_t, size_t))args[i*4+3] )(arg_name, args_vals, argc, *start);
            break;
          }
        }
      }
    }
    free(args_vals);
  }
}

inline void PARSE_ARGUMENTS_NAME(const size_t argc, char** argv) {
  char *argument_start = *argv, *argument_end;
  size_t start = 0;
  while (start < argc) {
    argument_start = argv[start++];
    argument_end = argument_start;
    if (argument_start[0] == '-') {
      for (; *argument_end != '\0' && *argument_end != '='; argument_end++);// argument can be like ['--foo=bar'] or ['--foo', 'bar']
      if (argument_start[1] == '-') {
        PARSABLE_ARGUMENTS_NAME(argument_start+2, (*argument_end? (*argument_end ='\0',argument_end+1): start<argc?argv[start++]:NULL), argc, &start);
      }else {
        char next = 0[++argument_start];
        for(; argument_start < argument_end; ++argument_start){
          char *arg_val = (argument_start[1]=='='? (argument_start[1]='\0',argument_start+2): (!argument_start[1]&&start<argc)?argv[start++]:NULL);
          argument_start[0] = next;next = argument_start[1];argument_start[1] = '\0';
          PARSABLE_ARGUMENTS_NAME(argument_start, arg_val, argc, &start);
        }
      }
    }else {
      PARSABLE_ARGUMENTS_NAME(NULL, argument_start, argc, &start);
    }
  }
}


#ifdef __cplusplus // if c++
}; // namespace arg_parser
#endif // end if c++

