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

#define byte unsigned char
#define word unsigned int

byte read_CMOS_reg (byte reg) {
  outportb (0x70, reg);
  return inportb (0x71);
}

void write_CMOS_reg (byte reg, byte value) {
  outportb (0x70, reg);
  outportb (0x71, value);
}

void delay_RTC (int secs) {
  byte x;
  while (secs--) {
    x = read_CMOS_reg(0);               /* read seconds from RTC */
    while (read_CMOS_reg(0) == x);      /* wait for the next value */
  };
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
  unsigned long far *BIOS_timer = MK_FP (0x40, 0x6C);

  clrscr();
  printf ("Welcome to the 1st PMode tutorial!\n\n");
  atexit (my_exit);

  if (read_msw() & 1) {
    printf ("The CPU is already in PMode.\nAborting...");
    return 0;
  };

  printf ("We're going to PMode using CR0 for 5 seconds...\n");

  /* disable interrupts so that IRQs don't cause exceptions */
  disable();

  /* disable NMIs as well */
  outportb (0x70, inportb(0x70) | 0x80);

  /* WOW!!! This switches us to PMode just setting up CR0.PM bit to 1 */
  write_cr0 (read_cr0() | 1L);

  /* a delay for 5 seconds */
  delay_RTC (5);

  /* get out of PMode clearing CR0.PM bit to 0 */
  write_cr0 (read_cr0() & 0xFFFFFFFEL);

  *BIOS_timer += 91L; /* 5*18.2 ticks total */

  /* enable NMIs */
  outportb (0x70, inportb(0x70) & 0x7F);

  /* enabling interrupts */
  enable();

  return 0;
}
