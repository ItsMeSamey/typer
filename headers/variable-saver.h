#pragma once

#ifndef __cplusplus // if !c++
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#endif // end if !c++

#ifdef __cplusplus // if c++
#include <cstdio>
#include <cstdlib>
#include <cstddef>
namespace variable_saver {
#endif // end if c++

struct variables_to_be_saved{
  char *name; char *value;
};

void save_variables_to_file(FILE *options_file, struct variables_to_be_saved *to_be_saved, size_t length){
  if (options_file == NULL) return; // Okay bro
  for (size_t i = 0; i < length; i++){
    fprintf(options_file, "%s = %s#\n", to_be_saved[i].name, to_be_saved[0].value); // write options to the file
  }
  // remove();
  // rename(, );
}
struct variables_to_be_saved* load_variables(FILE *options_file, size_t length_of_file){
  if (options_file == NULL) return NULL; // Handle this uslf
  size_t index = 0, array_size = 1024;
  struct variables_to_be_saved *array_to_return = (struct variables_to_be_saved *)malloc(array_size* sizeof(struct variables_to_be_saved));
  char *buff = (char*)malloc(sizeof(char) * length_of_file);
  char *buff_end = length_of_file + buff;
  char *i = buff;
  fread(buff, 1, length_of_file, options_file);

  for (; i < buff_end;){
    while ((*i == ' ' || *i == '\t' || *i == '\v') && i < buff_end) {i++;} // skip leading backspaces
    if (*i == '#') {while (*i != '\n' && i < buff_end) {i++;}} // if startswith '#', treat as comment i.e. ignore line
    else if (*i == '\n') {i++;} // skip line if line is an empty newline
    else{ // line has config variables set
      array_to_return[index].name = i; // store pointer to begining of variable name in `array_to_return`
      while (*i != '=' && i < buff_end){i++;} // move along until you see a '='
      char *j = i-1; // move i character before '='
      while (*j == ' ' || *j == '\t' || *j == '\v') {j--;} // check if variable ends with whitespace; move to last non-whotespace character of the variable
      *(j+1) = '\0'; // make `variable` end with '\0' i.e string termination
      i++; // move to the character after '='
      while ((*i == ' ' || *i == '\t' || *i == '\v') && i < buff_end) {i++;} // skip leading whitespace for `value`
      array_to_return[index].value = i; // store pointer to begining of `value` in `array_to_return`
      while ((*i != '\n' || *(i-1) != '#') && i < buff_end){i++;} // keep moving until you see '#' followed by a newline ('\n')
      *(i-1) = '\0'; // properly terminate the `value`; replace the last '#' with null-termination ('\0')
      i++;index++; // increase array index by one and
      if ((index > array_size) != 0){array_to_return = (struct variables_to_be_saved *)malloc((array_size*=2)* sizeof(struct variables_to_be_saved));} // reallocate array if it ran out of size
    }
  }
  return array_to_return;
}

#ifdef __cplusplus // if c++
}; // namespace arge parser
#endif // end if c++

