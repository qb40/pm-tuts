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

extern interrupt isr_00_wrapper();	/* exception #0 -- division by zero */
extern interrupt isr_20_wrapper();	/* just an ISR */

DESCR_SEG gdt[5];			/* GDT */
GDTR gdtr;				/* GDTR */

DESCR_INT idt[0x21];			/* IDT */
IDTR idtr;				/* IDTR */

word old_CS, old_DS, old_SS;

char msg[]="H e l l o   f r o m   P M o d e   I S R ! ";
char msg0[]="DOiOvO ObOyO O0O OEOxOcOeOpOtOiOoOnO!O";

void setup_GDT_entry (DESCR_SEG *item,
                      dword base, dword limit, byte access, byte attribs) {
  item->base_l = base & 0xFFFF;
  item->base_m = (base >> 16) & 0xFF;
  item->base_h = base >> 24;
  item->limit = limit & 0xFFFF;
  item->attribs = attribs | ((limit >> 16) & 0x0F);
  item->access = access;
}

void setup_IDT_entry (DESCR_INT *item,
                      word selector, dword offset, byte access, byte param_cnt) {
  item->selector = selector;
  item->offset_l = offset & 0xFFFF;
  item->offset_h = offset >> 16;
  item->access = access;
  item->param_cnt = param_cnt;
}

void shut_down() {
  /* get out of PMode clearing CR0.PM bit to 0 */
  write_cr0 (read_cr0() & 0xFFFFFFFEL);

  /* restoring real mode segment values */
  update_cs (old_CS);
  _ES = _DS = old_DS;
  _SS = old_SS;

  idtr.base = 0;
  idtr.limit = 0x3FF;
  lidt (&idtr);

  /* enable NMIs */
  outportb (0x70, inportb(0x70) & 0x7F);

  /* enabling interrupts */
  enable();
}

void isr_00() {
  char far *scr;
  char *p, c;

  /* writing a message to the screen */
  scr = MK_FP (0x20, 80*6);             /* selector for segment 0xB800 is 0x20 */
  p = msg0;
  for(;;) {
    c = *p++;
    if (c) *scr++=c;
    else break;
  }
  shut_down();
  exit (0);
}

void isr_20() {
  char far *scr;
  char *p, c;

  /* writing a message to the screen */
  scr = MK_FP (0x20, 80*4);             /* selector for segment 0xB800 is 0x20 */
  p = msg;
  for(;;) {
    c = *p++;
    if (c) *scr++=c;
    else break;
  }
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
  int zero=0;

  clrscr();
  printf ("Welcome to the 3rd PMode tutorial!\n\n\n");
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

  /* setting up the exception #0 handler and Int 0x20 ISR */
  setup_IDT_entry (&idt[0x00], 0x08, (word)&isr_00_wrapper, ACS_INT, 0);
  setup_IDT_entry (&idt[0x20], 0x08, (word)&isr_20_wrapper, ACS_INT, 0);

  /* setting up the IDTR register */
  idtr.base = ((dword)_DS)<<4;
  idtr.base += (word)&idt;
  idtr.limit = sizeof(idt)-1;
  lidt (&idtr);

  /* WOW!!! This switches us to PMode just setting up CR0.PM bit to 1 */
  write_cr0 (read_cr0() | 1L);

  /* loading segment registers with PMode selectors */
  update_cs (0x08);
  _ES = _DS = 0x10;
  _SS = 0x18;

  /* invoking our ISR */
  __emit__ (0xCD, 0x20);                /* Int 0x20 */

  /* causing an exception */
  zero = 1 / zero;                      /* try to remove this line */

  shut_down();

  printf ("\nzero=%d\n", zero);

  return 0;
}
