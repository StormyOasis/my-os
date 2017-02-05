;initmain.asm
;placed here after entering protected mode.
;Begins initialization of Operating System

[BITS 32]
[SECTION .text]

global entry_point
entry_point:
start:
.386P	

	mov ax,0x10
	mov ds,ax 
	mov es,ax
	mov fs,ax
	mov ss,ax	
	mov gs, ax
	
	mov [conventionalMemory], ebx
	mov [extendedMem], ecx			
	
	jmp go

extern main

%include "include/asm/data.inc"
%include "src/asm/gdt.asm"
%include "src/asm/idt.asm"
%include "src/asm/timer.asm"
%include "src/asm/keyboard.asm"
%include "src/asm/tss.asm"
%include "src/asm/lomem.asm"
%include "src/asm/video.asm"
%include "src/asm/kernel.asm"


go:
	cli		
	
	mov [BootDrive], DL		
	
	SIDT [OldIDTptr]

	;mov edi, 0x500
    ; 	mov esi, 0
    ; 	mov ecx, 1024
    ; 	cld
    ; 	rep movsb     	     	
	
		
	;mov edi, 0x7e01
     ;	mov esi, OldIDTptr
     ;	mov ecx, 6
     ;	cld
     ;	rep movsb     	     	
     	
     	;Now lets move all of the code     	     	
     	
     ;	mov edi, 0x7e07
     ;	mov esi, Pm16RestartCode
     ;	mov ecx, PM16_SIZE
     ;	cld
     ;	rep movsb   
     	
     ;	mov edi, 0x8206
     ;	mov esi, RealRestartCode
     ;	mov ecx, REAL_SIZE
     ;	cld
     ;	rep movsb     	  	
     					
	
	
	;mov [_conventionalMemory], ecx
	;mov [_extendedMem], ebx			

	;MOV [_BootDrive], EDX 		
	
	;Don't remeber what I was doing with the stack here.
	;By doing the following, Later on in the C code, I bump
	;up against the end of the stack and everything breaks
	 
	;LEA EAX, [OSStackTop]
	;MOV ESP, EAX	
		
	
	call GDTInit	
	
	call SetupBasicHardware				
	
	call main
	
	hlt


;===================================================================================
;SetupBasicHardware
;
;IN:  NONE
;OUT: NONE
;
;Sets up basic items such as the timer, video, and picu

SetupBasicHardware:

	push ebp
	mov ebp, esp	
	
	;setup idt
	
	cli
	call InitIDTSubsystem		

	;now setup the 8259's
	call Init8259
		
	;setup the keyboard
	call InitKeyboard
	
	cli
	in AL, 0x60 	;read byte
				
	
	;send end of interrupt
	PUSH word 0
	call 0x50:0x00	
	
	;setup the system timer
	call SetupTimer	
	
	call InitVideo				
		
	mov esp, ebp
	pop ebp
	
	retn	
	
	
;===================================================================================
;GDTInit
;
;IN:  NONE
;OUT: NONE
;
;Initializes the gdt...again
	
GDTInit:		
	
	push ebp
	mov ebp, esp	

	;move gdt to address 0x8830 and reload because we want a new base and limit	
	;mov ecx,40
	;mov eax,10h
	;mov ds,ax
	;mov es,ax
	;mov esi,gdt
	;mov edi, 0x800
	;rep movsd		

     ;reload it
	lgdt [gdtr]				
	
	jmp dword 0x8:NewCS
	nop
	nop
NewCS:	

	call InitCallGates		
	
	mov esp, ebp
	pop ebp
	
	retn			
	
;===================================================================================
;_RestartSystem
;
;
;Restarts the load process for debugging

extern clrscr

global RestartSystem
RestartSystem:	

	call clrscr
	
	cli			
	
	;clear the PG bit
	mov eax,cr0
	and eax,7FFFFFFFh
	mov cr0,eax

	;Move zeros to CR3 to clear out the paging cache.
	xor eax,eax
	mov cr3,eax	
	
	mov edi, 0
     	mov esi, 0x500
     	mov ecx, 1023
     	cld
     	rep movsb 
     	
;o32	lidt [RealIDT]     				  	     	   	     	     		     	
     	
		
	jmp word REAL_CODE_SEL:0x7e07 ;0x7e00 = RealIDT address, add 6 to get address of Pm16RestartCode

	
	
[BITS 16]

;RealIDT:
;	dw 1023 ; limit of 1023
;	dd 0	
	
Pm16RestartCode:						
	
	mov ax,REAL_DATA_SEL
	mov ds,ax
	mov es,ax	
	mov ss,ax
	mov fs,ax
	mov gs,ax		
	mov esp,0xffff
		
	mov word bx, 0x8206
	
	;Now clear the PE bit
	mov eax,cr0
	and al,0FEh
	mov cr0,eax	
	nop		
	
	jmp bx ;Should jmp to RealRestartCode in true real mode

	
Pm16RestartEnd:
PM16_SIZE equ ($-Pm16RestartCode)	
	
RealRestartCode:
	;the second stage boot loader should still be in its original location. Jump to it.				
	
;o32	lidt [0x7e01]
	
	mov ax,cs
	mov ss,ax
	mov ds,ax
	mov es,ax
	mov fs,ax
	mov gs,ax 
		
	
	xor eax, eax
	xor ecx, ecx
	xor edx, edx
	xor edi, edi
	xor esi, esi
	xor ebx, ebx
	xor ebp, ebp
	xor esp ,esp					
			
	;reset the system stack
	mov ax, 0x9000 			; Put stack at 0x90000
	mov ss, ax			; SS = 9000 and
	mov sp, 0xFBFF			; SP = 0000 => Stack = 90000	
	
	;make sure a20 is still enabled
IBEmm0:
	IN AL, 0x64
	TEST AL, 0x02
	LOOPNZ IBEmm0
	MOV AL, 0x0D1
	OUT 0x64, AL
	XOR CX, CX
IBEmm1:
	IN AL, 0x64
	TEST AL, 0x02
	LOOPNZ IBEmm1
	MOV AL, 0x0DF
	OUT 0x60, AL
	XOR CX, CX
IBEmm2:
	IN AL, 0x64
	TEST AL, 0x02
	LOOPNZ IBEmm2	
	
 	PICA00 EQU 20H ;master 8259 port 0
	PICA01 EQU 21H ;master 8259 port 1

	;restores the master pic to the original setting.
	MOV AL,11H
	OUT PICA00,AL
	MOV AL,08H ;Base vector = 8
	OUT PICA01,AL
	MOV AL,4 ;Slave on ir2
	OUT PICA01,AL
	MOV AL,1
	OUT PICA01,AL
	MOV AL,0 ;Restore original mask
	OUT PICA01,AL
	IN AL,60H ;Must clear keybd interrupt on	
	
	
	mov dl, 0
			
	mov ax, 0x8E00
	mov es, ax
	mov ds, ax
	push ax
	mov ax, 0
	push ax				
	
	retf	;should now restart the 2nd stage bootloader	
	
RealCodeEnd:
REAL_SIZE equ ($-RealRestartCode)
