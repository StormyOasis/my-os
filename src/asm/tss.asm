;tss.asm
;contains functions pertaining to tss and tasks.

%ifndef _TSS_ASM_
%define _TSS_ASM_

[BITS 32]

%include "include/asm/lomem.inc"
%include "include/asm/data.inc"

[SECTION .data]
	
Stack times (1024) db 0

[SECTION .text]

global addTSStoGDT
global ltrOSTSS
global InitOSTSS

global testADDTSS


Func:

	retn

testADDTSS:

	push ebp
	mov ebp, esp	
	
	push eax
	push edi
	push ebx
	push edx
	push ecx
	
	
	;mov edi, 0x20
	;add edi, [GDTBase]	
	
	;PUSH EAX	
	
	;mov eax, SIZEOF_TSS
	;DEC eax					;eax is limit			 
	
	;mov ebx, 0x0089				;edx is DPL, granularity, etc...
	;mov edx, [ebp +8]			;ebx is address of tss structure
	
	;call _AddTSSDesc
	
	;POP EAX
	
	mov ebx, [ebp +8]
		
	mov word  [ebx+102], 0x0ffff	
	mov word  [ebx+76], OSCodeSel
	mov word  [ebx+72], DataSel
	mov word  [ebx+84], DataSel
	mov word  [ebx+88], DataSel
	mov word  [ebx+92], DataSel
	mov word  [ebx+80], DataSel
	mov word  [ebx+8], DataSel	

	
	mov dword [ebx + 28], page_dir 			;cr3
	
	;mov dword [ebx+32], Func 		;eip
	mov dword [ebx + 4], Stack
	mov dword [ebx + 56], Stack
	
	
	pop ecx
	pop edx
	pop ebx
	pop edi
	pop eax	
	
	mov eax, [ebp +8]
	
	pop ebp				
	
	retn
	
InitOSTSS:		
	
	push ebp
	mov ebp, esp	
	
	push eax
	xor eax, eax	
	
	mov ax, OS_TSS_SELECTOR
	
	ltr word ax		
	
	pop eax		
	pop ebp				
	
	retn

addTSStoGDT:
	
	push ebp
	mov ebp, esp
	
	push eax
	push edi
	push ebx
	push edx
	push ecx
	
	;selector 104
	;dpl = 108
		
	mov eax, [ebp + 8]
	
	mov dword edi, [eax + 104]	
	add edi, [GDTBase]
		
	mov edx, [ebp + 8]
		
	mov ebx, 0x0089
	mov eax, 512
	call AddTSSDesc
	
	pop ecx
	pop edx
	pop ebx
	pop edi
	pop eax

	pop ebp

	retn
	
	

	;mov edi, dword [ebp + 8] 	;tss address
	;mov edi, dword [edi + 152]	;tss->selector
	;add edi, [GDTBase]
	
	;mov eax, 511 ;limit is sizeof tss - 1
		
	;mov ebx, [ebp + 108] 	;tss->dpl
	
	;ebx is now the dpl of the tss
	
	;we have to or that value with the rest of the descriptor bits
	;shl ebx, 5		
	;mov ecx, 0x0089
	;OR ebx, ecx
	
	;remove the following line...
	;mov ebx, 0x0089
		
	mov edx, dword [ebp + 8]		;tss address		
	
	;call _AddTSSDesc		
	
	;mov ebx, dword [edx + 152]
	
	;mov edi, _off_tss
	;add edi, ebx
	add edi, [GDTBase]		
	
	mov eax, 512
	DEC eax					;eax is limit			 	
	mov ebx, 0x0089				;edx is DPL, granularity, etc...	
	mov edx, dword [ebp + 8]		;tss address			
	
	call AddTSSDesc	
	
	pop ecx
	pop edx
	pop ebx
	pop edi
	pop eax

	pop ebp

	retn
		
	
;============================================================================
;AddTSSDesc - near
;
;IN: 	eax - limit of tss
;	edi - offset of TSS in GDT
;	ebx - properties of tss
;	edx - address of tss
;
;OUT: GDT is updated with the tss entry
;

