;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; PMode tutorials in C and Asm                          ;;
;; Copyright (C) 2000 Alexei A. Frounze                  ;;
;; The programs and sources come under the GPL           ;;
;; (GNU General Public License), for more information    ;;
;; read the file gnu-gpl.txt (originally named COPYING). ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

GLOBAL  _read_msw, _read_cr0, _write_cr0, _lgdt, _lidt, _lldt, _update_cs
GLOBAL  _ltr, _str, _clts, _load_fs, _load_gs, _jump_to_tss

SEGMENT _TEXT PUBLIC CLASS=CODE USE16

_read_msw:
        smsw    ax
        retn

_read_cr0:
        mov     eax, cr0        ; read CR0 to eax
        mov     edx, eax
        shr     edx, 16         ; dx:ax = CR0 (return value)
        retn

_write_cr0:
        push    bp
        mov     bp, sp
        mov     eax, [ss:bp+4]  ; eax = 32-bit parameter
        mov     cr0, eax
        pop     bp
        retn

_lgdt:
        push    bp
        mov     bp, sp
        push    bx
        mov     bx, [ss:bp+4]   ; ds:bx = pointer to GDTR structure
        lgdt    [ds:bx]         ; load GDTR
        pop     bx
        pop     bp
        retn

_lidt:
        push    bp
        mov     bp, sp
        push    bx
        mov     bx, [ss:bp+4]   ; ds:bx = pointer to IDTR structure
        lidt    [ds:bx]         ; load IDTR
        pop     bx
        pop     bp
        retn

_lldt:
        push    bp
        mov     bp, sp
        mov     ax, [ss:bp+4]
        lldt    ax              ; load LDTR
        pop     bp
        retn

_update_cs:
        push    bp
        mov     bp, sp
        mov     ax, [ss:bp+4]   ; ax = new cs
        push    ax              ; push segment
        push    word .1         ; push offset
        retf                    ; we have a new cs now
.1:
        pop     bp
        retn

_ltr:
        push    bp
        mov     bp, sp
        ltr     word [ss:bp+4]
        pop     bp
        retn

_str:
        str     ax
        retn

_clts:
        clts
        retn

_load_fs:
        push    bp
        mov     bp, sp
        mov     fs, [ss:bp+4]
        pop     bp
        retn

_load_gs:
        push    bp
        mov     bp, sp
        mov     gs, [ss:bp+4]
        pop     bp
        retn

_jump_to_tss:
        push    bp
        mov     bp, sp
        jmp far [ss:bp+2]
        pop     bp
        retn

SEGMENT _DATA PUBLIC CLASS=DATA
