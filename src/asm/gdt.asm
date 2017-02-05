;gdt.asm
;contains functions pertaining to gdt and call gates

%ifndef __GDT_ASM
%define __GDT_ASM

[BITS 32]

%include "include/asm/data.inc"
%include "src/asm/kernel.asm"
%include "src/asm/idt.asm"

[SECTION .data]
TempGDTLimit dw 0
TempGDTBase  dd 0

[SECTION .text]

global call_gate

extern CG_CreateProcess
extern CG_GetNextProc
extern CG_TaskSwitch
extern CG_ScheduleProc
extern CG_GetCurrentTick
extern CG_SendMessage
extern CG_WaitMessage

;*===========================================================================*
;*           call_gate                                                       *
;*===========================================================================*
; PUBLIC void call_gate( u16 segment );
; Executes a system call
;
;From nasm documentation:
;
;The CALL FAR mem forms execute a far call by loading the destination
;address out of memory. The address loaded consists of 16 or 32 bits of
;offset (depending on the operand size), and 16 bits of segment. The
;operand size may be overridden using CALL WORD FAR mem or CALL DWORD
;FAR mem.
;
;PR_ARGS =      + 4		+ 4
;         = eip		segment
;		= offset
;		= dont care!
call_gate:
	jmp	far [esp]		
    ret
        
;use in c for calling call gates: 
;call_gate( (u16) 0x20 );
;where 0x20 is selector number

InitCallGates:

	SGDT [TempGDTLimit]	;get the gdt base address

	;first, add the __AddCallGate function as a call gate
	
	MOV EAX, 0x08C00
	MOv ECX, 0x40
	MOV DX, 0x8
	MOV ESI, __AddCallGate
	MOVZX EBX, CX
	SUB EBX, 40
	SHR EBX, 3
	MOVZX EBX, CX
	add ebx, [TempGDTBase]
	MOV WORD [EBX+02], 0x8
	MOV [EBX], SI
	SHR ESI, 16
	MOV [EBX+06], SI
	MOV [EBX+04], AX	
	
	;now that the chicken/egg problem is solved, add the rest of the os publics
	;in as call gates	
	
	;__AddIDTGate	- DPL 0 DWORD 0
	MOV EAX, 0x08C00
	MOV ECX, 0x48
	MOV DX,  0x8
	MOV ESI, __AddIDTGate		
	
	call 0x40:0x00
	
	;__EndOfIRQ	- DPL 0 DWORD 1
	MOV EAX, 0x08C01
	MOV ECX, 0x50
	MOV DX,  0x8
	MOV ESI, __EndOfIRQ		
	
	call 0x40:0x00	
	
	;__MaskIRQ	- DPL 0 DWORD 1
	MOV EAX, 0x08C01
	MOV ECX, 0x58
	MOV DX,  0x8
	MOV ESI, __MaskIRQ		
	
	call 0x40:0x00		
	
	;__UnMaskIRQ	- DPL 0 DWORD 1
	MOV EAX, 0x08C01
	MOV ECX, 0x60
	MOV DX,  0x8
	MOV ESI, __UnMaskIRQ		
	
	call 0x40:0x00		
	
	;_CG_CreateProcess	- DPL 0 DWORD 8
	MOV EAX, 0x08C08
	MOV ECX, 0x68
	MOV DX,  0x8
	MOV ESI, CG_CreateProcess		
	
	call 0x40:0x00	
	
	;_CG_GetNextProc	- DPL 0 DWORD 0
	MOV EAX, 0x08C00
	MOV ECX, 0x70
	MOV DX,  0x8
	MOV ESI, CG_GetNextProc		
	
	call 0x40:0x00	
	
	;_CG_TaskSwitch	- DPL 0 DWORD 1
	MOV EAX, 0x08C01
	MOV ECX, 0x78
	MOV DX,  0x8
	MOV ESI, CG_TaskSwitch
	
	call 0x40:0x00	
	
	;_CG_ScheduleProc	- DPL 0 DWORD 1
	MOV EAX, 0x08C01
	MOV ECX, 0x80
	MOV DX,  0x8
	MOV ESI, CG_ScheduleProc
	
	call 0x40:0x00		
	
	;_CG_GetCurrentTick	- DPL 0 DWORD 0
	MOV EAX, 0x08C00
	MOV ECX, 0x88
	MOV DX,  0x8
	MOV ESI, CG_GetCurrentTick
	
	call 0x40:0x00		
	
	;_CG_SendMessage	- DPL 0 DWORD 5
	MOV EAX, 0x08C05
	MOV ECX, 0x90
	MOV DX,  0x8
	MOV ESI, CG_SendMessage
	
	call 0x40:0x00	
	
	;_CG_WaitMessage	- DPL 0 DWORD 1
	MOV EAX, 0x08C01
	MOV ECX, 0x98
	MOV DX,  0x8
	MOV ESI, CG_WaitMessage
	
	call 0x40:0x00									
									
	retn
	

;===================================================================================
;AddCallGate
;
;IN: AX - Word with Call Gate ID type as follows:
;
;			DPL entry of 3 EC0x   (most likely, Application level)
;			DPL entry of 2 CC0x   (Not used)
;			DPL entry of 1 AC0x   (Not used)
;			DPL entry of 0 8C0x   (OS call ONLY)
;			(x = count of DWord params 0-F)
;
;     CX - Selector number for call gate in GDT (constants!)
;     ESI - Offset of entry point in segment of code to execute
;
; OUT:  EAX	Returns Errors, else 0 if all's well
;
;Adds the function call gate into the gdt

__AddCallGate:	

	CMP CX, 40h 			;Is number within range of callgates?
	JAE AddCG01			;not too low.
	MOV EAX, ercBadGateNum
	RETF
AddCG01:
	MOVZX EBX, CX
	SUB EBX, 40			;sub call gate base selector
	SHR EBX, 3			;make index vice selector
	CMP EBX, nCallGates		;see if too high!
	JBE AddCG02			;No.
	MOV EAX, ercBadGateNum	;Yes.	
	RETF
AddCG02:

	MOVZX EBX, CX			;Extend selector into EBX
	ADD EBX, [GDTBase]		;NOW a true offset in GDT
	MOV WORD [EBX+02], 8	;Put Code Seg selector into Call gate
	MOV [EBX], SI			;0:15 of call offset
	SHR ESI, 16			;move upper 16 of offset into SI
	MOV [EBX+06], SI		;16:31 of call offset
	MOV [EBX+04], AX		;call DPL & ndParams
	XOR EAX, EAX			;0 = No Errors
	RETF

%endif
