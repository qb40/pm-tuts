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
#include <process.h>

#include "pm.h"
#include "pm_defs.h"
#include "isr_wrap.h"

#define stack_size      4096

word old_CS, old_DS, old_SS;
byte old_IRQ_mask[2];

DESCR_SEG gdt[0x1C];                    /* GDT  */
GDTR gdtr;                              /* GDTR */

DESCR_INT idt[0x30];                    /* IDT  */
IDTR idtr;                              /* IDTR */

TSS     tss_v86;
_TSS    tss_main,
        tss_gpf,
        tss_irq[0x10],
        tss_debug;

byte    v86_pl0_stack[stack_size];      /* pl#0 stacks for v86() */

byte    gpf_stack[stack_size];
byte    irq_stack[stack_size];
byte    debug_stack[stack_size];

void    v86();                          /* function prototype */

int     asterisk_time=0,
        asterisk=0;

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

void setup_GDT() {
  dword cbase, dbase, sbase;
  word i;

  cbase = ((dword)_CS)<<4;
  dbase = ((dword)_DS)<<4;
  sbase = ((dword)_SS)<<4;

  /* 0x00 -- null descriptor */
  setup_GDT_entry (&gdt[0], 0, 0, 0, 0);

  /* 0x08 -- code segment descriptor */
  setup_GDT_entry (&gdt[1], cbase, 0xFFFF, ACS_CODE, 0);

  /* 0x10 -- data segment descriptor */
  setup_GDT_entry (&gdt[2], dbase, 0xFFFF, ACS_DATA, 0);

  /* 0x18 -- stack segment descriptor */
  setup_GDT_entry (&gdt[3], sbase, 0xFFFF, ACS_STACK, 0);

  /* 0x20 -- text video mode segment descriptor */
  setup_GDT_entry (&gdt[4], 0xB8000L, 0xFFFF, ACS_DATA | ACS_DPL_3, 0);

  /* 0x28 -- descriptor for floating data segments */
  setup_GDT_entry (&gdt[5], 0L, 0xFFFF, ACS_DATA, 0x0F | ATTR_GRANULARITY);

  /* 0x30 -- TSS for main() */
  setup_GDT_entry (&gdt[6], dbase+(word)&tss_main, sizeof(_TSS), ACS_TSS, 0);

  /* 0x38 -- TSS for v86() */
  setup_GDT_entry (&gdt[7], dbase+(word)&tss_v86, sizeof(TSS), ACS_TSS, 0);

  /* 0x40 -- TSS for gpf_handler() */
  setup_GDT_entry (&gdt[8], dbase+(word)&tss_gpf, sizeof(_TSS), ACS_TSS, 0);

  /* 0x48 ... 0x -- TSSes for IRQs */
  for (i=0;i<0x10;i++)
    setup_GDT_entry (&gdt[9+i], dbase+(word)&tss_irq[i], sizeof(_TSS), ACS_TSS, 0);

  /* 0xC8 -- 2nd descriptor for floating data segments */
  setup_GDT_entry (&gdt[25], 0L, 0xFFFF, ACS_DATA, 0x0F | ATTR_GRANULARITY);

  /* 0xD0 -- 3rd descriptor for floating data segments */
  setup_GDT_entry (&gdt[26], 0L, 0xFFFF, ACS_DATA, 0x0F | ATTR_GRANULARITY);

  /* 0xD8 -- TSS for debug_handler() */
  setup_GDT_entry (&gdt[27], dbase+(word)&tss_debug, sizeof(_TSS), ACS_TSS, 0);

  /* setting up the GDTR register */
  gdtr.base = dbase+(word)&gdt;
  gdtr.limit = sizeof(gdt)-1;
  lgdt (&gdtr);
}

word seg2sel (word seg) {
  gdt[5].base_l = seg<<4;
  gdt[5].base_m = seg>>12;
  gdt[5].base_h = 0;
  return 5*8;
}

void setup_IDT() {
  word i, j;

  setup_IDT_entry (&idt[0], 0x08, wrappers[0], ACS_INT, 0);

  idt[1].selector = 0xD8;
  idt[1].access = ACS_TASK;

  for (i=2;i<0x0D;i++)
    setup_IDT_entry (&idt[i], 0x08, wrappers[i], ACS_INT, 0);

  idt[0x0D].selector = 0x40;
  idt[0x0D].access = ACS_TASK;

  for (i=0x0E;i<0x20;i++)
    setup_IDT_entry (&idt[i], 0x08, wrappers[i], ACS_INT, 0);

  for (i=0x20,j=0x48; i<0x30; i++,j+=8) {
    idt[i].selector = j;
    idt[i].access = ACS_TASK;
  }

  /* setting up the IDTR register */
  idtr.base = ((dword)_DS)<<4;
  idtr.base += (word)&idt;
  idtr.limit = sizeof(idt)-1;
  lidt (&idtr);
}

