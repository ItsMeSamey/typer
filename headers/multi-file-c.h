#pragma once

#include <cstring>
#ifndef __cplusplus // if !c++
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

void multi_file_write_oprations(multi_file_struct *STRUCT, uint64_t OPERATION = 0, void * DATA = NULL, uint64_t INDEX = 0,uint64_t len = 0){
  const uint64_t INIT=0, FREEABLE=1, NONFREE=2, DEINIT=3;
  uint64_t i, tell;
  FILE *multi_file;
  multi_file_struct *_cur, *cur;
  char *buff;
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
      *(uint64_t *)(((size_t)_cur->next) + sizeof(multi_file_struct)) = strlen((char*)DATA);
      _cur->next = cur->next;
      *(uint8_t *)((char*)cur->next + sizeof(multi_file_struct) + sizeof(size_t)) = 0;
      ((multi_file_struct*)cur->next)->next = NULL;
      break;
    case DEINIT:
      multi_file = fopen((char*)STRUCT->data, "w");
      assert(multi_file);
      fprintf(multi_file, "%p", (void*)(*(uint64_t*)STRUCT->next));
      buff = (char *)malloc(sizeof(char) * 36L*(*(uint64_t*)STRUCT->next));
//       fwrite(buff, sizeof(char), 36L*(*(uint64_t*)STRUCT->next), multi_file);
      fseek(multi_file, 18L+36L*(*(uint64_t*)STRUCT->next),SEEK_CUR);
      for (i = 0; i < *(uint64_t*)STRUCT->next; i++){
        _cur = &((multi_file_struct*)(2*sizeof(uint64_t)+(char*)STRUCT->next))[i];
        cur = (multi_file_struct*)(_cur->data);
        sprintf((buff + 36L*i), "%p", (void*)(i+1));
        sprintf((buff + 36L*i+18L), "%p", (void*)ftell(multi_file));
        while (cur != _cur->next){
          fwrite(cur->data, sizeof(char), *(size_t*)(((size_t)cur) +sizeof(multi_file_struct)), multi_file);
          if ( *(uint8_t *)(((size_t)cur) +sizeof(multi_file_struct) + sizeof(size_t)) ) free(cur->data);
          void* to_be_freed = (void*)cur;
          cur = (multi_file_struct*)cur->next;
          free(to_be_freed);
        }
      }
      fseek(multi_file,18L,SEEK_SET);
      fwrite(buff, sizeof(char), 36L*(*(uint64_t*)STRUCT->next), multi_file);
      fclose(multi_file); /* close the file */
      break;
    default: assert(!"operation was invalid");
  }
}



void write_file_and_stuff(char * file_name){
  size_t index = 0;

//   fseek(options_file,0L,SEEK_SET);
//   fread(buff, 1, length_of_file, options_file);
}



