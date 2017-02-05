;data.inc
;data definitions

[BITS 32]

%ifndef __DATA_INC_
%define __DATA_INC_

[SECTION .data]

%define work_around_nasm_prepend_bug
%include "include/asm/staticdata.inc"

extern _pCurrentTSS
extern _cCurrentLevel

global OsMsgBin
global nMsgBin
global bHalted
global TaskSwitches
global pCurrentTSS
global pTssHead
global nSwitches
global TSS_Sel
global nTSSNum
global pNextTSS
global intsEnabled
global conventionalMemory
global extendedMem
global TickCount
global SwitchTick
global _nRdyQ
global TSS
global TSS_Sel
global BootDrive
global GDTBase
global gdtr
global off_tss
global tss_sel_loc

OSCodeSel			EQU 0x0008			;OS code is 0 based on this selector
DataSel				EQU 0x0010			;All data is 0 based on this selector
UserCodeSel			EQU 0x18
NULL				EQU 0				;Duh...
nCallGates			EQU 50
nIDTGates			EQU 0xff
nTSS 				EQU 128				;Same as TSS_CACHE_SIZE in tss.h
SIZEOF_TSS			EQU	512
MSG_DUMMY			EQU	0x00000000

;Error Codes
ercBadCallGate		EQU 50				;not used... for now....
ercBadGateNum  		EQU 48				;tried to add an invalid GDT call gate


nTSSNum				DW 1
nSwitches			DD 0
intsEnabled 		DB 0
conventionalMemory 	DD 0
extendedMem 		DD 0
OSStackTop			DD 0x00000000
TickCount			DD 0
SwitchTick 			DD 0
bHalted				DB 0
TaskSwitches		DD 0
TSS					DD 0
TSS_Sel				DW 0
BootDrive 			DB 00
_nRdyQ				DD 0x000000
tss_sel_loc  		DD (off_tss - gdt)
OldIDTptr:
	OldIDTLimit		DW 0
	OldIDTBase		DD 0
	
;---------------------------------------------------------------------------------------
;GDT structure
;

gdtr
	GDTLimit dw gdt_end-gdt-1	; length of gdt 
	GDTBase dd gdt	    		; linear, physical address of gdt	
	
align 8, db 0
gdt:    			; null descriptor
	dw 0			; limit 15:0
	dw 0			; base 15:0		
	db 0			; base 23:16
	db 0			; type
	db 0			; limit 19:16, flags
	db 0			; base 31:24

; code segment descriptor
	;0x8
    dw 0xFFFF   	; limit 0xFFFFF
	dw 0x0000		; base 15:0
	db 0x00
	db 0x9A			; present, ring 0, code, non-conforming, readable
    db 0xCF    	 	; page-granular, 32-bit
	db 0x00	

; data segment descriptor
	;0x10
	dw 0xFFFF		; limit 0xFFFFF
	dw 0			; base 15:0
	db 0
	db 0x92			; present, ring 0, data, expand-up, writable
    db 0xCF        	; page-granular, 32-bit
	db 0x00
	
; user segment descriptor
	;0x18
     dw 0xFFFF      ; limit 0xFFFFF
	dw 0x0000		; base 15:0
	db 0x00
	db 0x9A			; present, ring 0, code, non-conforming, readable
    db 0xCF        	; page-granular, 32-bit
	db 0x00		
					
REAL_CODE_SEL equ $-gdt	; code segment descriptor for 16 bit mode
	dw 0xFFFF
	dw 0x0   		; base
	db 0x0          ; base
	db 0x9A         ; present, ring 0, code, non-conforming, readable
	db 0x00   		; 16 bit
	db 0		

REAL_DATA_SEL equ $-gdt	; data segment descriptor for 16 bit mode

	dw 0xFFFF
	dw 0x0          ; base
	db 0x0          ; base
	db 0x92         ; present, ring 0, data, expand-up, writable
	db 0x00			; 16 bit
	db 0
	
VIDEO_DATA_SEL	equ	$-gdt
	dw 3999			; limit 80*25*2-1
	dw 0x8000		; base 0xB8000
	db 0x0B
	db 0x92			; present, ring 0, data, expand-up, writable
	db 0			; byte-granular, 16-bit
	db 0
	
	Res5 dd 0,0		;Sel 0x38
	
	;reserve space for call gates
	times (nCallGates * 2) dd 0	;first: sel 0x40
off_tss:
	times ((nTSS) * 2) dd 0
	
gdt_end		
	
	;add one to nTSS for the idle task
	OS_TSS_SELECTOR equ (off_tss - gdt)	;this should be a selector number

;end struct


;---------------------------------------------------------------------------------------
;IDT structure
;

global idtr
idtr
	IDTLimit dw idt_end-idt-1	; length of gdt 
	IDTBase dd idt	    		; linear, physical address of gdt	

idt:
	times (nIDTGates * 2) dd 0	
idt_end
	
	
	
;end struct


%endif