void setup_PIC (byte master_vector, byte slave_vector) {
  outportb (PORT_8259M, 0x11);                  /* start 8259 initialization */
  outportb (PORT_8259S, 0x11);
  outportb (PORT_8259M+1, master_vector);       /* master base interrupt vector */
  outportb (PORT_8259S+1, slave_vector);        /* slave base interrupt vector */
  outportb (PORT_8259M+1, 1<<2);                /* bitmask for cascade on IRQ2 */
  outportb (PORT_8259S+1, 2);                   /* cascade on IRQ2 */
  outportb (PORT_8259M+1, 1);                   /* finish 8259 initialization */
  outportb (PORT_8259S+1, 1);
}

void setup_PMode() {
  word i, j;

  memset (&tss_v86, 0, sizeof(TSS));
  tss_v86.io_map_addr = sizeof(_TSS);           /* I/O map just after the TSS */

  memcpy (&tss_main, &tss_v86, sizeof(_TSS));

  for (i=0;i<0x10;i++) {
    tss_irq[i] = tss_main;
    tss_irq[i].cs = 0x08;
    tss_irq[i].ss = tss_irq[i].es = tss_irq[i].ds = 0x10;
    tss_irq[i].eflags = 0x2L;
    tss_irq[i].eip = wrappers[0x20+i];
    tss_irq[i].esp = (word)&irq_stack[stack_size];
  };

  tss_gpf = tss_irq[0];
  tss_gpf.eip = wrappers[0x0D];
  tss_gpf.esp = (word)&gpf_stack[stack_size];

  tss_debug = tss_irq[0];
  tss_debug.eip = wrappers[1];
  tss_debug.esp = (word)&debug_stack[stack_size];

  tss_v86.cs = _CS;
  tss_v86.es = tss_v86.ds = _DS;
  tss_v86.eflags = 0x23202L;
  tss_v86.eip = (word)&v86;
  tss_v86.ss0 = 0x10;
  tss_v86.esp0 = (word)&v86_pl0_stack[stack_size];

  /* disable interrupts so that IRQs don't cause exceptions */
  disable();

  /* disable NMIs as well */
  outportb (0x70, inportb(0x70) | 0x80);

  /* setup GDT */
  setup_GDT();

  /* setup IDT */
  setup_IDT();

  /* save IRQ masks */
  old_IRQ_mask[0] = inportb (PORT_8259M+1);
  old_IRQ_mask[1] = inportb (PORT_8259S+1);

  /* setup PIC */
  setup_PIC (0x20, 0x28);

  /* set new IRQ masks */
  outportb (PORT_8259M+1, old_IRQ_mask[0]);
  outportb (PORT_8259S+1, old_IRQ_mask[1]);

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

  /* if we don't load fs and gs with valid selectors, task switching may fail. */
  load_fs (0x10);
  load_gs (0x10);

  lldt (0);

  /* load the TR register */
  ltr (0x30);
}

void shut_down() {
  /* clear CR0.TS flag so that DPMI programs can normally startup 
     after this tut terminates. */
  clts();

  /* load fs and gs with selectors of 64KB segments */
  load_fs (0x10);
  load_gs (0x10);

  /* get out of PMode clearing CR0.PM bit to 0 */
  write_cr0 (read_cr0() & 0xFFFFFFFEL);

  /* restoring real mode segment values */
  update_cs (old_CS);
  _ES = _DS = old_DS;
  _SS = old_SS;

  idtr.base = 0;
  idtr.limit = 0x3FF;
  lidt (&idtr);

  /* setup PIC */
  setup_PIC (8, 0x70);

  /* restore IRQ masks */
  outportb (PORT_8259M+1, old_IRQ_mask[0]);     /* master */
  outportb (PORT_8259S+1, old_IRQ_mask[1]);     /* slave */

  /* enable NMIs */
  outportb (0x70, inportb(0x70) & 0x7F);

  /* enabling interrupts */
  enable();
}

void exc_handler (word exc_no, word cs, dword ip, word error) {
  word tr;

  tr=str();
  shut_down();

  textbackground (RED); textcolor (WHITE);
  clreol();
  printf ("exception no: %02XH\n", exc_no);
  clreol();
  printf ("at address  : %04XH:%08XH\n", cs, ip);
  if (exc_has_error[exc_no]) {
    clreol();
    printf ("error code  : %04XH [ Index:%04XH, Type:%d ]\n",
            error, error >> 3, error & 7);
  }
  clreol();
  printf ("TR: %04XH\n", tr);

  exit (0);
}

