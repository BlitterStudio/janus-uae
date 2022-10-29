#include <stdio.h>
#include <stdint.h>
#include "mpeg2.h"
#include "mpeg2convert.h"
#include "mpeg2_internal.h"


static mpeg2dec_t *mpeg_decoder;
  
int main(void) {

  mpeg2_state_t mpeg_state = mpeg2_parse(mpeg_decoder);
  printf("foo!\n");

  return 0;
}