AddTSSDesc:

	PUSH EAX
	PUSH EBX
	PUSH EDX
	PUSH EDI
		
	SHL EBX, 16
	ROL EDX, 16
	MOV BL, DH
	MOV BH, DL
	ROR EBX, 8
	MOV DX, AX
	AND EAX, 0x000F0000
	OR EBX, EAX
	MOV [EDI], EDX
	MOV [EDI+04], EBX
		
	POP EDI
	POP EDX
	POP EBX
	POP EAX

	retn

;_InitTSSCacheList:

;	mov eax, 1
;	mov edi, OS_TSS_SELECTOR+8	
;	mov ecx, SIZEOF_TSS

;TSS_Loop:
;	PUSH EAX
;	;first get the address in the aTSS array
;;	mov ebx, _aTSS
;	mul ecx
;	add ebx, eax
;	
;	;ebx should now be the correct offset
;	
;	mov dword [ebx+TSS_NEXT], 0
;	mov word  [ebx+TSS_MAP_BASE], 0x0ffff
;	
;	POP EAX
;	mov word  [ebx+TSS_NUM], ax
;	PUSH EAX
;	
;	mov word  [ebx+TSS_CS], OSCodeSel
;	mov word  [ebx+TSS_DS], DataSel
;	mov word  [ebx+TSS_ES], DataSel
;	mov word  [ebx+TSS_FS], DataSel
;	mov word  [ebx+TSS_GS], DataSel
;	mov word  [ebx+TSS_SS], DataSel
;	mov word [ebx+TSS_SS0], DataSel	
;	
;	mov dword [ebx+TSS_OWNER], 0
;	
;	mov word [ebx+TSS_ACTIVE], 0
;	mov dword [ebx+TSS_TICK], 0	
;
;	
;	jmp TSS_Loop_le
;the following is done in order to cheat around the short jump out of range...This needs to be fixed
;TSS_Loop_l:
;	jmp TSS_Loop
;TSS_Loop_le:
;		
;	PUSH EAX
;	
;	mov eax, edi
;	mov word [ebx+TSS_ID], ax
;	
;	PUSH EDI
;
;	mov eax, SIZEOF_TSS
;	mov edx, ebx
;	mov ebx, 0x0089
;	
;	add edi, [GDTBase]
;	
;	CALL _AddTSSDesc	
;	
;	POP EDI
;	POP EAX
;		
;	ADD EDI, 8	;point to next gdt slot
;	POP EAX
;	inc eax
;	;cmp eax, nTSS
;	jl TSS_Loop_l
;	
;	retn
;	
_InitIdleTaskInfo:
;
;	;we want to initialize the tss for the system idle loop process 
;	;and add it into the gdt
;	
;	;calculate position in gdt.  This should be the last tss in the gdt.			
;	mov ecx, 8
;	mov eax, nTSS
;	mul ecx
;	add eax, 8
;	
;	;eax is now the location in the gdt (nTSS+1) * 8	
;	
;	mov edi, eax
;	add edi, [GDTBase]	
;	
;	PUSH EAX	
	
;	mov eax, SIZEOF_TSS
;	DEC eax					;eax is limit			 
;	
;	mov ebx, 0x0089			;edx is DPL, granularity, etc...
;	;mov edx, _IdleTSS			;ebx is address of tss structure
;	
;	call _AddTSSDesc
;	
;	POP EAX
;	
;	;mov ebx, _IdleTSS
;		
;	mov dword [ebx+TSS_NEXT], 0
;	mov word  [ebx+TSS_MAP_BASE], 0x0ffff
;		
;	mov word [ebx+TSS_NUM], 101	
;	
;	mov word [ebx+TSS_ID], ax
;	
;	mov word  [ebx+TSS_CS], OSCodeSel
;	mov word  [ebx+TSS_DS], DataSel
;	mov word  [ebx+TSS_ES], DataSel
;	mov word  [ebx+TSS_FS], DataSel
;	mov word  [ebx+TSS_GS], DataSel
;;	mov word  [ebx+TSS_SS], DataSel
;	mov word  [ebx+TSS_SS0], DataSel	
;;	
;	mov word [ebx+TSS_ACTIVE], 0
;	mov dword [ebx+TSS_TICK], 0
;
;	retn
;


%endif
