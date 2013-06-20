;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; PMode tutorials in C and Asm                          ;;
;; Copyright (C) 2000 Alexei A. Frounze                  ;;
;; The programs and sources come under the GPL           ;;
;; (GNU General Public License), for more information    ;;
;; read the file gnu-gpl.txt (originally named COPYING). ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

GLOBAL  _task1, _task2

SEGMENT _TEXT PUBLIC CLASS=CODE USE32

_task1:
        mov     ax, 23h
        mov     es, ax
        mov     ebx, 80*10+10*2
.1
        inc     byte [es:ebx]
        jmp     .1

_task2:
        mov     ax, 23h
        mov     es, ax
        mov     ebx, 80*12+10*2
.1
        inc     byte [es:ebx]
        jmp     .1

SEGMENT _DATA PUBLIC CLASS=DATA
