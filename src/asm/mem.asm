;mem.asm
;contains functions pertaining to memory

%ifndef _MEM_ASM_
%define _MEM_ASM_

section .text

[BITS 32]

%include "include/asm/mem.inc"


		
1MemMgtLoopEnd:
    
	;page tables
	
	mov edx, PageTables
	xor eax, eax
IMM001:
	mov [edx], eax
	and dword [edx], 0x0fffff000
	or dword [edx], 0x0001
	mov ebx, ebx		
	add edx, 4
	add eax, 4096
	cmp eax, 0x30000
	jae short IMM002
	jmp IMM001
IMM002:
	mov eax, 0x0a0000
	mov ebx, eax
	shr ebx, 10
	mov edx, PageTables
	add edx, ebx
IMM003:
	mov [edx], eax
	and dword [edx], 0x0fffff000
	or dword [edx], 0101b
	mov ebx, eax		
	add edx, 4
	add eax, 4096
	cmp eax, 0x100000
	jae IMM004
	jmp IMM003		
	
IMM004:

	;setup page dir
	mov eax, PageTables
	mov edx, PageDir
	mov [edx], eax
	and dword [edx], 0x0fffff000
	or dword [edx], 000000000101b
	

	MOV EAX, PageDir
	MOV CR3, EAX
	MOV EAX, CR0
	OR EAX, 0x80000000
	MOV CR0, EAX	
	;enable paging

	jmp IMM005
IMM005:
	
	call StackAddresses
	;adjust nFreePages
	sub dword [nFreePages], 192
	sub dword [nFreePagesBelow16], 192
	
	retn
	
;==================================================================================
;StackAddresses - Near
;
;IN: NONE
;OUT: NONE
;
;loads the physical addresses onto appropriate stacks
;

;NOTE!!! Possible Error below...
	
StackAddresses:
	
	mov ebx, bKernPages
	
SABelowLoop:
	
	mov eax, [pStackBelow16ptr]
	mov [eax], ebx
	
	add dword [pStackBelow16ptr], 0x00000004
		
	add ebx, 0x00001000
	
	cmp ebx, 0x01000000
	jb SABelowLoop	
	
SAAboveLoop:

	mov eax, [pStackAbove16ptr]
	mov [eax], ebx
	
	add dword [pStackAbove16ptr], 0x00000004
		
	add ebx, 0x00001000
	
	cmp ebx, [MemTopAddr]
	jbe SAAboveLoop

	retn	
	
%endif
