#pragma once

#include <cstring>
#ifndef __cplusplus // if !c++
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#endif // end if !c++

#ifdef __cplusplus // if c++
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cassert>
#endif // end if c++

typedef struct multi_file_struct {
  void* data = NULL;/*stores filename/ data to be written*/
  void* next = NULL;/*stores indices array/ pointer to next element*/
} multi_file_struct;

void multi_file_write_oprations(multi_file_struct *STRUCT, uint64_t OPERATION = 0, void * DATA = NULL, uint64_t INDEX = 0, uint64_t len = 0){
  const uint64_t INIT=0, FREEABLE=1, NONFREE=2, DEINIT=3;
  uint64_t i, tell;
  FILE *multi_file;
  multi_file_struct *_cur, *cur;
  uint64_t *buff;
  switch(OPERATION){
    case INIT:
      for (i = 0; i < INDEX; i++){
        if (STRUCT->next == NULL){
          STRUCT->data = (void *)DATA;
          STRUCT->next = malloc(sizeof(uint64_t)*2 + sizeof(multi_file_struct)*1024);
          *(uint64_t *)STRUCT->next = 0; /*num of active indices*/
          *(uint64_t *)((size_t)STRUCT->next+sizeof(uint64_t)) = 1024; /*num of total indices*/
          --i;
        } else {
          if (*(uint64_t *)STRUCT->next == *(uint64_t *)((size_t)STRUCT->next + sizeof(uint64_t)))
            STRUCT->next = realloc(STRUCT->next, (*(uint64_t*)((size_t)STRUCT->next+ sizeof(uint64_t))+=1024) * sizeof(multi_file_struct));
          cur = &((multi_file_struct*)((size_t)STRUCT->next + 2*sizeof(uint64_t))) [(*(uint64_t *)(STRUCT->next))];
          cur->data = malloc(sizeof(multi_file_struct) + sizeof(size_t) + 1);
          cur->next = cur->data;
          (*(uint64_t *)(STRUCT->next))+=1;
          *(uint8_t *)((size_t)cur->next + sizeof(multi_file_struct) + sizeof(size_t)) = 0;
          ((multi_file_struct*)cur->next)->next = NULL;
        }
      } break;
    case FREEABLE:
      *(uint8_t *)( (size_t)(((multi_file_struct*)(2*sizeof(uint64_t)+(size_t)STRUCT->next))[INDEX].next) + sizeof(multi_file_struct) + sizeof(size_t)) = 1;
      break;
      /*[[fallthrough]]*/
    case NONFREE:
      _cur = &((multi_file_struct*)(2*sizeof(uint64_t)+(char*)STRUCT->next))[INDEX];
      cur = (multi_file_struct*)_cur->next;
      cur->data = (void*)DATA;
      cur->next = malloc(sizeof(multi_file_struct) + sizeof(size_t) + 1);
      *(uint64_t *)(((size_t)_cur->next) + sizeof(multi_file_struct)) = len ? len :strlen((char*)DATA);
      _cur->next = cur->next;
      *(uint8_t *)((char*)cur->next + sizeof(multi_file_struct) + sizeof(size_t)) = 0;
      ((multi_file_struct*)cur->next)->next = NULL;
      break;
    case DEINIT:
      multi_file = fopen((char*)STRUCT->data, "w");
      assert(multi_file);
      fwrite((uint64_t*)STRUCT->next, sizeof(uint64_t), 1, multi_file);
      fseek(multi_file, sizeof(uint64_t), SEEK_SET);
      buff = (uint64_t *)malloc(sizeof(uint64_t) * (*(uint64_t*)STRUCT->next));

      for (i = 0; i < *(uint64_t*)STRUCT->next; i++){
        _cur = &((multi_file_struct*)(2*sizeof(uint64_t)+(char*)STRUCT->next))[i];
        cur = (multi_file_struct*)(_cur->data);
        buff[i] = ftell(multi_file);
        while (cur != _cur->next){
          fwrite(cur->data, sizeof(char), *(size_t*)(((size_t)cur) +sizeof(multi_file_struct)), multi_file);
          if ( *(uint8_t *)(((size_t)cur) +sizeof(multi_file_struct) + sizeof(size_t)) ) free(cur->data);
          void* to_be_freed = (void*)cur;
          cur = (multi_file_struct*)cur->next;
          free(to_be_freed);
        }
      }
      fwrite(buff, sizeof(uint64_t), (*(uint64_t*)STRUCT->next), multi_file);
      free(buff);
      free(_cur->next);
      fclose(multi_file); /* close the file */
      break;
    default: assert(!"operation was invalid");
  }
}
char **multi_file_write_oprations(char * file_name){
  FILE *multi_file = fopen(file_name, "r");
  assert(multi_file);
  if (!multi_file) return NULL;
  uint64_t num;
  fseek(multi_file,0L,SEEK_SET);
  fread(&num, sizeof(uint64_t), 1, multi_file);

  fseek(multi_file, -num,SEEK_END);
  const size_t size = ftell(multi_file);
  char *buff = (char *)malloc(size + (num+2) *sizeof(uint64_t));
  fseek(multi_file, sizeof(uint64_t), SEEK_SET);
  fread(buff, sizeof(char), size, multi_file);

  *(void**)(buff+size) = buff;
  void **start = ((void**)(buff+size)) + 1;
  fseek(multi_file, -num,SEEK_END);
  fread(start, sizeof(uint64_t), num, multi_file);

  for (uint64_t i = 0; i < num; i++){start[i] = buff + ((uint64_t*)start)[i];}
  start[num] = NULL;
  fclose(multi_file); /* close the file */
  return (char **)start;
}


