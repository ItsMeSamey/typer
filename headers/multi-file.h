#pragma once

#include <cstring>
#ifndef __cplusplus // if !c++
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#endif // end if !c++

#ifdef __cplusplus // if c++
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#endif // end if c++


typedef struct multi_file_struct {
  void* file = NULL;/*stores filename/ data to be written*/
  void* next = NULL;/*stores indices array/ pointer to next element*/
} multi_file_struct;

void multi_file_init(multi_file_struct *STRUCT, uint64_t INDEX, FILE *FILEPTR = NULL){
  multi_file_struct *cur;
  uint64_t i;
  if (STRUCT->next == NULL){
    STRUCT->next = malloc(sizeof(uint64_t)*2 + sizeof(multi_file_struct)*1024);
    *(uint64_t *)STRUCT->next = 0; /*num of active indices*/
    *(uint64_t *)((size_t)STRUCT->next+sizeof(uint64_t)) = 1024; /*num of total indices*/
  }
  if (FILEPTR) STRUCT->file = (void *)FILEPTR;
  for (i = 0; i < INDEX; i++){
    if (*(uint64_t *)STRUCT->next == *(uint64_t *)((size_t)STRUCT->next + sizeof(uint64_t)))
      STRUCT->next = realloc(STRUCT->next, (*(uint64_t*)((size_t)STRUCT->next+ sizeof(uint64_t))+=1024) * sizeof(multi_file_struct));
    cur = &((multi_file_struct*)((size_t)STRUCT->next + 2*sizeof(uint64_t))) [(*(uint64_t *)(STRUCT->next))];
    cur->file = malloc(sizeof(multi_file_struct) + sizeof(size_t) + 1);
    cur->next = cur->file;
    (*(uint64_t *)(STRUCT->next))+=1;
    *(uint8_t *)((size_t)cur->next + sizeof(multi_file_struct) + sizeof(size_t)) = 0;
    ((multi_file_struct*)cur->next)->next = NULL;
  }
}
void multi_file_sub_file_init(multi_file_struct *PARENT, uint64_t INDEX, multi_file_struct *CHILD){
  multi_file_struct *_cur = INDEX + (multi_file_struct *)((size_t)PARENT->next + sizeof(uint64_t)*2);
  FILE *f = open_memstream( (char **)(&((multi_file_struct *)_cur->next)->file), (size_t *)((size_t)_cur->next + sizeof(multi_file_struct)));
  *(uint8_t *)( (size_t)(_cur->next) + sizeof(multi_file_struct) + sizeof(size_t)) = 1;
  multi_file_init(CHILD, 0, f);
}
FILE *multi_file_init(multi_file_struct *STRUCT, uint64_t INDEX, char *FILE_NAME){
  FILE *f = fopen(FILE_NAME, "w");
  fseek(f, 0L, SEEK_SET);
  multi_file_init(STRUCT, INDEX, f);
  return f;
}
void multi_file_write(multi_file_struct *STRUCT, uint64_t INDEX = 0,char *DATA = NULL, uint64_t LEN = 0, char OPERATION = 0){
  const char COPY=2, FREEABLE=1, NONFREE=0;
  uint64_t i, tell;
  FILE *multi_file;
  void *temp;
  multi_file_struct *_cur = ((multi_file_struct*)(2*sizeof(uint64_t)+(char*)STRUCT->next)) + INDEX;
  multi_file_struct *cur = (multi_file_struct*)_cur->next;
  LEN = LEN ? LEN :strlen((char*)DATA);
  switch(OPERATION){
    case COPY:
      temp = DATA; DATA = (char *)malloc(LEN * sizeof(char));memcpy(DATA, temp, sizeof(char)*LEN);
      /*[[fallthrough]]*/
    case FREEABLE:
      *(uint8_t *)( (size_t)(_cur->next) + sizeof(multi_file_struct) + sizeof(size_t)) = 1;
      /*[[fallthrough]]*/
    case NONFREE:
      cur->file = (void*)DATA;
      cur->next = malloc(sizeof(multi_file_struct) + sizeof(size_t) + 1);
      *(uint64_t *)(((size_t)_cur->next) + sizeof(multi_file_struct)) = LEN;
      _cur->next = cur->next;
      *(uint8_t *)((char*)cur->next + sizeof(multi_file_struct) + sizeof(size_t)) = 0;
      ((multi_file_struct*)cur->next)->next = NULL;
      break;
    default: throw "operation was invalid";
  }
}
void multi_file_flush(multi_file_struct *STRUCT){
  uint64_t *buff, i;
  FILE *multi_file;
  multi_file_struct *_cur, *cur;
  multi_file = (FILE*)STRUCT->file;
  buff = (uint64_t *)malloc(sizeof(uint64_t) * (*(uint64_t*)STRUCT->next));

  for (i = 0; i < *(uint64_t*)STRUCT->next; i++){
    _cur = &((multi_file_struct*)(2*sizeof(uint64_t)+(char*)STRUCT->next))[i];
    cur = (multi_file_struct*)(_cur->file);
    buff[i] = ftell(multi_file);
    while (cur != _cur->next){
      fwrite(cur->file, sizeof(char), *(size_t*)(((size_t)cur) +sizeof(multi_file_struct)), multi_file);
      if ( *(uint8_t *)(((size_t)cur) +sizeof(multi_file_struct) + sizeof(size_t)) ) free(cur->file);
      void* to_be_freed = (void*)cur;
      cur = (multi_file_struct*)cur->next;
      free(to_be_freed);
    }
  }
  fwrite(buff, sizeof(uint64_t), (*(uint64_t*)STRUCT->next), multi_file);
  fwrite((uint64_t*)STRUCT->next, sizeof(uint64_t), 1, multi_file);
  free(buff);
  free(_cur->next);
}
char **multi_file_read(char *BUFF, const size_t LEN){
  if (LEN < sizeof(uint64_t)) return NULL;
  uint64_t *num = (uint64_t *)(BUFF + LEN - sizeof(uint64_t));
  char **start = (char **)(((char *)BUFF) + LEN - sizeof(uint64_t)*(*num + 1));
  for (uint64_t i = 0; i < *num; i++){start[i] = ((char *)BUFF) + i[(uint64_t*)start];}
  start[*num] = NULL;
  return start;
}
void multi_file_read(multi_file_struct *MULTIFILE){
  if (MULTIFILE->next == NULL){
    if (!MULTIFILE->file){return;}
    FILE *f = fopen((char *)MULTIFILE->next, "w");
    if (!f){return;}
    fseek(f, 0L, SEEK_END);
    const size_t len = ftell(f);
    char *buff = (char *)malloc(len);
    fread(buff, 1, len, f);
    MULTIFILE->next = buff;
    MULTIFILE->file = buff + len;
    MULTIFILE->file = multi_file_read((char*)MULTIFILE->next, len);
  }else if ((MULTIFILE->next<MULTIFILE->file)&&(MULTIFILE->next<(*(void**)MULTIFILE->file)) ){
    char **temp = multi_file_read((char*)MULTIFILE->next, (*(char**)MULTIFILE->file)-(char*)MULTIFILE->next);
    if (temp) {MULTIFILE->file = temp;}
    else {MULTIFILE->file = NULL; MULTIFILE->next = NULL;}
  }
}

