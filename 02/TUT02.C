/*
  PMode tutorials in C and Asm
  Copyright (C) 2000 Alexei A. Frounze
  The programs and sources come under the GPL 
  (GNU General Public License), for more information
  read the file gnu-gpl.txt (originally named COPYING).
*/

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>

#include "pm.h"
#include "pm_defs.h"

DESCR_SEG gdt[5];			/* GDT */
GDTR gdtr;				/* GDTR */
word old_CS, old_DS, old_SS;

char msg[]="H e l l o   f r o m   P M o d e   w i t h   G D T ! ";

void setup_GDT_entry (DESCR_SEG *item, 
                      dword base, dword limit, byte access, byte attribs) {
  item->base_l = base & 0xFFFF;
  item->base_m = (base >> 16) & 0xFF;
  item->base_h = base >> 24;
  item->limit = limit & 0xFFFF;
  item->attribs = attribs | ((limit >> 16) & 0x0F);
  item->access = access;
}

void my_exit() {
  printf ("\nWe're back...\n");
  printf ("\nPMode Tutorial by Alexei A. Frounze (c) 2000\n");
  printf ("E-mail  : alexfru@chat.ru\n");
  printf ("Homepage: http://alexfru.chat.ru\n");
  printf ("Mirror  : http://members.xoom.com/alexfru\n");
  printf ("PMode...: http://welcome.to/pmode\n");
}

int main() {
  char far *scr;
  char *p, c;

  clrscr();
  printf ("Welcome to the 2nd PMode tutorial!\n\n");
  atexit (my_exit);

  if (read_msw() & 1) {
    printf ("The CPU is already in PMode.\nAborting...");
    return 0;
  };

  /* 0x00 -- null descriptor */
  setup_GDT_entry (&gdt[0], 0, 0, 0, 0);

  /* 0x08 -- code segment descriptor */
  setup_GDT_entry (&gdt[1], ((dword)_CS)<<4, 0xFFFF, ACS_CODE, 0);

  /* 0x10 -- data segment descriptor */
  setup_GDT_entry (&gdt[2], ((dword)_DS)<<4, 0xFFFF, ACS_DATA, 0);

  /* 0x18 -- stack segment descriptor */
  setup_GDT_entry (&gdt[3], ((dword)_SS)<<4, 0xFFFF, ACS_STACK, 0);

  /* 0x20 -- text video mode segment descriptor */
  setup_GDT_entry (&gdt[4], 0xB8000L, 0xFFFF, ACS_DATA, 0);

  /* disable interrupts so that IRQs don't cause exceptions */
  disable();

  /* disable NMIs as well */
  outportb (0x70, inportb(0x70) | 0x80);

  /* setting up the GDTR register */
  gdtr.base = ((dword)_DS)<<4; 
  gdtr.base += (word)&gdt;
  gdtr.limit = sizeof(gdt)-1;
  lgdt (&gdtr);

  /* saving real mode segment addresses */
  old_CS = _CS;
  old_DS = _DS;
  old_SS = _SS;

  /* WOW!!! This switches us to PMode just setting up CR0.PM bit to 1 */
  write_cr0 (read_cr0() | 1L);

  /* loading segment registers with PMode selectors */
  update_cs (0x08);
  _ES = _DS = 0x10;
  _SS = 0x18;

  /* writing a message to the screen */
  scr = MK_FP (0x20, 80*4);             /* selector for segment 0xB800 is 0x20 */
  p = msg;
  for(;;) {
    c = *p++;
    if (c) *scr++=c;
    else break;
  }

  /* get out of PMode clearing CR0.PM bit to 0 */
  write_cr0 (read_cr0() & 0xFFFFFFFEL);

  /* restoring real mode segment values */
  update_cs (old_CS);
  _ES = _DS = old_DS;
  _SS = old_SS;

  /* enable NMIs */
  outportb (0x70, inportb(0x70) & 0x7F);

  /* enabling interrupts */
  enable();

  return 0;
}
