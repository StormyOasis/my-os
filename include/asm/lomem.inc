;mem.asm
;contains functions pertaining to the low-level memory system

%ifndef _LOMEM_INC_
%define _LOMEM_INC_

[BITS 32]

section .data

AmtMemPresent		DD 	0				;physical mem on system


;align 4096, db 0
;PageDir			resb 4096
;align 4096, db 0
;PageTables		resb (4096*4)			;want to map the first 16 mb to the first 16mb

;errors
ercNoPagesAvail 	EQU 1


%endif
