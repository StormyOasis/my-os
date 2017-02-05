;lomem.asm
;contains functions pertaining to memory

%ifndef _LOMEM_ASM_
%define _LOMEM_ASM_

[BITS 32]

section .text

extern page_dir
extern page_table
	
global enablePaging
enablePaging:

	push	ebp
	mov	ebp, esp

;map roms
	mov eax, 0x0a0000
	mov ebx, eax
	shr ebx, 10
	mov edx, page_table
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
	mov eax, page_table
	mov edx, page_dir
	mov [edx], eax
	and dword [edx], 0x0fffff000
	or dword [edx], 000000000101b
	
	MOV EAX, page_dir
	MOV CR3, EAX
	MOV EAX, CR0
	OR EAX, 0x80000000
	MOV CR0, EAX	
	;enable paging

	jmp IMM005
IMM005:

	mov esp, ebp
	pop ebp

	retn	

%endif