void gpf_handler() {
  byte far *pb;
  word far *pw;
  word vector, op_len;
  word x;
  DESCR_SEG far *pds;
  DESCR_SEG tmp;
  word count;
  word far *src, far *dst;

  /* code for virtual 8086 mode */

  if (((tss_v86.eflags & 0x20000L) == 0x20000L) && (tss_gpf.link == 0x38)) {
    pb = MK_FP (seg2sel((word)tss_v86.cs), (word)tss_v86.eip);

    /* simulate the Int nn instruction */

    switch (pb[0]) {
      case 0xCC:
        vector = 3;
        op_len = 1;
      break;
      case 0xCD:
        vector = (word)pb[1];
        op_len = 2;
      break;
      case 0xCE:
        vector = 4;
        op_len = 1;
      break;
      default:
        goto lcrash;
    }

    if (vector == 0x15) {
      x = (word)tss_v86.eax;
      if ((x >> 8) == 0x87) {
        pds = MK_FP (seg2sel((word)tss_v86.es), (word)tss_v86.esi);
        gdt[25].base_l = pds[2].base_l;
        gdt[25].base_m = pds[2].base_m;
        gdt[25].base_h = pds[2].base_h;
        gdt[26].base_l = pds[3].base_l;
        gdt[26].base_m = pds[3].base_m;
        gdt[26].base_h = pds[3].base_h;
        src = MK_FP (0xC8, 0);
        dst = MK_FP (0xD0, 0);
        count = (word)tss_v86.ecx;
        while (count--) *dst++ = *src++;        /* transferr the data */
        (word)tss_v86.eax = (byte)tss_v86.eax;  /* AH = 0 */
        tss_v86.eflags &= 0xFFFFFFFEL;          /* clear EFLAGS.CF (CARRY) */
        tss_v86.eflags |= 0x00000040L;          /* set EFLAGS.ZF (ZERO) */
        tss_v86.eip += 2;                       /* adjust IP */
        return;
      }
    }

    /* adjust SP for FLAGS, CS and IP (6 bytes) */
    tss_v86.esp -= 6;

    /* push FLAGS, CS, IP like Int does */
    pw = MK_FP (seg2sel((word)tss_v86.ss), (word)tss_v86.esp);
    pw[0] = (word)tss_v86.eip+op_len;
    pw[1] = (word)tss_v86.cs;
    pw[2] = (word)tss_v86.eflags;

    /* change CS:IP according to the specified vector */
    pw = MK_FP (seg2sel(0), vector<<2);
    tss_v86.eip = (word)pw[0];
    tss_v86.cs = (word)pw[1];

    /* clear IF and TF */
    tss_v86.eflags &= 0xFFFFFCFFL;

    /* return to the Virtual 8086 task */
    return;
  }

lcrash:

  shut_down();

  /* is it a quit using read_cr0() ? :) */

  if ((tss_gpf.link == 0x38) && 
      ((word)tss_v86.cs==_CS) && 
      ((word)tss_v86.eip==(word)&read_cr0)) exit (0);

  textbackground (RED); textcolor (WHITE);
  clreol();
  printf ("General Protection Fault\n");
  clreol();
  printf ("TR    : %04lXH\n", tss_gpf.link);

  if (tss_gpf.link == 0x38) {
    clreol();
    printf ("EFLAGS: %08lXH\n", tss_v86.eflags);
    clreol();
    printf ("CS:EIP: %04lXH:%08lXH\n", tss_v86.cs, tss_v86.eip);
  }

  exit(0);
}

void irq_handler (word irq) {
  word far *pw;
  word vector;
  char far *scr;

  if (irq==0) {
    if (++asterisk_time >= 9) {
      asterisk_time = 0;
      asterisk = 1-asterisk;
    }
    scr = MK_FP (0x20, 0);
    if (asterisk) scr[79*2]='*';
    else scr[79*2]=' ';
  }

  /* adjust SP for FLAGS, CS and IP (6 bytes) */
  tss_v86.esp -= 6;

  /* push FLAGS, CS, IP like Int does */
  pw = MK_FP (seg2sel((word)tss_v86.ss), (word)tss_v86.esp);
  pw[0] = (word)tss_v86.eip;
  pw[1] = (word)tss_v86.cs;
  pw[2] = (word)tss_v86.eflags;

  /* change CS:IP according to the specified vector */
  if (irq < 8) vector = 8+irq;
  else if (irq < 16) vector = 0x68+irq;
  else {
    vector = irq & 0xFF;
  }
  pw = MK_FP (seg2sel(0), vector<<2);
  tss_v86.eip = (word)pw[0];
  tss_v86.cs = (word)pw[1];

  /* clear IF and TF */
  tss_v86.eflags &= 0xFFFFFCFFL;
}

void v86() {
  printf ("Hello from virtual 8086 mode!\n\n");
  printf ("Type EXIT to quit V86 mode...\n\n");

  spawnv (P_WAIT, getenv("COMSPEC"), NULL);
  if (errno)
    printf ("DOS reports an error: %d\n", errno);

  read_cr0();
}

void my_exit() {
  printf ("\nWe're back...\n");
  printf ("\nPMode Tutorial by Alexei A. Frounze (c) 2000\n");
  printf ("E-mail  : alexfru@chat.ru\n");
  printf ("Homepage: http://alexfru.chat.ru\n");
  printf ("Mirror  : http://members.xoom.com/alexfru\n");
  printf ("PMode...: http://welcome.to/pmode\n");
}

void main() {
  clrscr();
  printf ("Welcome to the 16th PMode tutorial!\n\n");
  atexit (my_exit);

  if (read_msw() & 1) {
    printf ("The CPU is already in PMode.\nAborting...");
    return;
  };

  /* setting up pmode */
  setup_PMode();

  /* let's use current stack for v86() task */
  tss_v86.ss = old_SS;
  tss_v86.esp = _SP;

  /* switch to v86() task */
  jump_to_tss (0x38);
}
