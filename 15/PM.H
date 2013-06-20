/*
  PMode tutorials in C and Asm
  Copyright (C) 2000 Alexei A. Frounze
  The programs and sources come under the GPL 
  (GNU General Public License), for more information
  read the file gnu-gpl.txt (originally named COPYING).
*/

#ifndef _pm_h_
#define _pm_h_

#include "pm_defs.h"

unsigned int    read_msw();
unsigned long   read_cr0();
void            write_cr0 (unsigned long value);
unsigned long   read_cr3();
void            write_cr3 (unsigned long value);

void            lgdt (GDTR *gdtr);
void            lidt (IDTR *idtr);
void            lldt (unsigned int selector);

void            ltr (unsigned int selector);
unsigned int    str();
void            clts();
void            jump_to_tss (unsigned int selector);

void            update_cs (unsigned int bew_cs);
void            load_fs (unsigned int fs);
void            load_gs (unsigned int gs);

#endif
