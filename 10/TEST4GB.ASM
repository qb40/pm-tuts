;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; PMode tutorials in C and Asm                          ;;
;; Copyright (C) 2000 Alexei A. Frounze                  ;;
;; The programs and sources come under the GPL           ;;
;; (GNU General Public License), for more information    ;;
;; read the file gnu-gpl.txt (originally named COPYING). ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

GLOBAL  _test_4gb

SEGMENT _TEXT PUBLIC CLASS=CODE USE16

_test_4gb:
        push    es
        xor     ax, ax
        mov     es, ax
        mov     ecx, msg_end-msg
        mov     esi, msg
        mov     edi, 0B8000h+80*4
        cld
        db 67h                          ; use 32-bit registers in 16-bit code
        rep     movsb
        pop     es
        retn

SEGMENT _DATA PUBLIC CLASS=DATA
msg     DB      "  4   G B   S e g m e n t   i n   R e a l   M o d e !   "
        DB      "( a k a   B i g / U n r e a l   M o d e )   "
msg_end:
