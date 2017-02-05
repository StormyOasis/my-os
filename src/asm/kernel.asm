;kernel.asm
;contains the kernel primitives
;

[BITS 32]

%ifndef __KERNEL__ASM
%define __KERNEL__ASM

%include "include/asm/data.inc"

section .text

global DoTaskSwitch

DoTaskSwitch:		
	
	push ebp
	mov ebp, esp
	
	mov eax, [ebp + 8]	
	mov word [TSS_Sel], ax		
	
	mov byte [intsEnabled], 1 ;The first task switch is a sign that we are ready to go with ints enabled
	sti
	;we could probably move the two above lines somewhere that will not be called every task switch!
	
	jmp far [TSS]
	
	pop ebp
	
	retn
		
	
%endif
