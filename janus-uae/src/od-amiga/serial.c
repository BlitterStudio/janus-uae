/************************************************************************ 
 *
 * Copyright 2017 Oliver Brunner - aros<at>oliver-brunner.de
 *
 * This file is part of Janus-UAE.
 *
 * Janus-UAE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Janus-UAE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Janus-UAE. If not, see <http://www.gnu.org/licenses/>.
 *
 ************************************************************************
 *
 * serial port emulation
 *
 * just a quick hack, sorry
 *
 * enable serial output to file with "serial_filename=" in the used uaerc,
 * serial_filename defaults to "NIL:" otherwise and disables output
 * 
 ************************************************************************
 *
 * $Id$
 *
 ************************************************************************/

#include "j.h"
#include "od-amiga/serial.h"

//#define DebOut(...) do { kprintf("%s:%d  %s(): ",__FILE__,__LINE__,__func__);kprintf(__VA_ARGS__); } while(0)
#define DebOut(...) 

static BPTR serial_file;

int checkserwrite(void) {
  if(serial_file) {
    //DebOut("check ok\n");
    return 1;
  }
  DebOut("check failed\n");
  return 0;
}

void writeser(uae_u16 data) {
  DebOut("entered(data: %d)\n", data);
  if(serial_file) {
    FPutC(serial_file, data);
    Flush(serial_file);
  }
}

int readseravail(void) {
  DebOut("entered\n");
  return 0;
}

int readser(int *data) {
  DebOut("entered\n");
  return 0;
}


void setbaud(int rate) {
  DebOut("new rate: %d\n", rate);
}

void setserstat(int state, int foo) {
  DebOut("entered\n");
}

int openser(char *name) {

  DebOut("using serial output file: %s\n", name);
  write_log ("Serial output file: \"%s\"\n", name);
  
  if(serial_file) {
    closeser();
  }

  serial_file=Open(name, MODE_OLDFILE);
  if(serial_file == BNULL) {
    serial_file=Open(name, MODE_NEWFILE);
  }

  if(serial_file != BNULL) {
    /* append */
    Seek(serial_file, 0, OFFSET_END);
    DebOut("Serial file %s opened\n", name);
    return 1;
  }

  DebOut("ERROR: could not open %s\n", name);
  return 0;
}

void closeser(void) {
  DebOut("entered\n");
  if(serial_file) {
    FPutC(serial_file, '\n');
    FPutC(serial_file, '\n');
    Flush(serial_file);
    Close(serial_file);
    serial_file=NULL;
    DebOut("serial_file closed\n");
  }
}

void getserstat(int *status) {
  DebOut("entered\n");
}

void serialuartbreak(int v) {
  DebOut("entered\n");
}





