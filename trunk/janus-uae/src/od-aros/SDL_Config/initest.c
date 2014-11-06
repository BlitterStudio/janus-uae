#include <stdio.h>
#include "SDL_config_lib.h"

CFG_File  config;
CFG_File  config2;

char filename[]="PROGDIR:test.ini";

void print_r(int r) {
  switch(r) {
    case CFG_OK: printf("CFG_OK\n"); break;
    case CFG_ENTRY_CREATED: printf("CFG_ENTRY_CREATED\n"); break;
    case CFG_ERROR: printf("CFG_ERROR\n"); break;
    default: printf("unknown return code: %d\n", r);
  }
}

int main (int argc, char **argv) {
  char str[1024];
  const char *res;
  int r;
  int ri;

  if (CFG_OK != CFG_OpenFile(filename, &config )) {
    printf("OK: %s does not exist\n", filename);
    if (CFG_OK != CFG_OpenFile(NULL, &config )) {
      printf("ERROR: failed to open NULL file\n");
      exit(1);
    }
    else {
      printf("OK: opened NULL file\n");
    }
  }
  else {
    printf("OK: %s opened\n", filename);
  }

  CFG_SelectGroup("mygroup1", true);
  printf("OK: select mygroup1\n");

  res=CFG_ReadText("mydata", "default value");
  printf("OK: res: %s\n", res);

  CFG_SelectGroup("mygroup2", true);
  printf("OK: select mygroup2\n");

  res=CFG_ReadText("mydata", "default value");
  printf("OK: res: %s\n", res);

  CFG_SelectGroup("mygroup3", true);
  printf("OK: select mygroup2\n");

  res=CFG_ReadText("mydata", "default value");
  printf("OK: res: %s\n");


  CFG_SelectGroup("mygroup3", true);
  printf("OK: select mygroup3\n");
  r=CFG_WriteInt("mydata", 123);
  printf("write mydata, 123: "); print_r(r);
  r=CFG_WriteInt("mydata", 321);
  printf("write mydata, 312: "); print_r(r);

  CFG_SaveFile(NULL, CFG_SORT_ORIGINAL, CFG_COMPRESS_OUTPUT);
  printf("OK: saved file\n");
  CFG_CloseFile(NULL);
  printf("OK: closed file\n");


  if (CFG_OK != CFG_OpenFile(filename, &config2)) {
    printf("ERROR: %s does not exist!!\n", filename);
    exit(1);
  }
  else {
    printf("OK: %s opened\n", filename);
  }


  CFG_SelectGroup("mygroup1", true);
  printf("OK: select mygroup1\n");

  res=CFG_ReadText("mydata", "default value");
  printf("res: %s\n", res);

  CFG_SelectGroup("mygroup2", true);
  printf("OK: select mygroup2\n");

  res=CFG_ReadText("mydata", "default value");
  printf("res: %s\n", res);

  CFG_SelectGroup("mygroup3", true);
  printf("OK: select mygroup3\n");

  ri=CFG_ReadInt("mydata", 666);
  printf("res: %d\n", ri);

  CFG_CloseFile(NULL);


}
