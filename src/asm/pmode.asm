;pmode.asm

[BITS 32]

;section .text

;global EnterProtectedMode
	
	
;EnterProtectedMode:
	
;o32	LGDT [GDTptr]
;o32	LIDT [IDTptr]
	
	;enter pmode...
	
;	mov eax,cr0
;	or al,1
;	mov cr0,eax
;	nop				

;	db 0x66				; operand size override prefix
;	db 0xea				; jmp opcode
;	dd (0x01000<<4) + pm     ; want to jump here
;	dw 0x8   				; ...in the kernel code segment
			
;GDTptr:
;	GDTLimit	DW 0x17ff
;	GDTBase	DD 0x0800	
	
;IDTptr:
;IDTLimit		DW 0x7ff
;IDTBase		DD 0x00000000		
				 		
;[BITS 32]
;pm:
	;reload selectors

;	mov ax, 0x10
 ;	mov ds, ax
	;mov es, ax
	;mov fs, ax
	;mov gs, ax
;	mov ss, ax	
	
	;dont know about this..but want to return to the c func
	
		
;	retn
