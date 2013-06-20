;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; PMode tutorials in C and Asm                          ;;
;; Copyright (C) 2000 Alexei A. Frounze                  ;;
;; The programs and sources come under the GPL           ;;
;; (GNU General Public License), for more information    ;;
;; read the file gnu-gpl.txt (originally named COPYING). ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

GLOBAL  _isr_00_wrapper, _isr_20_wrapper
EXTERN  _isr_00, _isr_20

SEGMENT _TEXT PUBLIC CLASS=CODE USE16

_isr_00_wrapper:
        push    ds
        push    es                      ; saving segment registers and
        pusha                           ; other regs because it's an ISR
        mov     ax, 10h
        mov     ds, ax
        mov     es, ax                  ; load ds and es with valid selector
        call    _isr_00                 ; call actual ISR code
        popa                            ; restoring the regs
        pop     es
        pop     ds
        iret

_isr_20_wrapper:
        push    ds
        push    es                      ; saving segment registers and
        pusha                           ; other regs because it's an ISR
        mov     ax, 10h
        mov     ds, ax
        mov     es, ax                  ; load ds and es with valid selector
        call    _isr_20                 ; call actual ISR code
        popa                            ; restoring the regs
        pop     es
        pop     ds
        iret

SEGMENT _DATA PUBLIC CLASS=DATA
